#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "AudioEngine.h"
#include "PluginHostManager.h"
#include "SettingsManager.h"
#include "UI/DevicePanel.h"
#include "UI/PluginPanel.h"
#include "UI/LevelMeter.h"

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateStatusText();
    void saveSettings();
    void restoreLastPluginAsync();
    void configureButton(juce::TextButton& button, juce::Colour colour, juce::Colour textColour = juce::Colours::white);

    SettingsManager settingsManager;
    juce::var loadedSettings;

    PluginHostManager pluginHost;
    AudioEngine audioEngine;

    std::unique_ptr<DevicePanel> devicePanel;
    std::unique_ptr<PluginPanel> pluginPanel;

    LevelMeter inputMeter;
    LevelMeter outputMeter;

    juce::Label appTitle;
    juce::Label subtitle;
    juce::Label statusTitle;
    juce::Label statusDetails;

    juce::TextButton audioToggleButton;
    juce::TextButton muteButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
