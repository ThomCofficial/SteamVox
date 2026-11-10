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
  juce_generate_juce_header(SteamVox)
  target_sources(SteamVox PRIVATE
    plugin/PluginProcessor.cpp
    plugin/PluginEditor.cpp)
  target_include_directories(SteamVox PRIVATE src plugin)
  # Normalize the inadvertently doubled file extension on the uploaded needle.
  # Copying into the build tree leaves the user's actual artwork untouched.
  set(STEAMVOX_NEEDLE "${CMAKE_CURRENT_SOURCE_DIR}/assets/steamvox_vu_needle.png")
  if(NOT EXISTS "${STEAMVOX_NEEDLE}" AND
     EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/steamvox_vu_needle.png.png")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/normalized-assets")
    configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/assets/steamvox_vu_needle.png.png"
      "${CMAKE_CURRENT_BINARY_DIR}/normalized-assets/steamvox_vu_needle.png" COPYONLY)
    set(STEAMVOX_NEEDLE "${CMAKE_CURRENT_BINARY_DIR}/normalized-assets/steamvox_vu_needle.png")
  endif()
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/steamvox_faceplate.png"
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/steamvox_large_knob.png"
    AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/steamvox_small_knob.png"
    AND EXISTS "${STEAMVOX_NEEDLE}")
    juce_add_binary_data(SteamVoxArtwork SOURCES
      assets/steamvox_faceplate.png
      assets/steamvox_large_knob.png
      assets/steamvox_small_knob.png
      "${STEAMVOX_NEEDLE}")
    target_link_libraries(SteamVox PRIVATE SteamVoxArtwork)
    target_compile_definitions(SteamVox PRIVATE STEAMVOX_HAS_ARTWORK=1)
  endif()
  target_compile_features(SteamVox PRIVATE cxx_std_17)
  target_compile_definitions(SteamVox PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(SteamVox PRIVATE SteamVoxDSP
    juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra
    juce::juce_dsp juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
endif()
