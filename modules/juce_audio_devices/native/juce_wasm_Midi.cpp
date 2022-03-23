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

#if ! defined(JUCE_WEBMIDI)
#define JUCE_WEBMIDI 1
#endif

#if JUCE_WEBMIDI
#define __WEB_MIDI_API__ 1
#endif

#include "emscripten/RtMidi.cpp"

#undef __WEB_MIDI_API__

namespace juce
{

class MidiInput::Pimpl
{
public:
    Pimpl(MidiInput* midi_in) : midiIn{ midi_in } {}

    std::unique_ptr<RtMidiIn> rtmidi = std::make_unique<RtMidiIn>();
    MidiInput* midiIn;
    MidiInputCallback* callback{ nullptr };
};

//==============================================================================

void rtmidiCallback(double timeStamp, std::vector<unsigned char>* message, void* userData)
{
    auto ctx = static_cast<MidiInput::Pimpl*>(userData);
    auto callback = ctx->callback;
    auto midiIn = ctx->midiIn;
    const void* data = message->data();
    int numBytes = static_cast<int>(message->size());
    // JUCE does not accept zero timestamp value, but RtMidi is supposed to send 0 for the first
    // message. To resolve that conflict, we offset 0.0 to slightly positive time.
    MidiMessage midiMessage{ data, numBytes, timeStamp > 0.0 ? timeStamp : 0.00000001 };

    callback->handleIncomingMidiMessage(midiIn, midiMessage);
}

std::unique_ptr<MidiInput> MidiInput::createNewDevice(const String& deviceName, MidiInputCallback* callback)
{
    RtMidiIn rtmidi{};

    rtmidi.openPort();
    jassert(rtmidi.isPortOpen());

    auto midiInput = std::unique_ptr<MidiInput>(new MidiInput(deviceName, rtmidi.getPortName()));
    midiInput->internal->callback = callback;
    rtmidi.setCallback(rtmidiCallback, midiInput->internal.get());

    return midiInput;
}

MidiInput::MidiInput (const String& deviceName, const String& deviceID)
    : deviceInfo (deviceName, deviceID) {
    internal = std::make_unique<Pimpl>(this);
}

MidiInput::~MidiInput() = default;

void MidiInput::start() { }

void MidiInput::stop() { }

Array<MidiDeviceInfo> MidiInput::getAvailableDevices() {
    Array<MidiDeviceInfo> ret{};
    RtMidiIn rtmidi{};

    for (unsigned int i = 0; i < rtmidi.getPortCount(); i++)
        ret.add(MidiDeviceInfo(rtmidi.getPortName(i), String::formatted("MidiIn_%d", i)));

    return ret;
}

MidiDeviceInfo MidiInput::getDefaultDevice() {
    return getAvailableDevices()[0];
}

std::unique_ptr<MidiInput> MidiInput::openDevice (const String& deviceIdentifier, MidiInputCallback* callback) {
    RtMidiIn rtmidiStatic{};

    std::unique_ptr<MidiInput> ret{nullptr};
    for (unsigned int i = 0; i < rtmidiStatic.getPortCount(); i++)
        if (String::formatted("MidiIn_%d", i) == deviceIdentifier) {
            ret = std::unique_ptr<MidiInput>(new MidiInput(rtmidiStatic.getPortName(i), deviceIdentifier));
            ret->internal->callback = callback;
            auto& rtmidi = *ret->internal->rtmidi;
            rtmidi.setCallback(rtmidiCallback, ret->internal->callback);
            rtmidi.openPort(i);
            return ret;
        }
    jassertfalse;
    return nullptr;
}

StringArray MidiInput::getDevices() {
    StringArray ret{};
    for (auto dev : getAvailableDevices())
        ret.add(dev.name);
    return {};
}

int MidiInput::getDefaultDeviceIndex() { return 0; }

std::unique_ptr<MidiInput> MidiInput::openDevice (int index, MidiInputCallback* callback) {
    return openDevice(getAvailableDevices()[index].identifier, callback);
}

//==============================================================================

class MidiOutput::Pimpl
{
public:
    std::unique_ptr<RtMidiOut> rtmidi = std::make_unique<RtMidiOut>();
};

std::unique_ptr<MidiOutput> MidiOutput::createNewDevice(const String& deviceName)
{
    RtMidiOut rtmidi{};

    rtmidi.openPort();
    jassert(rtmidi.isPortOpen());

    auto midiOutput = std::unique_ptr<MidiOutput>(new MidiOutput(deviceName, rtmidi.getPortName()));
    midiOutput->internal = std::make_unique<Pimpl>();
    return midiOutput;
}

MidiOutput::~MidiOutput() = default;

void MidiOutput::sendMessageNow (const MidiMessage& message) {
    internal->rtmidi->sendMessage(message.getRawData(), static_cast<size_t>(message.getRawDataSize()));
}

Array<MidiDeviceInfo> MidiOutput::getAvailableDevices() {
    Array<MidiDeviceInfo> ret{};
    RtMidiOut rtmidi{};

    for (unsigned int i = 0; i < rtmidi.getPortCount(); i++)
        ret.add(MidiDeviceInfo(rtmidi.getPortName(i), String::formatted("MidiOut_%d", i)));

    return ret;
}

MidiDeviceInfo MidiOutput::getDefaultDevice() {
    return getAvailableDevices()[0];
}

std::unique_ptr<MidiOutput> MidiOutput::openDevice (const String& deviceIdentifier) {
    RtMidiOut rtmidi{};
    std::unique_ptr<MidiOutput> ret{nullptr};
    for (unsigned int i = 0; i < rtmidi.getPortCount(); i++) {
        if (String::formatted("MidiOut_%d", i) == deviceIdentifier) {
            ret = std::unique_ptr<MidiOutput>(new MidiOutput(rtmidi.getPortName(i), deviceIdentifier));
            auto& rtmidiOut = *ret->internal->rtmidi;
            rtmidiOut.openPort(i);
            return ret;
        }
    }
    jassertfalse;
    return nullptr;
}

StringArray MidiOutput::getDevices() {
    StringArray ret{};
    for (auto dev : getAvailableDevices())
        ret.add(dev.name);
    return {};
}
int MidiOutput::getDefaultDeviceIndex() { return 0; }

std::unique_ptr<MidiOutput> MidiOutput::openDevice (int index) {
    return openDevice(getAvailableDevices()[index].identifier);
}

} // namespace juce
