/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

// Declarations from juce_emscripten_Messaging.
extern void registerCallbackToMainThread (std::function<void()> f);

int getAudioContextSampleRate() {
    return MAIN_THREAD_EM_ASM_INT({
        var AudioContext = window.AudioContext || window.webkitAudioContext;
        var ctx = new AudioContext();
        var sr = ctx.sampleRate;
        ctx.close();
        return sr;
    });
}

//==============================================================================
class OpenALAudioIODevice : public AudioIODevice
{
    static const int numBuffers{3};
    int bufferSize{512};
    double sampleRate{44100.0};
    int numIn{0}, numOut{2};
    int numUnderRuns{0};
    bool playing{false};

    ALCdevice* device{nullptr};
    ALCcontext* context{nullptr};
    ALuint source, buffers[numBuffers];
    ALuint frequency;
    ALenum format, errorCode{AL_NO_ERROR};

public:
    class AudioFeedStateMachine
    {
    public:
        enum StateType
        {
            StateWaitingForInteraction,
            StatePlaying,
            StateStopped
        };

        enum StatusType
        {
            StatusGood,
            StatusError,
            StatusNeedToWait
        };

    private:
        OpenALAudioIODevice* parent{nullptr};
        AudioIODeviceCallback* callback{nullptr};
        ALuint source, buffers[numBuffers];
        int numOut, bufferSize;

        int16* formatBuffer{nullptr};
        float** buffersIn{nullptr};
        float** buffersOut{nullptr};

        StateType state{StateWaitingForInteraction};

    public:
        AudioFeedStateMachine (OpenALAudioIODevice* parent) : parent(parent) { }

        ~AudioFeedStateMachine ()
        {
            OpenALAudioIODevice::sessionsOnMainThread.removeAllInstancesOf (this);

            if (StatePlaying)
            {
                callback->audioDeviceStopped();
            }

            if (formatBuffer)
                delete [] formatBuffer;
            if (buffersOut)
            {
                for (int i = 0; i < parent->numOut; i ++)
                    delete [] buffersOut[i];
                delete [] buffersOut;
            }
            if (buffersIn)
                delete [] buffersIn;
        }

        StateType getState () const { return state; }

        void start (AudioIODeviceCallback* callback)
        {
            this->callback = callback;
            callback->audioDeviceAboutToStart (parent);

            source = parent->source;
            for (int i = 0; i < numBuffers; i ++)
                buffers[i] = parent->buffers[i];
            
            numOut = parent->numOut;
            bufferSize = parent->bufferSize;
        }

        // Loosely adapted from https://kcat.strangesoft.net/openal-tutorial.html
        StatusType nextStep (bool shouldStop = false)
        {
            if (state == StateWaitingForInteraction)
            {
                if (shouldStop)
                {
                    state = StateStopped;
                    return StatusGood;
                }
                int status = MAIN_THREAD_EM_ASM_INT ({
                    return window.juce_hadUserInteraction;
                });
                if (status)
                {
                    state = StatePlaying;
                    
                    alSourceQueueBuffers (source, numBuffers, buffers);
                    alSourcePlay (source);
                    if ((parent->errorCode = alGetError()) != AL_NO_ERROR)
                    {
                        DBG("OpenAL error occurred when starting to play.");
                        return StatusError;
                    }

                    formatBuffer = new int16[bufferSize * numOut];
                    buffersIn  = new float*[16];
                    buffersOut = new float*[16];
                    for (int i = 0; i < numOut; i ++)
                        buffersOut[i] = new float[bufferSize];
                }
            }
            if (state == StatePlaying)
            {
                if (shouldStop)
                {
                    state = StateStopped;
                    callback->audioDeviceStopped();
                    return StatusGood;
                }

                ALuint buffer;
                ALint val;

                alGetSourcei (source, AL_SOURCE_STATE, & val);
                if(val != AL_PLAYING)
                    alSourcePlay (source);
                
                alGetSourcei (source, AL_BUFFERS_PROCESSED, & val);
                if (val <= 0) return StatusNeedToWait;
                if (val == numBuffers)
                    parent->numUnderRuns ++;

                int bytePerSampleOut = 2 * numOut;

                while (val --)
                {
                    for (int c = 0; c < numOut; c ++)
                        for (int i = 0; i < bufferSize; i ++)
                            buffersOut[c][i] = 0;
                    
                    callback->audioDeviceIOCallback (
                        (const float**)buffersIn, parent->numIn,
                        buffersOut, numOut, bufferSize);
                    
                    for (int c = 0; c < numOut; c ++)
                    {
                        for (int i = 0; i < bufferSize; i ++)
                        {
                            float x = std::min(buffersOut[c][i], 1.0f);
                            x = std::max(x, -1.0f);
                            formatBuffer[c + i * numOut] = x * 32767;
                        }
                    }
                    
                    alSourceUnqueueBuffers (source, 1, & buffer);
                    alBufferData (buffer, parent->format, formatBuffer,
                        bufferSize * bytePerSampleOut, parent->frequency);
                    alSourceQueueBuffers (source, 1, & buffer);

                    if ((parent->errorCode = alGetError()) != AL_NO_ERROR)
                    {
                        DBG("OpenAL error occurred when playing.");
                        return StatusError;
                    }
                }
            }
            return StatusGood;
        }
    };

    struct AudioThread : public Thread
    {
        AudioFeedStateMachine stateMachine;
        OpenALAudioIODevice* parent;

        AudioThread (OpenALAudioIODevice* parent)
          : Thread("OpenAL Audio Thread"), stateMachine(parent)
        {
            this->parent = parent;
        }

        ~AudioThread () { }

        void start (AudioIODeviceCallback* callback)
        {
            DBG("Starting OpenAL Audio Thread...");
            stateMachine.start (callback);
            startThread (Thread::realtimeAudioPriority);
        }

        void stop ()
        {
            DBG("Stopping OpenAL Audio Thread...");
            stopThread (500);
        }

        void run () override
        {
            while (stateMachine.getState() != AudioFeedStateMachine::StateStopped)
            {
                auto status = stateMachine.nextStep (threadShouldExit());
                if (status == AudioFeedStateMachine::StatusNeedToWait)
                    sleep (1);
            }
        }
    };

public:
    OpenALAudioIODevice (bool threadBased = false)
    : AudioIODevice ("OpenAL", "OpenAL"), threadBased(threadBased)
    {
        DBG("OpenALAudioIODevice: constructor");
    }

    ~OpenALAudioIODevice ()
    {
        DBG("OpenALAudioIODevice: destructor");
        if (isDeviceOpen)
            close();
    }

    StringArray getOutputChannelNames () override
    {
        return { "Out #1", "Out #2" };
    }

    StringArray getInputChannelNames () override
    {
        return {};
    }

    Array<double> getAvailableSampleRates () override {
        return { getAudioContextSampleRate() };
    }
    
    Array<int> getAvailableBufferSizes () override 
    {
        return { 128, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 8192 };
    }
    
    int getDefaultBufferSize () override
    {
        return 2048;
    }

    String open (const BigInteger& inputChannels,
                 const BigInteger& outputChannels,
                 double sampleRate,
                 int bufferSizeSamples) override
    {
        DBG("OpenALAudioIODevice: open");
        ScopedLock lock (sessionsLock);
        return openInternal (inputChannels, outputChannels,
            sampleRate, bufferSizeSamples);
    }
    
    void close () override
    {
        DBG("OpenALAudioIODevice: close");
        ScopedLock lock (sessionsLock);
        closeInternal();
    }

    bool isOpen () override
    {
        return isDeviceOpen;
    }

    void start (AudioIODeviceCallback* newCallback) override
    {
        DBG("OpenALAudioIODevice: start");
        ScopedLock lock (sessionsLock);
        startInternal (newCallback);
    }

    void stop () override
    {
        DBG("OpenALAudioIODevice: stop");
        ScopedLock lock (sessionsLock);
        if (isPlaying())
            stopInternal();
    }

    bool isPlaying () override
    {
        return playing;
    }

    String getLastError () override
    {
        switch (errorCode)
        {
        case AL_NO_ERROR:
            return "";
        case AL_INVALID_NAME:
            return "AL_INVALID_NAME";
        case AL_INVALID_ENUM:
            return "AL_INVALID_NAME";
        case AL_INVALID_VALUE:
            return "AL_INVALID_VALUE";
        case AL_INVALID_OPERATION:
            return "AL_INVALID_OPERATION";
        case AL_OUT_OF_MEMORY:
            return "AL_OUT_OF_MEMORY";
        default:
            return "UNDEFINED_ERROR";
        }
    }

    //==============================================================================
    int getCurrentBufferSizeSamples () override           { return bufferSize; }
    double getCurrentSampleRate () override               { return sampleRate; }
    int getCurrentBitDepth () override                    { return 16; }

    BigInteger getActiveOutputChannels () const override
    {
        BigInteger b;
        b.setRange (0, numOut, true);
        return b;
    }
    BigInteger getActiveInputChannels () const override {
        BigInteger b;
        b.setRange (0, numIn, true);
        return b;
    }

    int getOutputLatencyInSamples () override
    {
        return numBuffers * bufferSize;
    }

    int getInputLatencyInSamples () override
    {
        return numBuffers * bufferSize;
    }

    int getXRunCount () const noexcept override
    {
        return numUnderRuns;
    }
    
    static CriticalSection sessionsLock;
    static Array<AudioFeedStateMachine*> sessionsOnMainThread;

private:
    bool isDeviceOpen{false};
    bool threadBased{false};

    String openInternal (const BigInteger& inputChannels,
                         const BigInteger& outputChannels,
                         double sampleRate,
                         int bufferSizeSamples)
    {
        closeInternal();

        this->bufferSize = bufferSizeSamples;
        this->sampleRate = sampleRate;

        ALenum errorCode = alGetError ();
        device = alcOpenDevice (nullptr);
        if (device == nullptr)
            return "Failed to open device.";
        DBG("OpenAL device has opened.");

        context = alcCreateContext (device, nullptr);
        alcMakeContextCurrent (context);
        if (context == nullptr || (errorCode = alGetError()) != AL_NO_ERROR)
            return "Failed to create context.";
        DBG("OpenAL context is created.");

        alGenBuffers (numBuffers, buffers);
        alGenSources (1, & source);
        if ((errorCode = alGetError()) != AL_NO_ERROR)
            return "Failed to generate sources.";
        DBG("OpenAL sources and buffers are generated.");

        if (numOut == 1)
            format = AL_FORMAT_MONO16;
        else if (numOut == 2)
            format = AL_FORMAT_STEREO16;
        else
            return "Invalid output channel configuration.";
        frequency = (ALuint)sampleRate;
        
        isDeviceOpen = true;

        return {};
    }
    
    void closeInternal ()
    {
        stopInternal();

        if (isDeviceOpen)
        {
            alDeleteSources (1, & source);
            alDeleteBuffers (numBuffers, buffers);
        }
        if (context)
        {
            alcMakeContextCurrent (nullptr);
            alcDestroyContext (context);
            context = nullptr;
        }
        if (device)
        {
            alcCloseDevice (device);
            device = nullptr;
        }
        isDeviceOpen = false;
    }

    void startInternal (AudioIODeviceCallback* newCallback)
    {
        numUnderRuns = 0;
        if (threadBased)
        {
            audioThread.reset (new AudioThread(this));
            audioThread->start (newCallback);
        } else
        {
            audioStateMachine.reset (new AudioFeedStateMachine(this));
            audioStateMachine->start (newCallback);
            sessionsOnMainThread.add (audioStateMachine.get());
        }
        playing = true;
    }

    void stopInternal ()
    {
        if (audioThread)
            audioThread->stop ();
        if (audioStateMachine)
            audioStateMachine.reset (nullptr);
    }

    std::unique_ptr<AudioThread> audioThread;
    std::unique_ptr<AudioFeedStateMachine> audioStateMachine;
};

Array<OpenALAudioIODevice::AudioFeedStateMachine*>
OpenALAudioIODevice::sessionsOnMainThread;
CriticalSection OpenALAudioIODevice::sessionsLock;

//==============================================================================
struct OpenALAudioIODeviceType  : public AudioIODeviceType
{
    OpenALAudioIODeviceType () : AudioIODeviceType ("OpenAL")
    {
        MAIN_THREAD_EM_ASM({
            if (window.juce_hadUserInteraction == undefined)
            {
                window.juce_hadUserInteraction = false;
                window.juce_interactionListener = function() {
                    window.juce_hadUserInteraction = true;
                    window.removeEventListener("mousedown",
                      window.juce_interactionListener);
                };
                window.addEventListener("mousedown",
                    window.juce_interactionListener);
            }
        });
        
        if (! openALMainThreadRegistered)
        {
            registerCallbackToMainThread ([]() {
                using AudioFeedStateMachine = OpenALAudioIODevice::AudioFeedStateMachine;
                ScopedLock lock (OpenALAudioIODevice::sessionsLock);
                for (auto* session : OpenALAudioIODevice::sessionsOnMainThread)
                {
                    if (session->getState() != AudioFeedStateMachine::StateStopped)
                        session->nextStep ();
                }
            });
            openALMainThreadRegistered = true;
        }
    }

    bool openALMainThreadRegistered{false};

    StringArray getDeviceNames (bool) const override                       { return StringArray ("OpenAL"); }
    void scanForDevices () override                                        {}
    int getDefaultDeviceIndex (bool) const override                        { return 0; }
    int getIndexOfDevice (AudioIODevice* device, bool) const override      { return device != nullptr ? 0 : -1; }
    bool hasSeparateInputsAndOutputs () const override                     { return false; }

    AudioIODevice* createDevice (const String& outputName, const String& inputName) override
    {
        if (outputName == "OpenAL" || inputName == "OpenAL")
            return new OpenALAudioIODevice();

        return nullptr;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OpenALAudioIODeviceType)
};

} // namespace juce
