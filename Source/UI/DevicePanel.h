#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "AudioEngine.h"

class DevicePanel final : public juce::Component,
                          private juce::ComboBox::Listener,
                          private juce::Timer
{
public:
    explicit DevicePanel(AudioEngine& engineToUse);
    ~DevicePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void timerCallback() override;
    void refreshLabels();

    AudioEngine& audioEngine;

    juce::Label titleLabel;
    juce::Label helperLabel;
    juce::Label bufferLabel;
    juce::Label sampleRateLabel;
    juce::Label latencyLabel;

    juce::ComboBox bufferCombo;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> selector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DevicePanel)
};
