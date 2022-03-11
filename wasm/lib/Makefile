# Web Audio Modules (WAMs)
# jari kleimola 2017-2018 (jari@webaudiomodules.org)
# wasm makefile for JUCE AudioWorklet lib

TARGET = juce-audioworklet.bc

SOURCES = juce_lib.cpp
INC = -I../../modules

Wno = -Wno-inconsistent-missing-override -Wno-deprecated-register -Wno-unused-private-field
JUCEFLAGS = -DJUCE_STANDALONE_APPLICATION=0 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
CFLAGS  = -DWASM -DAUDIOWORKLET -std=c++11 -Wall $(Wno) $(INC) $(JUCEFLAGS)
LDFLAGS = -O2
EMFLAGS = -s ALLOW_MEMORY_GROWTH=1 -s WASM=1

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(EMFLAGS) -o $@ $(SOURCES)
