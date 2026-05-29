#include "PluginPanel.h"

namespace
{
    juce::Colour purple() { return juce::Colour::fromRGB(112, 72, 232); }

    void styleLabel(juce::Label& label, float size, bool bold = false)
    {
        label.setFont(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain));
        label.setColour(juce::Label::textColourId, juce::Colour::fromRGB(62, 52, 82));
    }

    void styleButton(juce::TextButton& button)
    {
        button.setColour(juce::TextButton::buttonColourId, purple());
        button.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(92, 55, 205));
        button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }
}

PluginPanel::PluginPanel(AudioEngine& engineToUse, PluginHostManager& pluginHostToUse)
    : audioEngine(engineToUse), pluginHost(pluginHostToUse)
{
    titleLabel.setText("Plugin VST3", juce::dontSendNotification);
    styleLabel(titleLabel, 20.0f, true);
    addAndMakeVisible(titleLabel);

    pluginNameLabel.setText("Plugin: Ninguno", juce::dontSendNotification);
    styleLabel(pluginNameLabel, 15.0f, true);
    addAndMakeVisible(pluginNameLabel);

    statusLabel.setText(statusMessage, juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    statusLabel.setMinimumHorizontalScale(0.75f);
    styleLabel(statusLabel, 13.0f);
    addAndMakeVisible(statusLabel);

    loadButton.setButtonText("Load VST3 Plugin");
    styleButton(loadButton);
    loadButton.onClick = [this] { loadPluginClicked(); };
    addAndMakeVisible(loadButton);

    editorButton.setButtonText("Abrir editor");
    styleButton(editorButton);
    editorButton.onClick = [this] { openEditorClicked(); };
    addAndMakeVisible(editorButton);

    unloadButton.setButtonText("Descargar plugin");
    unloadButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(245, 238, 255));
    unloadButton.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(82, 59, 155));
    unloadButton.onClick = [this] { unloadPluginClicked(); };
    addAndMakeVisible(unloadButton);

    refreshState();
    startTimerHz(4);
}

void PluginPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::white.withAlpha(0.94f));
    g.fillRoundedRectangle(bounds, 18.0f);

    g.setColour(juce::Colour::fromRGB(225, 219, 246));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 18.0f, 1.0f);

    auto glow = bounds.reduced(16.0f).removeFromBottom(78.0f);
    juce::ColourGradient gradient(juce::Colour::fromRGB(198, 174, 255).withAlpha(0.18f), glow.getX(), glow.getY(),
                                  juce::Colour::fromRGB(102, 190, 255).withAlpha(0.18f), glow.getRight(), glow.getBottom(), false);
    gradient.addColour(0.7, juce::Colour::fromRGB(255, 132, 196).withAlpha(0.14f));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(glow, 16.0f);
}

void PluginPanel::resized()
{
    auto area = getLocalBounds().reduced(18);

    titleLabel.setBounds(area.removeFromTop(30));
    area.removeFromTop(8);
    pluginNameLabel.setBounds(area.removeFromTop(28));
    statusLabel.setBounds(area.removeFromTop(52));

    area.removeFromTop(14);
    loadButton.setBounds(area.removeFromTop(46));
    area.removeFromTop(10);
    editorButton.setBounds(area.removeFromTop(42));
    area.removeFromTop(10);
    unloadButton.setBounds(area.removeFromTop(38));
}

void PluginPanel::timerCallback()
{
    refreshState();
}

void PluginPanel::loadPluginClicked()
{
    const auto defaultVst3Folder = juce::File("C:\\Program Files\\Common Files\\VST3");

    fileChooser = std::make_unique<juce::FileChooser>("Selecciona un plugin VST3",
                                                       defaultVst3Folder.exists() ? defaultVst3Folder : juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                                       "*.vst3");

    const auto flags = juce::FileBrowserComponent::openMode
                     | juce::FileBrowserComponent::canSelectFiles
                     | juce::FileBrowserComponent::canSelectDirectories;

    juce::Component::SafePointer<PluginPanel> safeThis(this);

    fileChooser->launchAsync(flags, [safeThis] (const juce::FileChooser& chooser)
    {
        auto* self = safeThis.getComponent();
        if (self == nullptr)
            return;

        const auto result = chooser.getResult();
        if (! result.exists())
            return;

        self->statusMessage = "Cargando plugin...";
        self->statusLabel.setText(self->statusMessage, juce::sendNotificationSync);

        const bool ok = self->pluginHost.loadPluginFromFile(result,
                                                            self->audioEngine.getCurrentSampleRate(),
                                                            self->audioEngine.getCurrentBufferSize());

        self->statusMessage = ok ? "Plugin cargado. Ruta: " + result.getFullPathName()
                                 : "Error: " + self->pluginHost.getLastError();

        self->refreshState();
    });
}

void PluginPanel::openEditorClicked()
{
    if (! pluginHost.showEditor())
        statusMessage = "Error: " + pluginHost.getLastError();

    refreshState();
}

void PluginPanel::unloadPluginClicked()
{
    pluginHost.unloadPlugin();
    statusMessage = "Plugin descargado. Bypass activo.";
    refreshState();
}

void PluginPanel::refreshState()
{
    pluginNameLabel.setText("Plugin: " + pluginHost.getPluginName(), juce::dontSendNotification);

    if (pluginHost.hasPlugin() && statusMessage.startsWithIgnoreCase("Sin plugin"))
        statusMessage = "Plugin cargado correctamente.";

    if (! pluginHost.hasPlugin() && ! statusMessage.startsWithIgnoreCase("Error"))
        statusMessage = "Sin plugin cargado. Bypass activo.";

    statusLabel.setText(statusMessage, juce::dontSendNotification);
    editorButton.setEnabled(pluginHost.hasPlugin());
    unloadButton.setEnabled(pluginHost.hasPlugin());
}
