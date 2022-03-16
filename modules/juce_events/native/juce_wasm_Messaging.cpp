/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#include <emscripten.h>

#include <deque>
#include <mutex>

namespace juce
{

static void createDirIfNotExists(File::SpecialLocationType type)
{
    File dir = File::getSpecialLocation(type);
    if (!dir.exists()) dir.createDirectory();
}

static bool appIsInsideEmrun{ false };

static std::deque<MessageManager::MessageBase*> messageQueue;
static std::deque<MessageManager::MessageBase*> eventQueue;
static std::mutex queueMtx;
static std::atomic<bool> quitReceived{ false };
static double timeDispatchBeginMS{ 0 };

static Thread::ThreadID messageThreadID{ nullptr }; // JUCE message thread
static Thread::ThreadID mainThreadID{ nullptr };    // Javascript main thread

std::unique_ptr<juce::ScopedJuceInitialiser_GUI> libraryInitialiser;
std::vector<std::function<void()>> preDispatchLoopFuncs;
// These callbacks are only executed if main thread isn't message thread.
std::vector<std::function<void()>> mainThreadLoopFuncs;

extern bool isMessageThreadProxied()
{
    return messageThreadID != mainThreadID;
}

extern void registerCallbackToMainThread(std::function<void()> f)
{
    if (mainThreadID == messageThreadID)
        preDispatchLoopFuncs.push_back(f);
    else
        mainThreadLoopFuncs.push_back(f);
}

extern std::deque<std::string> debugPrintQueue;
extern std::mutex debugPrintQueueMtx;

void MessageManager::doPlatformSpecificInitialisation()
{
    createDirIfNotExists(File::userHomeDirectory);
    createDirIfNotExists(File::userDocumentsDirectory);
    createDirIfNotExists(File::userMusicDirectory);
    createDirIfNotExists(File::userMoviesDirectory);
    createDirIfNotExists(File::userPicturesDirectory);
    createDirIfNotExists(File::userDesktopDirectory);
    createDirIfNotExists(File::userApplicationDataDirectory);
    createDirIfNotExists(File::commonDocumentsDirectory);
    createDirIfNotExists(File::commonApplicationDataDirectory);
    createDirIfNotExists(File::globalApplicationsDirectory);
    createDirIfNotExists(File::tempDirectory);

    messageThreadID = Thread::getCurrentThreadId();

    appIsInsideEmrun = MAIN_THREAD_EM_ASM_INT({
        return document.title == "Emscripten-Generated Code";
    });

    MAIN_THREAD_EM_ASM({
        if (window.juce_animationFrameCallback) return;

        window.juce_animationFrameCallback = Module.cwrap(
            'juce_animationFrameCallback', 'int',['number']);

        if (window.juce_animationFrameCallback(-1.0) == 1)
        {
            window.juce_animationFrameWrapper = function(timestamp)
            {
                window.juce_animationFrameCallback(timestamp);
                window.requestAnimationFrame(window.juce_animationFrameWrapper);
            };

            window.requestAnimationFrame(window.juce_animationFrameWrapper);
        }
    });
}

void MessageManager::doPlatformSpecificShutdown() {}

// If timestamp < 0, this callback tests if the calling thread (main thread) is
//   different from the message thread and return the result.
// If timestamp >= 0, it always returns 0.
extern "C" int juce_animationFrameCallback(double timestamp)
{
    if (timestamp < 0)
    {
        mainThreadID = Thread::getCurrentThreadId();
        return mainThreadID != messageThreadID;
    }

    static double prevTimestamp = 0;
    if (timestamp - prevTimestamp > 20)
        DBG("juce_animationFrameCallback " << timestamp - prevTimestamp);
    prevTimestamp = timestamp;

    for (auto f : mainThreadLoopFuncs) f();

    return 0;
}

double getTimeSpentInCurrentDispatchCycle()
{
    double currentTimeMS = Time::getMillisecondCounterHiRes();
    // DBG("getTimeSpentInCurrentDispatchCycle: " << currentTimeMS - timeDispatchBeginMS);
    return (currentTimeMS - timeDispatchBeginMS) / 1000.0;
}

static void dispatchEvents()
{
    queueMtx.lock();
    std::deque<MessageManager::MessageBase*> eventCopy = eventQueue;
    eventQueue.clear();
    queueMtx.unlock();

    while (!eventCopy.empty())
    {
        MessageManager::MessageBase* message = eventCopy.front();
        eventCopy.pop_front();
        message->messageCallback();
        message->decReferenceCount();
    }
}

static void dispatchLoop()
{
    if (quitReceived.load())
    {
        emscripten_cancel_main_loop();
        auto* app = JUCEApplicationBase::getInstance();
        app->shutdownApp();
        libraryInitialiser.reset(nullptr);
        return;
    }

    // DBG("new dispatch loop cycle");
    timeDispatchBeginMS = Time::getMillisecondCounterHiRes();

    dispatchEvents();

    for (auto f : preDispatchLoopFuncs) f();

#if JUCE_DEBUG
    debugPrintQueueMtx.lock();
    while (!debugPrintQueue.empty())
    {
        std::cout << debugPrintQueue.front() << std::endl;
        debugPrintQueue.pop_front();
    }
    debugPrintQueueMtx.unlock();
#endif

    queueMtx.lock();
    std::deque<MessageManager::MessageBase*> messageCopy = messageQueue;
    messageQueue.clear();
    queueMtx.unlock();

    while (!messageCopy.empty())
    {
        MessageManager::MessageBase* message = messageCopy.front();
        messageCopy.pop_front();
        message->messageCallback();
        message->decReferenceCount();
    }

    if (appIsInsideEmrun)
    {
        MAIN_THREAD_EM_ASM({
            var logArea = document.querySelector("#output");
            var n = logArea.value.length;
            if (n > 1000)
                logArea.value = logArea.value.substring(n - 1000, n);
        });
    }
    // DBG("ending dispatch loop cycle");
}

bool MessageManager::postMessageToSystemQueue(MessageManager::MessageBase* const message)
{
    queueMtx.lock();
    if (dynamic_cast<EmscriptenEventMessage* const>(message))
        eventQueue.push_back(message);
    else
        messageQueue.push_back(message);
    message->incReferenceCount();
    queueMtx.unlock();
    return true;
}

void MessageManager::broadcastMessage(const String&)
{
}

void MessageManager::runDispatchLoop()
{
    emscripten_set_main_loop(dispatchLoop, 0, 0);
}

struct QuitCallback : public CallbackMessage
{
    QuitCallback() {}
    void messageCallback() override
    {
        quitReceived = true;
    }
};

void MessageManager::stopDispatchLoop()
{
    (new QuitCallback())->post();
    quitMessagePosted = true;
}

}
