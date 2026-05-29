#pragma once

#include <juce_core/juce_core.h>

class SettingsManager
{
public:
    SettingsManager();

    juce::var load() const;
    bool save(const juce::var& root) const;

    juce::File getSettingsFile() const noexcept { return settingsFile; }
    juce::File getConfigDirectory() const noexcept { return configDirectory; }

    juce::String getLastPluginPath(const juce::var& settings) const;

private:
    juce::File configDirectory;
    juce::File settingsFile;
    juce::File defaultSettingsFile;

    static juce::var createFallbackSettings();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsManager)
};
