/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

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


// class FineTuneSlider : public juce::Slider
// {
// public:
//     FineTuneSlider()
//     {
//         setRange(-100.0, 100.0, 1.0);
//         setValue(0);
//         setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
//         setSliderStyle(juce::Slider::LinearHorizontal);
//         setSliderSnapsToMousePosition(false);
//     }

//     juce::Rectangle<int> getThumbBoundsManually() const
//     {
//         auto sliderBounds = getLocalBounds().toFloat();

//         // Define thumb size (depends on slider style and your drawing)
//         float thumbWidth = 250.0f;  // example fixed width
//         float thumbHeight = 90.0f; // example fixed height

//         // Map the slider value to position on slider length
//         float sliderValue = (float) getValue();
//         float minValue = (float) getRange().getStart();
//         float maxValue = (float) getRange().getEnd();

//         float x = juce::jmap(sliderValue, minValue, maxValue,
//                             sliderBounds.getX() + thumbWidth * 0.5f,
//                             sliderBounds.getRight() - thumbWidth * 0.5f);

//         // Center the thumb rectangle on x
//         juce::Rectangle<float> thumbRect;
//         thumbRect.setSize(thumbWidth, thumbHeight);
//         thumbRect.setCentre(x, sliderBounds.getCentreY());

//         return thumbRect.toNearestInt();
//     }

//     void paint(juce::Graphics& g) override
//     {
//         // Your existing drawing code...
//         if (logoImage.isValid())
//         {
//             float targetWidth = 250.0f;
//             float aspectRatio = (float)logoImage.getHeight() / logoImage.getWidth();
//             float targetHeight = targetWidth * aspectRatio;

//             auto sliderBounds = getLocalBounds().toFloat();

//             float knobPositionX = juce::jmap((float)getValue(), 
//                                              (float)getRange().getStart(), 
//                                              (float)getRange().getEnd(),
//                                              sliderBounds.getX() + targetWidth * 0.5f,
//                                              sliderBounds.getRight() - targetWidth * 0.5f);

//             juce::Rectangle<float> imageBounds;
//             imageBounds.setSize(targetWidth, targetHeight);
//             imageBounds.setCentre(knobPositionX, sliderBounds.getCentreY());

//             g.drawImage(logoImage, imageBounds);
//         }
//         else
//         {
//             juce::Slider::paint(g);
//         }
//     }

//     void mouseDown(const juce::MouseEvent& event) override
//     {
//         if (event.mods.isRightButtonDown())
//         {
//             new FineTuneInputWindow(getValue(), [this](float val)
//             {
//                 setValue(val, juce::sendNotification);
//             });
//         }
//         else if (event.getNumberOfClicks() == 2)
//         {
//             setValue(0.0f, juce::sendNotification);
//         }
//         else
//         {
//             if (getThumbBoundsManually().contains(event.position.toInt()))
//                 juce::Slider::mouseDown(event);
//         }
//     }


//     void mouseDrag(const juce::MouseEvent& event) override
//     {
//         juce::Slider::mouseDrag(event);  // allow dragging normally
//     }

//     void mouseUp(const juce::MouseEvent& event) override
//     {
//         juce::Slider::mouseUp(event);
//     }

//     void setImage(const juce::Image& img)
//     {
//         logoImage = img;
//     }

// private:
//     juce::Image logoImage;
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



