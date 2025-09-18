#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// class FineTuneInputWindow : public juce::AlertWindow
// {
// public:
//     FineTuneInputWindow(float initialValue, std::function<void(float)> onFinish)
//       : AlertWindow("Enter FineTune Value", "Enter value (-100 to 100 cents):", AlertWindow::NoIcon),
//         callback(std::move(onFinish))
//     {
//         addTextEditor("value", juce::String(initialValue), "Value:");
//         addButton("OK", 1);
//         addButton("Cancel", 0);

//         // Show modal asynchronously with callback
//         enterModalState(true, juce::ModalCallbackFunction::create([this](int result)
//         {
//             if (result == 1) // OK clicked
//             {
//                 float val = juce::jlimit(-100.0f, 100.0f, getTextEditor("value")->getText().getFloatValue());
//                 callback(val);
//             }
//             delete this;  // Self delete
//         }));
//     }

// private:
//     std::function<void(float)> callback;
// };

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel();
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;
private:
    juce::Image knobImage;
};

// Custom LookAndFeel for switches
class SwitchLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SwitchLookAndFeel();
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
private:
    juce::Image switchImage;
};

//==============================================================================
/**
*/
class PlucksAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      private juce::Timer
{

public:
    PlucksAudioProcessorEditor (PlucksAudioProcessor&);
    ~PlucksAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    void timerCallback() override; // now overrides juce::Timer's pure virtual function
    
    juce::Image InfoOverlayImage;
    juce::Image buttonOverlayImage;
    bool overlayActive = true; // start showing overlay by default
    void mouseDown(const juce::MouseEvent& event) override;

    juce::Rectangle<int> buttonBounds; // for click detection

    juce::TooltipWindow tooltipWindow;

    PlucksAudioProcessor& audioProcessor;
    
    KnobLookAndFeel knobLNF;
    SwitchLookAndFeel switchLNF;
    // FineTuneSlider finetuneSlider;

    juce::Slider decaySlider;
    juce::Slider dampSlider;
    juce::Slider colorSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttachment;

    // std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> finetuneAttachment;

    
    juce::ToggleButton gateButton { "GATE" };
    juce::ToggleButton stereoButton { "STEREO" };
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stereoAttachment;
    
    juce::Image backgroundImage;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlucksAudioProcessorEditor)
};



