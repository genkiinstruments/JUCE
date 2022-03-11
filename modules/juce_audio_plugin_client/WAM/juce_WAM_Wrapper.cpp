#include "AppConfig.h"
#include "../../../JuceHeader.h"
#include "../utility/juce_CheckSettingMacros.h"

#if JucePlugin_Build_WAM
#include "wamsdk/processor.h"

using namespace juce;
extern AudioProcessor* JUCE_API JUCE_CALLTYPE createPluginFilterOfType (AudioProcessor::WrapperType);

//==============================================================================

class JuceWAMWrapper : public WAM::Processor
{
public:
    JuceWAMWrapper (AudioProcessor* proc)
    : m_proc(proc)
    {
    }

    ~JuceWAMWrapper ()
    {
    }
    
    const char* init(uint32_t bufsize, uint32_t sr, void* desc)
    {
        m_inChannels  = 0; // m_proc->getNumInputChannels();
        m_outChannels = 2; // m_proc->getNumOutputChannels();
        m_proc->setPlayConfigDetails(m_inChannels, m_outChannels, sr, 128);
        
        m_audiobufs = new AudioSampleBuffer(m_inChannels + m_outChannels, 128);
        m_midibuf = new MidiBuffer();
        m_proc->prepareToPlay(sr, 128);

				// TODO: descriptor
        return nullptr;
    }

		void terminate()
		{
			m_proc->releaseResources();
			delete m_audiobufs;
			delete m_midibuf;
		}
	
		void onMidi(WAM::byte status, WAM::byte data1, WAM::byte data2)
		{
			if ((status & 0xF0) == 0xC0)
				m_proc->setCurrentProgram(data1);
			else if (status != 0xF0)
			{
				unsigned char msg[3] = { status, data1, data2 };
				m_midibuf->addEvent(msg, 3, 0);
			}
		}

		void onSysex(WAM::byte* msg, uint32_t size)
		{
		}
    
		void onPatch(void* patch, uint32_t size)
		{
			m_proc->setCurrentProgramStateInformation(patch, size);
		}

		void onParam(uint32_t idparam, double value)
		{
			m_proc->setParameter((int)idparam, (float)value);
		}

    void onProcess(WAM::AudioBus* audio, void* data)
    {
    }

private:
    AudioProcessor* m_proc;
		AudioSampleBuffer* m_audiobufs;
		MidiBuffer* m_midibuf;
    int m_inChannels;
    int m_outChannels;
};

extern "C" {
EMSCRIPTEN_KEEPALIVE void* createModule()
{
    JuceWAMWrapper* wrapper = nullptr;
    AudioProcessor* proc = createPluginFilterOfType (AudioProcessor::wrapperType_WAM);
    if (proc)
        wrapper = new JuceWAMWrapper (proc);    
    return wrapper;
}}

#endif
