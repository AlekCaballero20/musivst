#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <atomic>

class LevelMeter final : public juce::Component,
                         private juce::Timer
{
public:
    LevelMeter();

    void setLabel(const juce::String& newLabel);
    void setLevel(float newLevel) noexcept;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    juce::String label { "Level" };
    std::atomic<float> level { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};
