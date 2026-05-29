cmake_minimum_required(VERSION 3.24)

project(MusiVST_GuitarHost
    VERSION 0.1.0
    DESCRIPTION "MusiVST Guitar Host - portable JUCE VST3 guitar host"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# JUCE is fetched automatically so the project can be built cleanly in GitHub Actions.
# You may override this from the command line:
# cmake -S . -B build -DMUSIVST_JUCE_TAG=8.0.13
set(MUSIVST_JUCE_TAG "8.0.13" CACHE STRING "JUCE git tag to use")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${MUSIVST_JUCE_TAG}
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(MusiVSTGuitarHost
    PRODUCT_NAME "MusiVST Guitar Host"
    COMPANY_NAME "Musicala"
    COMPANY_WEBSITE "https://www.imusicala.com"
    BUNDLE_ID "com.musicala.musivstguitarhost"
    VERSION "${PROJECT_VERSION}"
)

target_sources(MusiVSTGuitarHost
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/AudioEngine.cpp
        Source/PluginHostManager.cpp
        Source/SettingsManager.cpp
        Source/UI/LevelMeter.cpp
        Source/UI/DevicePanel.cpp
        Source/UI/PluginPanel.cpp
)

target_include_directories(MusiVSTGuitarHost
    PRIVATE
        Source
        Source/UI
)

target_compile_definitions(MusiVSTGuitarHost
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_PLUGINHOST_VST3=1
        JUCE_PLUGINHOST_VST=0
        JUCE_PLUGINHOST_AU=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_PLUGINHOST_LV2=0
)


if (MSVC)
    target_compile_options(MusiVSTGuitarHost PRIVATE /utf-8)
endif()

target_link_libraries(MusiVSTGuitarHost
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Keep output paths predictable for manual local packaging.
set_target_properties(MusiVSTGuitarHost PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
