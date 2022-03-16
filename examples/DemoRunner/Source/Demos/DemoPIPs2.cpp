/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include <juce_analytics/juce_analytics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_box2d/juce_box2d.h>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_events/juce_events.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_opengl/juce_opengl.h>
#include <juce_osc/juce_osc.h>
#include <juce_product_unlocking/juce_product_unlocking.h>
#if ! JUCE_EMSCRIPTEN
 #include <juce_video/juce_video.h>
#endif

using namespace juce;

#include "../../../Assets/DemoUtilities.h"
#include "JUCEDemos.h"

#include "../../../Assets/AudioLiveScrollingDisplay.h"

//==============================================================================
#if JUCE_MAC || JUCE_WINDOWS
 #include "../../../GUI/AccessibilityDemo.h"
#endif
#include "../../../GUI/AnimationAppDemo.h"
#include "../../../GUI/AnimationDemo.h"
#include "../../../GUI/BouncingBallWavetableDemo.h"
#if JUCE_USE_CAMERA && ! (JUCE_LINUX || JUCE_BSD || JUCE_EMSCRIPTEN)
 #include "../../../GUI/CameraDemo.h"
#endif
#if ! JUCE_ANDROID
 #include "../../../GUI/CodeEditorDemo.h"
#endif
#include "../../../GUI/ComponentDemo.h"
#include "../../../GUI/ComponentTransformsDemo.h"
#include "../../../GUI/DialogsDemo.h"
#include "../../../GUI/FlexBoxDemo.h"
#include "../../../GUI/FontsDemo.h"
#include "../../../GUI/GraphicsDemo.h"
#include "../../../GUI/GridDemo.h"
#include "../../../GUI/ImagesDemo.h"
#include "../../../GUI/KeyMappingsDemo.h"
#include "../../../GUI/LookAndFeelDemo.h"
#include "../../../GUI/MDIDemo.h"
#include "../../../GUI/MenusDemo.h"
#include "../../../GUI/MultiTouchDemo.h"
#if JUCE_OPENGL
 #include "../../../GUI/OpenGLAppDemo.h"
 #include "../../../GUI/OpenGLDemo.h"
 #include "../../../GUI/OpenGLDemo2D.h"
#endif
#include "../../../GUI/PropertiesDemo.h"
#if ! (JUCE_LINUX || JUCE_BSD || JUCE_EMSCRIPTEN)
 #include "../../../GUI/VideoDemo.h"
#endif
#if ! JUCE_EMSCRIPTEN
 #include "../../../GUI/WebBrowserDemo.h"
#endif
#include "../../../GUI/WidgetsDemo.h"
#include "../../../GUI/WindowsDemo.h"

void registerDemos_Two() noexcept
{
   #if JUCE_MAC || JUCE_WINDOWS
    REGISTER_DEMO (AccessibilityDemo,         GUI, false)
   #endif
    REGISTER_DEMO (AnimationAppDemo,          GUI, false)
    REGISTER_DEMO (AnimationDemo,             GUI, false)
    REGISTER_DEMO (BouncingBallWavetableDemo, GUI, false)
   #if JUCE_USE_CAMERA && ! (JUCE_LINUX || JUCE_BSD || JUCE_EMSCRIPTEN)
    REGISTER_DEMO (CameraDemo,                GUI, true)
   #endif
   #if ! JUCE_ANDROID
    REGISTER_DEMO (CodeEditorDemo,            GUI, false)
   #endif
    REGISTER_DEMO (ComponentDemo,             GUI, false)
    REGISTER_DEMO (ComponentTransformsDemo,   GUI, false)
    REGISTER_DEMO (DialogsDemo,               GUI, false)
    REGISTER_DEMO (FlexBoxDemo,               GUI, false)
    REGISTER_DEMO (FontsDemo,                 GUI, false)
    REGISTER_DEMO (GraphicsDemo,              GUI, false)
    REGISTER_DEMO (GridDemo,                  GUI, false)
    REGISTER_DEMO (ImagesDemo,                GUI, false)
    REGISTER_DEMO (KeyMappingsDemo,           GUI, false)
    REGISTER_DEMO (LookAndFeelDemo,           GUI, false)
    REGISTER_DEMO (MDIDemo,                   GUI, false)
    REGISTER_DEMO (MenusDemo,                 GUI, false)
    REGISTER_DEMO (MultiTouchDemo,            GUI, false)
   #if JUCE_OPENGL
    REGISTER_DEMO (OpenGLAppDemo,             GUI, true)
    REGISTER_DEMO (OpenGLDemo2D,              GUI, true)
    REGISTER_DEMO (OpenGLDemo,                GUI, true)
   #endif
    REGISTER_DEMO (PropertiesDemo,            GUI, false)
   #if ! (JUCE_LINUX || JUCE_BSD || JUCE_EMSCRIPTEN)
    REGISTER_DEMO (VideoDemo,                 GUI, true)
   #endif
   #if ! JUCE_EMSCRIPTEN
    REGISTER_DEMO (WebBrowserDemo,            GUI, true)
   #endif
    REGISTER_DEMO (WidgetsDemo,               GUI, false)
    REGISTER_DEMO (WindowsDemo,               GUI, false)
}

CodeEditorComponent::ColourScheme getDarkColourScheme()
{
    return getDarkCodeEditorColourScheme();
}

CodeEditorComponent::ColourScheme getLightColourScheme()
{
    return getLightCodeEditorColourScheme();
}
