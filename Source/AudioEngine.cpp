#include "AudioEngine.h"
#include <cmath>

namespace
{
    constexpr int defaultInputs = 2;
    constexpr int defaultOutputs = 2;
    constexpr int minimumProcessingChannels = 2;
    constexpr int safetyMaxBlockSize = 4096;
}

AudioEngine::AudioEngine(PluginHostManager& pluginHost)
    : pluginManager(pluginHost)
{
}

AudioEngine::~AudioEngine()
{
    deviceManager.removeAudioCallback(this);
    deviceManager.closeAudioDevice();
}

void AudioEngine::initialise(const juce::var& settings)
{
    std::unique_ptr<juce::XmlElement> savedAudioState;

    const auto audio = settings.getProperty("audio", juce::var());
    const auto stateXml = audio.getProperty("stateXml", juce::var()).toString();

    if (stateXml.isNotEmpty())
        savedAudioState = juce::parseXML(stateXml);

    const auto initError = deviceManager.initialise(defaultInputs,
                                                    defaultOutputs,
                                                    savedAudioState.get(),
                                                    true,
                                                    {},
                                                    nullptr);

    if (initError.isNotEmpty())
        DBG("Audio init error: " << initError);

    const int requestedBuffer = static_cast<int>(audio.getProperty("bufferSize", 128));
    if (requestedBuffer > 0)
        setRequestedBufferSize(requestedBuffer);

    deviceManager.addAudioCallback(this);
}

juce::var AudioEngine::createAudioSettingsJson()
{
    auto* audio = new juce::DynamicObject();
    auto setup = deviceManager.getAudioDeviceSetup();

    audio->setProperty("deviceType", deviceManager.getCurrentAudioDeviceType());
    audio->setProperty("inputDeviceName", setup.inputDeviceName);
    audio->setProperty("outputDeviceName", setup.outputDeviceName);
    audio->setProperty("sampleRate", setup.sampleRate);
    audio->setProperty("bufferSize", setup.bufferSize);
    audio->setProperty("inputChannelsMask", setup.inputChannels.toString(16));
    audio->setProperty("outputChannelsMask", setup.outputChannels.toString(16));

    if (auto state = deviceManager.createStateXml())
        audio->setProperty("stateXml", state->toString());

    return juce::var(audio);
}

void AudioEngine::setAudioEnabled(bool shouldBeEnabled) noexcept
{
    audioEnabled.store(shouldBeEnabled, std::memory_order_release);

    if (! shouldBeEnabled)
    {
        inputLevel.store(0.0f, std::memory_order_release);
        outputLevel.store(0.0f, std::memory_order_release);
    }
}

bool AudioEngine::isAudioEnabled() const noexcept
{
    return audioEnabled.load(std::memory_order_acquire);
}

void AudioEngine::setMuted(bool shouldBeMuted) noexcept
{
    muted.store(shouldBeMuted, std::memory_order_release);

    if (shouldBeMuted)
        outputLevel.store(0.0f, std::memory_order_release);
}

bool AudioEngine::isMuted() const noexcept
{
    return muted.load(std::memory_order_acquire);
}

void AudioEngine::panic() noexcept
{
    setMuted(true);
}

void AudioEngine::setRequestedBufferSize(int requestedBufferSize)
{
    auto setup = deviceManager.getAudioDeviceSetup();
    setup.bufferSize = requestedBufferSize;
    const auto error = deviceManager.setAudioDeviceSetup(setup, true);

    if (error.isNotEmpty())
        DBG("Buffer change error: " << error);
}

double AudioEngine::getCurrentSampleRate() const noexcept
{
    return sampleRate.load(std::memory_order_acquire);
}

int AudioEngine::getCurrentBufferSize() const noexcept
{
    return bufferSize.load(std::memory_order_acquire);
}

int AudioEngine::getInputLatencySamples() const noexcept
{
    return inputLatencySamples.load(std::memory_order_acquire);
}

int AudioEngine::getOutputLatencySamples() const noexcept
{
    return outputLatencySamples.load(std::memory_order_acquire);
}

double AudioEngine::getApproxTotalLatencyMs() const noexcept
{
    const auto sr = getCurrentSampleRate();
    if (sr <= 0.0)
        return 0.0;

    const auto totalSamples = getInputLatencySamples()
                            + getOutputLatencySamples()
                            + getCurrentBufferSize()
                            + pluginManager.getPluginLatencySamples();

    return 1000.0 * static_cast<double>(totalSamples) / sr;
}

float AudioEngine::getInputLevel() const noexcept
{
    return inputLevel.load(std::memory_order_acquire);
}

float AudioEngine::getOutputLevel() const noexcept
{
    return outputLevel.load(std::memory_order_acquire);
}

juce::String AudioEngine::getCurrentDeviceType()
{
    return deviceManager.getCurrentAudioDeviceType();
}

juce::String AudioEngine::getCurrentInputDeviceName()
{
    return deviceManager.getAudioDeviceSetup().inputDeviceName;
}

juce::String AudioEngine::getCurrentOutputDeviceName()
{
    return deviceManager.getAudioDeviceSetup().outputDeviceName;
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    for (int channel = 0; channel < numOutputChannels; ++channel)
        if (outputChannelData[channel] != nullptr)
            juce::FloatVectorOperations::clear(outputChannelData[channel], numSamples);

    if (! audioEnabled.load(std::memory_order_acquire)
        || muted.load(std::memory_order_acquire)
        || ! deviceRunning.load(std::memory_order_acquire))
    {
        inputLevel.store(smoothLevel(inputLevel.load(), 0.0f), std::memory_order_release);
        outputLevel.store(0.0f, std::memory_order_release);
        return;
    }

    if (numSamples <= 0 || numSamples > processingBuffer.getNumSamples())
    {
        outputLevel.store(0.0f, std::memory_order_release);
        return;
    }

    processingBuffer.clear(0, numSamples);

    if (numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        processingBuffer.copyFrom(0, 0, inputChannelData[0], numSamples);
        processingBuffer.copyFrom(1, 0, inputChannelData[0], numSamples);
    }

    if (numInputChannels > 1 && inputChannelData[1] != nullptr)
        processingBuffer.copyFrom(1, 0, inputChannelData[1], numSamples);

    const auto rawInputPeak = calculatePeakLevelFromRawInput(inputChannelData, numInputChannels, numSamples);
    inputLevel.store(smoothLevel(inputLevel.load(), rawInputPeak), std::memory_order_release);

    emptyMidi.clear();
    pluginManager.processBlock(processingBuffer, emptyMidi);

    if (muted.load(std::memory_order_acquire))
    {
        processingBuffer.clear(0, numSamples);
        outputLevel.store(0.0f, std::memory_order_release);
        return;
    }

    const auto processedPeak = calculatePeakLevel(processingBuffer, numSamples);
    outputLevel.store(smoothLevel(outputLevel.load(), processedPeak), std::memory_order_release);

    for (int channel = 0; channel < numOutputChannels; ++channel)
    {
        if (outputChannelData[channel] == nullptr)
            continue;

        const int sourceChannel = juce::jmin(channel, processingBuffer.getNumChannels() - 1);
        juce::FloatVectorOperations::copy(outputChannelData[channel], processingBuffer.getReadPointer(sourceChannel), numSamples);
    }
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    const auto sr = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    const auto blockSize = device != nullptr ? device->getCurrentBufferSizeSamples() : 512;

    sampleRate.store(sr, std::memory_order_release);
    bufferSize.store(blockSize, std::memory_order_release);
    inputLatencySamples.store(device != nullptr ? device->getInputLatencyInSamples() : 0, std::memory_order_release);
    outputLatencySamples.store(device != nullptr ? device->getOutputLatencyInSamples() : 0, std::memory_order_release);

    processingBuffer.setSize(minimumProcessingChannels,
                             juce::jlimit(64, safetyMaxBlockSize, juce::jmax(blockSize, 64)),
                             false,
                             false,
                             true);

    pluginManager.prepare(sr, blockSize);
    deviceRunning.store(true, std::memory_order_release);
}

void AudioEngine::audioDeviceStopped()
{
    deviceRunning.store(false, std::memory_order_release);
    inputLevel.store(0.0f, std::memory_order_release);
    outputLevel.store(0.0f, std::memory_order_release);
}

float AudioEngine::calculatePeakLevel(const juce::AudioBuffer<float>& buffer, int numSamples)
{
    float peak = 0.0f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        peak = juce::jmax(peak, buffer.getMagnitude(channel, 0, numSamples));

    return juce::jlimit(0.0f, 1.0f, peak);
}

float AudioEngine::calculatePeakLevelFromRawInput(const float* const* inputChannelData, int numInputChannels, int numSamples)
{
    float peak = 0.0f;

    for (int channel = 0; channel < numInputChannels; ++channel)
    {
        if (inputChannelData[channel] == nullptr)
            continue;

        peak = juce::jmax(peak, juce::FloatVectorOperations::findMinAndMax(inputChannelData[channel], numSamples).getEnd());
        peak = juce::jmax(peak, std::abs(juce::FloatVectorOperations::findMinAndMax(inputChannelData[channel], numSamples).getStart()));
    }

    return juce::jlimit(0.0f, 1.0f, peak);
}

float AudioEngine::smoothLevel(float previous, float next) noexcept
{
    const auto attack = 0.65f;
    const auto release = 0.18f;
    const auto coefficient = next > previous ? attack : release;
    return previous + coefficient * (next - previous);
}
