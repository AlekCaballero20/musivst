#include "LevelMeter.h"

namespace
{
    juce::Colour purple() { return juce::Colour::fromRGB(112, 72, 232); }
    juce::Colour lilac()  { return juce::Colour::fromRGB(198, 174, 255); }
    juce::Colour pink()   { return juce::Colour::fromRGB(255, 132, 196); }
    juce::Colour blue()   { return juce::Colour::fromRGB(102, 190, 255); }
}

LevelMeter::LevelMeter()
{
    startTimerHz(30);
}

void LevelMeter::setLabel(const juce::String& newLabel)
{
    label = newLabel;
    repaint();
}

void LevelMeter::setLevel(float newLevel) noexcept
{
    level.store(juce::jlimit(0.0f, 1.0f, newLevel), std::memory_order_release);
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.setColour(juce::Colours::white);
    g.fillRoundedRectangle(bounds, 14.0f);

    g.setColour(juce::Colour::fromRGB(229, 223, 246));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 14.0f, 1.0f);

    auto textArea = getLocalBounds().removeFromTop(22);
    g.setColour(juce::Colour::fromRGB(65, 55, 82));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(label, textArea.reduced(8, 0), juce::Justification::centredLeft);

    auto meterArea = getLocalBounds().reduced(10, 28).toFloat();
    g.setColour(juce::Colour::fromRGB(243, 239, 252));
    g.fillRoundedRectangle(meterArea, 8.0f);

    const auto current = level.load(std::memory_order_acquire);
    const auto width = meterArea.getWidth() * current;

    if (width > 1.0f)
    {
        juce::ColourGradient gradient(lilac(), meterArea.getX(), meterArea.getY(),
                                      purple(), meterArea.getRight(), meterArea.getY(), false);
        gradient.addColour(0.65, blue());
        gradient.addColour(0.90, pink());
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(meterArea.withWidth(width), 8.0f);
    }

    g.setColour(juce::Colour::fromRGB(82, 70, 102));
    g.setFont(juce::FontOptions(12.0f));
    const auto db = juce::Decibels::gainToDecibels(current, -60.0f);
    g.drawText(juce::String(db, 1) + " dB", getLocalBounds().removeFromBottom(18).reduced(8, 0), juce::Justification::centredRight);
}

void LevelMeter::resized()
{
}

void LevelMeter::timerCallback()
{
    repaint();
}
