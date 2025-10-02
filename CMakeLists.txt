cmake_minimum_required(VERSION 3.15)
project(Plucks VERSION 0.9.18)

add_definitions(-DJUCE_WEB_BROWSER=0)

add_subdirectory(JUCE) # JUCE source in ./JUCE

# --- Platform-specific dependencies and workarounds ---
if(UNIX AND NOT APPLE)
    # Linux-specific: require GTK3 for JUCE GUI
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    include_directories(${GTK3_INCLUDE_DIRS})
    link_directories(${GTK3_LIBRARY_DIRS})
    add_definitions(${GTK3_CFLAGS_OTHER})

    # Linux-specific: require libcurl for networking
    pkg_check_modules(CURL REQUIRED libcurl)
    include_directories(${CURL_INCLUDE_DIRS})
    link_directories(${CURL_LIBRARY_DIRS})
    add_definitions(${CURL_CFLAGS_OTHER})
    set(PLATFORM_LIBS ${GTK3_LIBRARIES} ${CURL_LIBRARIES})

elseif(APPLE)
    # macOS-specific: Optionally link curl if found
    find_package(CURL)
    if(CURL_FOUND)
        message(STATUS "libcurl found on macOS, will link.")
        set(PLATFORM_LIBS CURL::libcurl)
    else()
        message(STATUS "libcurl not found on macOS, will not link.")
        set(PLATFORM_LIBS "")
    endif()

elseif(WIN32)
    # Windows-specific: No extra dependencies required
    message(STATUS "Configuring for Windows (no extra dependencies)")
    set(PLATFORM_LIBS "")
endif()
# ------------------------------------------------------

# --- Embed your PNG as binary data ---
juce_add_binary_data(BinaryData SOURCES
    BinaryData/Background.png
    BinaryData/Background2.png
    BinaryData/Button.png
    BinaryData/Knob.png
    BinaryData/Switch.png
    BinaryData/FaderKnob.png
    BinaryData/FaderTrack.png
    BinaryData/comic.ttf
)

juce_add_plugin(Plucks
    COMPANY_NAME "AmateurTools"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER FALSE
    IS_MIDI_EFFECT FALSE
    PLUGIN_MANUFACTURER_CODE Amat # 4-char, at least one uppercase
    PLUGIN_CODE PlxU              # 4-char, at least one uppercase
    FORMATS VST3
    PRODUCT_NAME "Plucks"
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(Plucks PRIVATE
    Source/PluckSound.h
    Source/PluckVoice.h
    Source/PluginEditor.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.h
    Source/TuningSystem.h
)

target_compile_definitions(Plucks PUBLIC
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_generate_juce_header(Plucks)

target_link_libraries(Plucks PRIVATE
    BinaryData
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    ${PLATFORM_LIBS}
)