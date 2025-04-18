/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
LuckyPluckerAudioProcessorEditor::LuckyPluckerAudioProcessorEditor (LuckyPluckerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);
    
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "DECAY", decaySlider);
    dampAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "DAMP", dampSlider);
    colorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "COLOR", colorSlider);

    // Set up slider styles (optional customization)
    decaySlider.setSliderStyle(juce::Slider::Rotary);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(decaySlider);

    dampSlider.setSliderStyle(juce::Slider::Rotary);
    dampSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(dampSlider);
    
    colorSlider.setSliderStyle(juce::Slider::Rotary);
    colorSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(colorSlider);
}

LuckyPluckerAudioProcessorEditor::~LuckyPluckerAudioProcessorEditor()
{
}

//==============================================================================
void LuckyPluckerAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (15.0f));
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void LuckyPluckerAudioProcessorEditor::resized()
{
    decaySlider.setBounds(40, 40, 100, 100);
    dampSlider.setBounds(140, 40, 100, 100);
    colorSlider.setBounds(240, 40, 100, 100);
}
