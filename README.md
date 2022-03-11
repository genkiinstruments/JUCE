![alt text](https://assets.juce.com/juce/JUCE_banner_github.png "JUCE")

JUCE is an open-source cross-platform C++ application framework for creating high quality
desktop and mobile applications, including VST, VST3, AU, AUv3, RTAS and AAX audio plug-ins.
JUCE can be easily integrated with existing projects via CMake, or can be used as a project
generation tool via the [Projucer](https://juce.com/discover/projucer), which supports
exporting projects for Xcode (macOS and iOS), Visual Studio, Android Studio, Code::Blocks
and Linux Makefiles as well as containing a source code editor.

## Getting Started

The JUCE repository contains a [master](https://github.com/juce-framework/JUCE/tree/master)
and [develop](https://github.com/juce-framework/JUCE/tree/develop) branch. The develop branch
contains the latest bugfixes and features and is periodically merged into the master
branch in stable [tagged releases](https://github.com/juce-framework/JUCE/releases)
(the latest release containing pre-built binaries can be also downloaded from the
[JUCE website](https://juce.com/get-juce)).

JUCE projects can be managed with either the Projucer (JUCE's own project-configuration
tool) or with CMake.

### The Projucer

The repository doesn't contain a pre-built Projucer so you will need to build it
for your platform - Xcode, Visual Studio and Linux Makefile projects are located in
[extras/Projucer/Builds](/extras/Projucer/Builds)
(the minimum system requirements are listed in the __System Requirements__ section below).
The Projucer can then be used to create new JUCE projects, view tutorials and run examples.
It is also possible to include the JUCE modules source code in an existing project directly,
or build them into a static or dynamic library which can be linked into a project.

For further help getting started, please refer to the JUCE
[documentation](https://juce.com/learn/documentation) and
[tutorials](https://juce.com/learn/tutorials).

### CMake

Version 3.15 or higher is required. To use CMake, you will need to install it,
either from your system package manager or from the [official download
page](https://cmake.org/download/). For comprehensive documentation on JUCE's
CMake API, see the [JUCE CMake documentation](/docs/CMake%20API.md). For
examples which may be useful as starting points for new CMake projects, see the
[CMake examples directory](/examples/CMake).

#### Building Examples

To use CMake to build the examples and extras bundled with JUCE, simply clone
JUCE and then run the following commands, replacing "DemoRunner" with the name
of the target you wish to build.

    cd /path/to/JUCE
    cmake . -B cmake-build -DJUCE_BUILD_EXAMPLES=ON -DJUCE_BUILD_EXTRAS=ON
    cmake --build cmake-build --target DemoRunner

## Minimum System Requirements

#### Building JUCE Projects

- __macOS/iOS__: Xcode 9.2 (macOS 10.12.6)
- __Windows__: Windows 8.1 and Visual Studio 2015 Update 3 64-bit
- __Linux__: g++ 5.0 or Clang 3.4 (for a full list of dependencies, see
[here](/docs/Linux%20Dependencies.md)).
- __Android__: Android Studio on Windows, macOS or Linux

#### Deployment Targets

- __macOS__: macOS 10.7
- __Windows__: Windows Vista
- __Linux__: Mainstream Linux distributions
- __iOS__: iOS 9.0
- __Android__: Jelly Bean (API 16)

## Contributing

Please see our [contribution guidelines](.github/contributing.md).

## License

The core JUCE modules (juce_audio_basics, juce_audio_devices, juce_core and juce_events)
are permissively licensed under the terms of the
[ISC license](http://www.isc.org/downloads/software-support-policy/isc-license/).
Other modules are covered by a
[GPL/Commercial license](https://www.gnu.org/licenses/gpl-3.0.en.html).

There are multiple commercial licensing tiers for JUCE, with different terms for each:
- JUCE Personal (developers or startup businesses with revenue under 50K USD) - free
- JUCE Indie (small businesses with revenue under 500K USD) - $40/month
- JUCE Pro (no revenue limit) - $130/month
- JUCE Educational (no revenue limit) - free for bona fide educational institutes

For full terms see [LICENSE.md](LICENSE.md).

# JUCE 5 (WebAssembly/AudioWorklet version)

This repo ports the DSP subsection of JUCE library to WebAssembly (WASM). The port enables JUCE plugins to run in web browsers as Web Audio API AudioWorklets. Precompiled static WASM bitcode library (juce-audioworklet.bc) and its build configuration (Makefile and juce\_lib.cpp) are [here](https://github.com/jariseon/JUCE/tree/master/wasm/lib)

The port is bundled into two commits. The [first one](https://github.com/jariseon/JUCE/commit/e4b5ed006e3a3bf0a2518d48a8fae164be883e5d) contains modifications to existing JUCE files (mostly just #ifdefs), while the [second one](https://github.com/jariseon/JUCE/commit/5156a5d30fa4aed0eb77fd2a08679282c49920d6) contains build configuration, Web Audio Module (WAM) wrapper code and some dummy overrides for functionality that is unsupported in WASM and AudioWorklet environments (threads and high resolution timer). Porting was surprisingly straight forward.

## usage

please see this [example](https://github.com/jariseon/webOBXD)

## license

This WASM version of JUCE uses juce\_audio\_basics, juce\_core and juce\_events modules, which are permissively licensed under [ISC](http://www.isc.org/downloads/software-support-policy/isc-license) license. It also uses limited functionality from juce\_audio\_processors and juce\_audio\_plugin\_client modules, which are under JUCE license. Please see the original JUCE [repo] (https://github.com/WeAreROLI/JUCE) for more licensing info.
