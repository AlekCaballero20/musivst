#include "SettingsManager.h"

SettingsManager::SettingsManager()
{
    const auto exe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    configDirectory = exe.getParentDirectory().getChildFile("config");
    settingsFile = configDirectory.getChildFile("user-settings.json");
    defaultSettingsFile = configDirectory.getChildFile("default-settings.json");

    configDirectory.createDirectory();
}

juce::var SettingsManager::load() const
{
    auto loadFromFile = [] (const juce::File& file) -> juce::var
    {
        if (! file.existsAsFile())
            return {};

        auto parsed = juce::JSON::parse(file);
        return parsed.isObject() ? parsed : juce::var();
    };

    if (auto userSettings = loadFromFile(settingsFile); userSettings.isObject())
        return userSettings;

    if (auto defaultSettings = loadFromFile(defaultSettingsFile); defaultSettings.isObject())
        return defaultSettings;

    return createFallbackSettings();
}

bool SettingsManager::save(const juce::var& root) const
{
    if (! root.isObject())
        return false;

    configDirectory.createDirectory();
    const auto text = juce::JSON::toString(root, true);
    return settingsFile.replaceWithText(text);
}

juce::String SettingsManager::getLastPluginPath(const juce::var& settings) const
{
    const auto plugin = settings.getProperty("plugin", juce::var());
    return plugin.getProperty("lastPluginPath", juce::var()).toString();
}

juce::var SettingsManager::createFallbackSettings()
{
    auto* root = new juce::DynamicObject();
    auto* audio = new juce::DynamicObject();
    auto* plugin = new juce::DynamicObject();

    root->setProperty("app", "MusiVST Guitar Host");
    root->setProperty("version", "0.1.0");

    audio->setProperty("bufferSize", 128);
    audio->setProperty("sampleRate", 44100.0);
    audio->setProperty("stateXml", "");

    plugin->setProperty("lastPluginPath", "");

    root->setProperty("audio", juce::var(audio));
    root->setProperty("plugin", juce::var(plugin));

    return juce::var(root);
}
