#include <unistd.h>
#include <signal.h>

namespace juce {
	
int64  Time::getHighResolutionTicksPerSecond() noexcept  { return 0; }
int64  Time::getHighResolutionTicks() noexcept           { return 0; }
double Time::getMillisecondCounterHiRes() noexcept       { return 0; }
uint32 juce_millisecondsSinceStartup() noexcept					 { return 0; }
	
Thread::ThreadID JUCE_CALLTYPE Thread::getCurrentThreadId()	{ return 0; }

// DummyCriticalSection
CriticalSection::CriticalSection() noexcept			{}
CriticalSection::~CriticalSection() noexcept  	{}
void CriticalSection::enter() const noexcept    {}
bool CriticalSection::tryEnter() const noexcept { return true; }
void CriticalSection::exit() const noexcept			{}
void SpinLock::enter() const noexcept						{}

}
