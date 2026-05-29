#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "PluginHostManager.h"

class AudioEngine final : public juce::AudioIODeviceCallback
{
public:
    explicit AudioEngine(PluginHostManager& pluginHost);
    ~AudioEngine() override;

    void initialise(const juce::var& settings);
    juce::var createAudioSettingsJson();

    juce::AudioDeviceManager& getDeviceManager() noexcept { return deviceManager; }
    const juce::AudioDeviceManager& getDeviceManager() const noexcept { return deviceManager; }

    void setAudioEnabled(bool shouldBeEnabled) noexcept;
    bool isAudioEnabled() const noexcept;

    void setMuted(bool shouldBeMuted) noexcept;
    bool isMuted() const noexcept;
    void panic() noexcept;

    void setRequestedBufferSize(int bufferSize);

    double getCurrentSampleRate() const noexcept;
    int getCurrentBufferSize() const noexcept;
    int getInputLatencySamples() const noexcept;
    int getOutputLatencySamples() const noexcept;
    double getApproxTotalLatencyMs() const noexcept;

    float getInputLevel() const noexcept;
    float getOutputLevel() const noexcept;

    juce::String getCurrentDeviceType();
    juce::String getCurrentInputDeviceName();
    juce::String getCurrentOutputDeviceName();

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;

    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

private:
    static float calculatePeakLevel(const juce::AudioBuffer<float>& buffer, int numSamples);
    static float calculatePeakLevelFromRawInput(const float* const* inputChannelData, int numInputChannels, int numSamples);
    static float smoothLevel(float previous, float next) noexcept;

    PluginHostManager& pluginManager;
    juce::AudioDeviceManager deviceManager;

    juce::AudioBuffer<float> processingBuffer;
    juce::MidiBuffer emptyMidi;

    std::atomic<bool> audioEnabled { false };
    std::atomic<bool> muted { false };
    std::atomic<bool> deviceRunning { false };

    std::atomic<double> sampleRate { 44100.0 };
    std::atomic<int> bufferSize { 512 };
    std::atomic<int> inputLatencySamples { 0 };
    std::atomic<int> outputLatencySamples { 0 };

    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
