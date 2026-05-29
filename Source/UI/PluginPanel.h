#pragma once

#include <juce_gui_extra/juce_gui_extra.h>
#include "AudioEngine.h"
#include "PluginHostManager.h"

class PluginPanel final : public juce::Component,
                          private juce::Timer
{
public:
    PluginPanel(AudioEngine& engineToUse, PluginHostManager& pluginHostToUse);

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::String getStatusMessage() const { return statusMessage; }

private:
    void timerCallback() override;
    void loadPluginClicked();
    void openEditorClicked();
    void unloadPluginClicked();
    void refreshState();

    AudioEngine& audioEngine;
    PluginHostManager& pluginHost;

    juce::Label titleLabel;
    juce::Label pluginNameLabel;
    juce::Label statusLabel;
    juce::TextButton loadButton;
    juce::TextButton editorButton;
    juce::TextButton unloadButton;

    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String statusMessage { "Sin plugin cargado. Bypass activo." };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginPanel)
};
