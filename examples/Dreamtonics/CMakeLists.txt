cmake_minimum_required(VERSION 3.15)

project(Dreamtonics_Example VERSION 0.0.1)

juce_add_gui_app(Dreamtonics_Example
    # VERSION ...                       # Set this if the app version is different to the project version
    # ICON_BIG ...                      # ICON_* arguments specify a path to an image file to use as an icon
    # ICON_SMALL ...
    # DOCUMENT_EXTENSIONS ...           # Specify file extensions that should be associated with this app
    # COMPANY_NAME ...                  # Specify the name of the app's author
    PRODUCT_NAME "Dreamtonics Example") # The name of the final executable, which can differ from the target name

juce_generate_juce_header(Dreamtonics_Example)

target_sources(Dreamtonics_Example
    PRIVATE
        Source/Main.cpp
        Source/UI/MainContentComponent.cpp)

target_include_directories(Dreamtonics_Example
    PUBLIC
        Source/UI)

target_compile_definitions(Dreamtonics_Example
    PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_gui_app` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_gui_app` call
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Dreamtonics_Example,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Dreamtonics_Example,JUCE_VERSION>")

target_link_libraries(Dreamtonics_Example
    PRIVATE
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
