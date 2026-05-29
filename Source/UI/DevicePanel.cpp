#include "DevicePanel.h"

namespace
{
    void styleLabel(juce::Label& label, float size, bool bold = false)
    {
        label.setFont(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(62, 52, 82));
    }
}

DevicePanel::DevicePanel(AudioEngine& engineToUse)
    : audioEngine(engineToUse)
{
    titleLabel.setText("Dispositivo de audio", juce::dontSendNotification);
    styleLabel(titleLabel, 20.0f, true);
    addAndMakeVisible(titleLabel);

    helperLabel.setText("Escoge tu interfaz, Input 1 para guitarra y Output 1-2 para salida estéreo.", juce::dontSendNotification);
    helperLabel.setJustificationType(juce::Justification::centredLeft);
    helperLabel.setMinimumHorizontalScale(0.8f);
    styleLabel(helperLabel, 13.0f);
    addAndMakeVisible(helperLabel);

    selector = std::make_unique<juce::AudioDeviceSelectorComponent>(audioEngine.getDeviceManager(),
                                                                    1, 2,
                                                                    2, 2,
                                                                    false,
                                                                    false,
                                                                    true,
                                                                    false);
    selector->setName("Selector de dispositivo");
    addAndMakeVisible(*selector);

    bufferLabel.setText("Buffer", juce::dontSendNotification);
    styleLabel(bufferLabel, 14.0f, true);
    addAndMakeVisible(bufferLabel);

    bufferCombo.addItem("64 samples", 64);
    bufferCombo.addItem("128 samples", 128);
    bufferCombo.addItem("256 samples", 256);
    bufferCombo.addItem("512 samples", 512);
    bufferCombo.setSelectedId(audioEngine.getCurrentBufferSize(), juce::dontSendNotification);
    bufferCombo.addListener(this);
    addAndMakeVisible(bufferCombo);

    styleLabel(sampleRateLabel, 13.0f);
    addAndMakeVisible(sampleRateLabel);

    styleLabel(latencyLabel, 13.0f);
    addAndMakeVisible(latencyLabel);

    refreshLabels();
    startTimerHz(4);
}

DevicePanel::~DevicePanel()
{
    bufferCombo.removeListener(this);
}

void DevicePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::white.withAlpha(0.94f));
    g.fillRoundedRectangle(bounds, 18.0f);

    g.setColour(juce::Colour::fromRGB(225, 219, 246));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 18.0f, 1.0f);
}

void DevicePanel::resized()
{
    auto area = getLocalBounds().reduced(18);

    titleLabel.setBounds(area.removeFromTop(28));
    helperLabel.setBounds(area.removeFromTop(38));

    area.removeFromTop(8);
    selector->setBounds(area.removeFromTop(250));

    area.removeFromTop(12);
    auto bufferRow = area.removeFromTop(34);
    bufferLabel.setBounds(bufferRow.removeFromLeft(80));
    bufferCombo.setBounds(bufferRow.removeFromLeft(160));

    area.removeFromTop(10);
    sampleRateLabel.setBounds(area.removeFromTop(24));
    latencyLabel.setBounds(area.removeFromTop(24));
}

void DevicePanel::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &bufferCombo)
    {
        const int selectedBuffer = bufferCombo.getSelectedId();
        if (selectedBuffer > 0)
            audioEngine.setRequestedBufferSize(selectedBuffer);
    }
}

void DevicePanel::timerCallback()
{
    refreshLabels();
}

void DevicePanel::refreshLabels()
{
    const auto sr = audioEngine.getCurrentSampleRate();
    const auto buffer = audioEngine.getCurrentBufferSize();

    if (bufferCombo.getSelectedId() != buffer)
        bufferCombo.setSelectedId(buffer, juce::dontSendNotification);

    sampleRateLabel.setText("Sample rate actual: " + juce::String(sr, 0) + " Hz", juce::dontSendNotification);
    latencyLabel.setText("Latencia aprox.: " + juce::String(audioEngine.getApproxTotalLatencyMs(), 1) + " ms", juce::dontSendNotification);
}
