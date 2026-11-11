cmake_minimum_required(VERSION 3.22)
project(PandaRage VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

if(DEFINED ENV{CI})
    set(PANDA_COPY FALSE)
else()
    set(PANDA_COPY TRUE)
endif()

juce_add_plugin(PandaRage
    COMPANY_NAME "PandaLabs"
    PLUGIN_MANUFACTURER_CODE Pndl
    PLUGIN_CODE Prge
    FORMATS VST3 Standalone
    PRODUCT_NAME "Panda Rage"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${PANDA_COPY}
)

target_sources(PandaRage PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.cpp
    PluginEditor.h
)

target_compile_definitions(PandaRage PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(PandaRage
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
