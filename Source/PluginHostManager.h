#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <atomic>
#include <mutex>

class PluginHostManager
{
public:
    PluginHostManager();
    ~PluginHostManager();

    bool loadPluginFromFile(const juce::File& pluginFile, double sampleRate, int blockSize);
    bool restorePluginFromPath(const juce::String& pluginPath, double sampleRate, int blockSize);

    void unloadPlugin();
    void prepare(double sampleRate, int blockSize);
    void releaseResources();

    // Called from the audio thread. It never loads files and never blocks waiting for a long operation.
    // If the plugin is being swapped, this returns true and silences the current buffer for safety.
    bool processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    bool hasPlugin() const noexcept;
    bool hasEditor() const;
    bool showEditor();
    void closeEditor();

    juce::String getPluginName() const;
    juce::String getPluginPath() const;
    juce::String getLastError() const;
    int getPluginLatencySamples() const noexcept;

private:
    class PluginEditorWindow;

    bool findPluginDescriptionForFile(const juce::File& pluginFile, juce::PluginDescription& description, juce::String& errorMessage);
    void setError(const juce::String& message);

    juce::AudioPluginFormatManager formatManager;

    mutable std::mutex pluginMutex;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    juce::PluginDescription currentDescription;

    std::unique_ptr<PluginEditorWindow> editorWindow;

    std::atomic<bool> pluginReady { false };
    std::atomic<int> pluginLatencySamples { 0 };

    juce::String pluginName { "Ningún plugin cargado" };
    juce::String pluginPath;
    juce::String lastError;

    double currentSampleRate { 44100.0 };
    int currentBlockSize { 512 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHostManager)
};
