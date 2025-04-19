/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class LuckyPluckerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    LuckyPluckerAudioProcessorEditor (LuckyPluckerAudioProcessor&);
    ~LuckyPluckerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    LuckyPluckerAudioProcessor& audioProcessor;
    
    juce::Slider decaySlider;
    juce::Slider dampSlider;
    juce::Slider toneSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> toneAttachment;
    
    juce::TextButton gateButton { "GATE" };
    juce::TextButton stereoButton { "STEREO" };
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stereoAttachment;
    
    juce::Image backgroundImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuckyPluckerAudioProcessorEditor)
};
