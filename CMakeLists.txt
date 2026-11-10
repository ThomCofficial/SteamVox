cmake_minimum_required(VERSION 3.22)
project(SteamVox VERSION 0.3.0 LANGUAGES CXX)

add_library(SteamVoxDSP INTERFACE)
target_include_directories(SteamVoxDSP INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_features(SteamVoxDSP INTERFACE cxx_std_17)

include(CTest)
if(BUILD_TESTING)
  add_executable(SteamVoxStress tests/stress.cpp)
  target_link_libraries(SteamVoxStress PRIVATE SteamVoxDSP)
  add_test(NAME SteamVoxStress COMMAND SteamVoxStress)
endif()

option(STEAMVOX_BUILD_PLUGIN "Build the JUCE VST3 plugin" OFF)
if(STEAMVOX_BUILD_PLUGIN)
  include(FetchContent)
  FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
     GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)

  juce_add_plugin(SteamVox
    COMPANY_NAME ThomC
    BUNDLE_ID com.thomc.steamvox
    PLUGIN_MANUFACTURER_CODE ThmC
    PLUGIN_CODE StVx
    FORMATS VST3
    PRODUCT_NAME "SteamVox"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  target_sources(SteamVox PRIVATE
    plugin/PluginProcessor.cpp
    plugin/PluginEditor.cpp)
  target_include_directories(SteamVox PRIVATE src plugin)
  target_compile_features(SteamVox PRIVATE cxx_std_17)
  target_compile_definitions(SteamVox PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(SteamVox PRIVATE SteamVoxDSP
    juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra
    juce::juce_dsp juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
endif()
