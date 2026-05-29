#include "MainComponent.h"

namespace
{
    juce::Colour purple() { return juce::Colour::fromRGB(112, 72, 232); }
    juce::Colour violet() { return juce::Colour::fromRGB(142, 93, 255); }
    juce::Colour blue()   { return juce::Colour::fromRGB(102, 190, 255); }
    juce::Colour pink()   { return juce::Colour::fromRGB(255, 132, 196); }
    juce::Colour ink()    { return juce::Colour::fromRGB(56, 45, 76); }
}

MainComponent::MainComponent()
    : loadedSettings(settingsManager.load()),
      audioEngine(pluginHost)
{
    setSize(1040, 720);

    audioEngine.initialise(loadedSettings);

    devicePanel = std::make_unique<DevicePanel>(audioEngine);
    pluginPanel = std::make_unique<PluginPanel>(audioEngine, pluginHost);

    appTitle.setText("MusiVST Guitar Host", juce::dontSendNotification);
    appTitle.setFont(juce::FontOptions(32.0f, juce::Font::bold));
    appTitle.setColour(juce::Label::textColourId, ink());
    addAndMakeVisible(appTitle);

    subtitle.setText("Guitarra → Interfaz → VST3 → Salida estéreo", juce::dontSendNotification);
    subtitle.setFont(juce::FontOptions(15.0f));
    subtitle.setColour(juce::Label::textColourId, juce::Colour::fromRGB(94, 82, 119));
    addAndMakeVisible(subtitle);

    configureButton(audioToggleButton, purple());
    audioToggleButton.setButtonText("Audio OFF");
    audioToggleButton.onClick = [this]
    {
        const auto nextState = ! audioEngine.isAudioEnabled();
        audioEngine.setAudioEnabled(nextState);
        if (nextState)
            audioEngine.setMuted(false);
        updateStatusText();
    };
    addAndMakeVisible(audioToggleButton);

    configureButton(muteButton, juce::Colour::fromRGB(255, 105, 145));
    muteButton.setButtonText("Mute / Panic");
    muteButton.onClick = [this]
    {
        if (audioEngine.isMuted())
            audioEngine.setMuted(false);
        else
            audioEngine.panic();

        updateStatusText();
    };
    addAndMakeVisible(muteButton);

    inputMeter.setLabel("Input level");
    outputMeter.setLabel("Output level");
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    addAndMakeVisible(*devicePanel);
    addAndMakeVisible(*pluginPanel);

    statusTitle.setText("Estado de escenario", juce::dontSendNotification);
    statusTitle.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    statusTitle.setColour(juce::Label::textColourId, ink());
    addAndMakeVisible(statusTitle);

    statusDetails.setFont(juce::FontOptions(14.0f));
    statusDetails.setColour(juce::Label::textColourId, juce::Colour::fromRGB(66, 57, 90));
    statusDetails.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(statusDetails);

    restoreLastPluginAsync();
    updateStatusText();
    startTimerHz(12);
}

MainComponent::~MainComponent()
{
    stopTimer();
    saveSettings();
}

void MainComponent::configureButton(juce::TextButton& button, juce::Colour colour, juce::Colour textColour)
{
    button.setColour(juce::TextButton::buttonColourId, colour);
    button.setColour(juce::TextButton::buttonOnColourId, colour.darker(0.15f));
    button.setColour(juce::TextButton::textColourOffId, textColour);
    button.setColour(juce::TextButton::textColourOnId, textColour);
}

void MainComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient background(juce::Colour::fromRGB(252, 250, 255), bounds.getX(), bounds.getY(),
                                    juce::Colour::fromRGB(238, 247, 255), bounds.getRight(), bounds.getBottom(), false);
    background.addColour(0.45, juce::Colour::fromRGB(249, 243, 255));
    background.addColour(0.82, juce::Colour::fromRGB(255, 242, 249));
    g.setGradientFill(background);
    g.fillAll();

    g.setColour(violet().withAlpha(0.10f));
    g.fillEllipse(-90.0f, -100.0f, 300.0f, 300.0f);

    g.setColour(blue().withAlpha(0.13f));
    g.fillEllipse(static_cast<float>(getWidth() - 260), 20.0f, 260.0f, 260.0f);

    g.setColour(pink().withAlpha(0.10f));
    g.fillEllipse(static_cast<float>(getWidth() - 260), static_cast<float>(getHeight() - 220), 320.0f, 260.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(24);

    auto header = area.removeFromTop(86);
    auto titleArea = header.removeFromLeft(560);
    appTitle.setBounds(titleArea.removeFromTop(44));
    subtitle.setBounds(titleArea.removeFromTop(26));

    auto controls = header.removeFromRight(390);
    audioToggleButton.setBounds(controls.removeFromLeft(180).reduced(0, 14));
    controls.removeFromLeft(16);
    muteButton.setBounds(controls.removeFromLeft(180).reduced(0, 14));

    area.removeFromTop(8);
    auto meters = area.removeFromTop(86);
    inputMeter.setBounds(meters.removeFromLeft(getWidth() / 2 - 34));
    meters.removeFromLeft(20);
    outputMeter.setBounds(meters);

    area.removeFromTop(18);
    auto middle = area.removeFromTop(390);
    if (devicePanel != nullptr)
        devicePanel->setBounds(middle.removeFromLeft((getWidth() - 68) / 2));
    middle.removeFromLeft(20);
    if (pluginPanel != nullptr)
        pluginPanel->setBounds(middle);

    area.removeFromTop(18);
    auto status = area;
    statusTitle.setBounds(status.removeFromTop(28));
    statusDetails.setBounds(status.reduced(0, 4));
}

void MainComponent::timerCallback()
{
    inputMeter.setLevel(audioEngine.getInputLevel());
    outputMeter.setLevel(audioEngine.getOutputLevel());
    updateStatusText();
}

void MainComponent::updateStatusText()
{
    audioToggleButton.setButtonText(audioEngine.isAudioEnabled() ? "Audio ON" : "Audio OFF");
    muteButton.setButtonText(audioEngine.isMuted() ? "Unmute" : "Mute / Panic");

    const juce::String audioState = audioEngine.isAudioEnabled() ? "ON" : "OFF";
    const juce::String muteState = audioEngine.isMuted() ? "Sí" : "No";
    const juce::String pluginState = pluginHost.hasPlugin() ? pluginHost.getPluginName() : "Bypass activo";

    juce::String status;
    status << "Audio: " << audioState
           << "    |    Mute: " << muteState
           << "    |    Plugin cargado: " << pluginState << "\n"
           << "Input: " << audioEngine.getCurrentInputDeviceName()
           << "    |    Output: " << audioEngine.getCurrentOutputDeviceName() << "\n"
           << "Buffer: " << audioEngine.getCurrentBufferSize() << " samples"
           << "    |    Sample rate: " << juce::String(audioEngine.getCurrentSampleRate(), 0) << " Hz"
           << "    |    Latencia aprox.: " << juce::String(audioEngine.getApproxTotalLatencyMs(), 1) << " ms\n"
           << "Input level: " << juce::String(audioEngine.getInputLevel(), 2)
           << "    |    Output level: " << juce::String(audioEngine.getOutputLevel(), 2);

    statusDetails.setText(status, juce::dontSendNotification);
}

void MainComponent::saveSettings()
{
    auto* root = new juce::DynamicObject();
    auto* plugin = new juce::DynamicObject();

    root->setProperty("app", "MusiVST Guitar Host");
    root->setProperty("version", "0.1.0");
    root->setProperty("audio", audioEngine.createAudioSettingsJson());

    plugin->setProperty("lastPluginPath", pluginHost.getPluginPath());
    plugin->setProperty("lastPluginName", pluginHost.getPluginName());
    root->setProperty("plugin", juce::var(plugin));

    settingsManager.save(juce::var(root));
}

void MainComponent::restoreLastPluginAsync()
{
    const auto lastPath = settingsManager.getLastPluginPath(loadedSettings);

    if (lastPath.trim().isEmpty())
        return;

    juce::Component::SafePointer<MainComponent> safeThis(this);

    juce::Timer::callAfterDelay(350, [safeThis, lastPath]
    {
        if (auto* self = safeThis.getComponent())
        {
            self->pluginHost.restorePluginFromPath(lastPath,
                                                  self->audioEngine.getCurrentSampleRate(),
                                                  self->audioEngine.getCurrentBufferSize());
            self->updateStatusText();
        }
    });
}
