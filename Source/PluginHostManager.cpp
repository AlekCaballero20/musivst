#include "PluginHostManager.h"

class PluginHostManager::PluginEditorWindow final : public juce::DocumentWindow
{
public:
    explicit PluginEditorWindow(juce::AudioProcessorEditor* editor)
        : juce::DocumentWindow("Editor del plugin",
                               juce::Colours::white,
                               juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton)
    {
        setUsingNativeTitleBar(true);
        setResizable(true, true);
        setContentOwned(editor, true);
        centreWithSize(juce::jmax(640, getWidth()), juce::jmax(420, getHeight()));
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

PluginHostManager::PluginHostManager()
{
    juce::addDefaultFormatsToManager(formatManager);
}

PluginHostManager::~PluginHostManager()
{
    closeEditor();
    unloadPlugin();
}

bool PluginHostManager::findPluginDescriptionForFile(const juce::File& pluginFile,
                                                     juce::PluginDescription& description,
                                                     juce::String& errorMessage)
{
    if (! pluginFile.exists())
    {
        errorMessage = "No se encontró el archivo o carpeta del plugin.";
        return false;
    }

    const auto identifier = pluginFile.getFullPathName();
    juce::OwnedArray<juce::PluginDescription> foundTypes;

    for (auto* format : formatManager.getFormats())
    {
        if (format == nullptr)
            continue;

        if (! format->fileMightContainThisPluginType(identifier))
            continue;

        format->findAllTypesForFile(foundTypes, identifier);
    }

    if (foundTypes.isEmpty())
    {
        errorMessage = "JUCE no reconoció ese archivo/carpeta como un plugin VST3 válido.";
        return false;
    }

    description = *foundTypes.getFirst();
    return true;
}

bool PluginHostManager::loadPluginFromFile(const juce::File& pluginFile, double sampleRate, int blockSize)
{
    closeEditor();

    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    currentBlockSize = blockSize > 0 ? blockSize : 512;

    juce::PluginDescription description;
    juce::String errorMessage;

    if (! findPluginDescriptionForFile(pluginFile, description, errorMessage))
    {
        setError(errorMessage);
        return false;
    }

    juce::String creationError;
    auto newPlugin = formatManager.createPluginInstance(description, currentSampleRate, currentBlockSize, creationError);

    if (newPlugin == nullptr)
    {
        setError(creationError.isNotEmpty() ? creationError : "El plugin falló al crearse. Qué detalle tan humano: fallar sin explicar por qué.");
        return false;
    }

    try
    {
        newPlugin->setPlayConfigDetails(2, 2, currentSampleRate, currentBlockSize);
        newPlugin->prepareToPlay(currentSampleRate, currentBlockSize);
    }
    catch (...)
    {
        setError("El plugin lanzó un error durante prepareToPlay(). La app sigue viva, el plugin no.");
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(pluginMutex);
        pluginReady.store(false, std::memory_order_release);

        if (plugin != nullptr)
            plugin->releaseResources();

        plugin = std::move(newPlugin);
        currentDescription = description;
        pluginName = plugin->getName();
        pluginPath = pluginFile.getFullPathName();
        lastError.clear();
        pluginLatencySamples.store(plugin->getLatencySamples(), std::memory_order_release);
        pluginReady.store(true, std::memory_order_release);
    }

    return true;
}

bool PluginHostManager::restorePluginFromPath(const juce::String& path, double sampleRate, int blockSize)
{
    if (path.trim().isEmpty())
        return false;

    return loadPluginFromFile(juce::File(path), sampleRate, blockSize);
}

void PluginHostManager::unloadPlugin()
{
    closeEditor();

    std::lock_guard<std::mutex> lock(pluginMutex);
    pluginReady.store(false, std::memory_order_release);

    if (plugin != nullptr)
        plugin->releaseResources();

    plugin.reset();
    pluginName = "Ningún plugin cargado";
    pluginPath.clear();
    pluginLatencySamples.store(0, std::memory_order_release);
}

void PluginHostManager::prepare(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    currentBlockSize = blockSize > 0 ? blockSize : 512;

    std::lock_guard<std::mutex> lock(pluginMutex);

    if (plugin == nullptr)
        return;

    pluginReady.store(false, std::memory_order_release);
    plugin->releaseResources();
    plugin->setPlayConfigDetails(2, 2, currentSampleRate, currentBlockSize);
    plugin->prepareToPlay(currentSampleRate, currentBlockSize);
    pluginLatencySamples.store(plugin->getLatencySamples(), std::memory_order_release);
    pluginReady.store(true, std::memory_order_release);
}

void PluginHostManager::releaseResources()
{
    std::lock_guard<std::mutex> lock(pluginMutex);

    if (plugin != nullptr)
        plugin->releaseResources();

    pluginReady.store(false, std::memory_order_release);
}

bool PluginHostManager::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (! pluginReady.load(std::memory_order_acquire))
        return false;

    if (! pluginMutex.try_lock())
    {
        buffer.clear();
        return true;
    }

    std::unique_lock<std::mutex> lock(pluginMutex, std::adopt_lock);

    if (plugin == nullptr)
        return false;

    try
    {
        plugin->processBlock(buffer, midiMessages);
    }
    catch (...)
    {
        // A crashing plugin should not blast the PA. Silence is boring, but safe.
        buffer.clear();
    }

    return true;
}

bool PluginHostManager::hasPlugin() const noexcept
{
    return pluginReady.load(std::memory_order_acquire);
}

bool PluginHostManager::hasEditor() const
{
    std::lock_guard<std::mutex> lock(pluginMutex);
    return plugin != nullptr && plugin->hasEditor();
}

bool PluginHostManager::showEditor()
{
    if (editorWindow != nullptr)
    {
        editorWindow->setVisible(true);
        editorWindow->toFront(true);
        return true;
    }

    std::lock_guard<std::mutex> lock(pluginMutex);

    if (plugin == nullptr)
    {
        setError("No hay plugin cargado para abrir editor.");
        return false;
    }

    if (! plugin->hasEditor())
    {
        setError("Este plugin no expone editor visual.");
        return false;
    }

    auto* editor = plugin->createEditor();

    if (editor == nullptr)
    {
        setError("El plugin dijo que tenía editor, pero no lo entregó. Clásico drama de plugins.");
        return false;
    }

    editorWindow = std::make_unique<PluginEditorWindow>(editor);
    return true;
}

void PluginHostManager::closeEditor()
{
    editorWindow.reset();
}

juce::String PluginHostManager::getPluginName() const
{
    return pluginName;
}

juce::String PluginHostManager::getPluginPath() const
{
    return pluginPath;
}

juce::String PluginHostManager::getLastError() const
{
    return lastError;
}

int PluginHostManager::getPluginLatencySamples() const noexcept
{
    return pluginLatencySamples.load(std::memory_order_acquire);
}

void PluginHostManager::setError(const juce::String& message)
{
    lastError = message;
}
