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
    setSize (300, 200);

    // Set up slider styles (no text boxes)
    decaySlider.setSliderStyle(juce::Slider::Rotary);
    decaySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(decaySlider);

    dampSlider.setSliderStyle(juce::Slider::Rotary);
    dampSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(dampSlider);

    colorSlider.setSliderStyle(juce::Slider::Rotary);
    colorSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible(colorSlider);
    
    gateButton.setClickingTogglesState(true);
    gateButton.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(gateButton);
    
    stereoButton.setClickingTogglesState(true);
    stereoButton.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(stereoButton);
    
    gateButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);          // OFF state
    gateButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);      // ON state
    stereoButton.setColour(juce::TextButton::buttonColourId, juce::Colours::grey);          // OFF state
    stereoButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);      // ON state
    
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::LuckyPlucker_png, BinaryData::LuckyPlucker_pngSize);
    
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "DECAY", decaySlider);
    dampAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "DAMP", dampSlider);
    colorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "COLOR", colorSlider);
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "GATE", gateButton);
    stereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "STEREO", stereoButton);
}

LuckyPluckerAudioProcessorEditor::~LuckyPluckerAudioProcessorEditor()
{
}

//==============================================================================
void LuckyPluckerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colours::black); // fallback color
    g.drawImage(backgroundImage, getLocalBounds().toFloat());
}

void LuckyPluckerAudioProcessorEditor::resized()
{
    decaySlider.setBounds(0, 0, 100, 100);
    dampSlider.setBounds(100, 100, 100, 100);
    colorSlider.setBounds(200, 0, 100, 100);
    
    gateButton.setBounds(10, 130, 80, 40);
    stereoButton.setBounds(210, 130, 80, 40);
}
