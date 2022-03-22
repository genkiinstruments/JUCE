# ==============================================================================
#
#  This file is part of the JUCE library.
#  Copyright (c) 2020 - Raw Material Software Limited
#
#  JUCE is an open source library subject to commercial or open-source
#  licensing.
#
#  By using JUCE, you agree to the terms of both the JUCE 6 End-User License
#  Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).
#
#  End User License Agreement: www.juce.com/juce-6-licence
#  Privacy Policy: www.juce.com/juce-privacy-policy
#
#  Or: You may also use this code under the terms of the GPL v3 (see
#  www.gnu.org/licenses).
#
#  JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
#  EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
#  DISCLAIMED.
#
# ==============================================================================

set(CMAKE_FOLDER examples)

add_subdirectory(CMake)
add_subdirectory(DemoRunner)

function(_juce_add_pips)
    file(GLOB headers
        CONFIGURE_DEPENDS LIST_DIRECTORIES false
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

    if((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_SYSTEM_NAME MATCHES ".*BSD"))
        list(REMOVE_ITEM headers
            "${CMAKE_CURRENT_SOURCE_DIR}/CameraDemo.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/PushNotificationsDemo.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/VideoDemo.h")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        list(REMOVE_ITEM headers
            "${CMAKE_CURRENT_SOURCE_DIR}/PushNotificationsDemo.h")
    endif()

    foreach(header IN ITEMS ${headers})
        juce_add_pip(${header} added_target)
        target_link_libraries(${added_target} PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_lto_flags
            juce::juce_recommended_warning_flags)
    endforeach()

    if (EMSCRIPTEN)
        set(EMSCRIPTEN_EXECUTABLE_LINK_OPTIONS
                "--emrun"
                "--enable-features=SharedArrayBuffer"
                "-sEXPORTED_FUNCTIONS=['_main', '_juce_mouseCallback', '_juce_keyboardCallback', '_juce_animationFrameCallback', '_emscripten_sync_run_in_main_runtime_thread_']"
                "-sEXPORTED_RUNTIME_METHODS=['cwrap', 'ccall', 'printErr']"
                "-sINITIAL_MEMORY=128MB"
                "-sSAFE_HEAP=1"
                "--profiling-funcs"
                "--profiling"
                "-pthread"
                )

        target_compile_options(${added_target} PUBLIC ${EMSCRIPTEN_COMPILE_OPTIONS})
        target_link_options(${added_target} PUBLIC
                ${EMSCRIPTEN_LINK_OPTIONS}
                ${EMSCRIPTEN_EXECUTABLE_LINK_OPTIONS}

                "SHELL:--embed-file ${JUCE_SOURCE_DIR}/wasm/fonts@/usr/X11R6/lib/X11/fonts"
                "SHELL:--embed-file ${JUCE_SOURCE_DIR}/examples/Assets@/examples/Assets"
                "SHELL:--embed-file ${JUCE_SOURCE_DIR}/examples/GUI@/GUI"
                "SHELL:--embed-file ${JUCE_SOURCE_DIR}/examples/Audio@/Audio"
                "SHELL:--embed-file ${JUCE_SOURCE_DIR}/examples/DSP@/DSP"
                "SHELL:--embed-file ${JUCE_SOURCE_DIR}/examples/GUI@/GUI"
                "SHELL:--embed-file ${JUCE_SOURCE_DIR}/examples/Utilities@/Utilities"
        )
    endif ()
endfunction()

add_subdirectory(Audio)
add_subdirectory(DSP)
add_subdirectory(GUI)
add_subdirectory(Plugins)
add_subdirectory(Utilities)
