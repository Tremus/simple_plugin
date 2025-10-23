cmake_minimum_required(VERSION 3.15)
project(test_plugin
    VERSION 0.0.1
    )

set(CMAKE_CXX_VERSION 20)

add_subdirectory(modules/JUCE)

juce_add_plugin(${PROJECT_NAME}
    PRODUCT_NAME ${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}

    COMPANY_NAME tremus
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Exac # 4 character code. Required by AU, used in .plists on macOS
    PLUGIN_CODE test              # 4 character code. Required by AU, used in .plists on macOS
    HARDENED_RUNTIME_ENABLED FALSE # a macos permissions thing. Can't remember if required by Logic Pro or iOS apps: TODO test later
    FORMATS Standalone AU VST3 # Options: Standalone Unity VST3 AU AUv3 AAX VST LV2
)

target_sources(${PROJECT_NAME}
    PRIVATE
        src/PluginEditor.cpp
        src/PluginProcessor.cpp
        )

target_include_directories(${PROJECT_NAME}
    PRIVATE
        modules/xhl/include
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        NEEDS_SIDECHAIN=0
        JUCE_MODAL_LOOPS_PERMITTED=0
)

# juce_add_binary_data(${PROJECT_NAME} SOURCES 
# )


target_link_libraries(${PROJECT_NAME}
    PRIVATE
        # Binary data must be linked here if created
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

juce_generate_juce_header(${PROJECT_NAME})

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(
        -Wno-unused-parameter
        )
endif()
