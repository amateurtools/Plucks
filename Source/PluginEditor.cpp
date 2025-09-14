/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

// =================== Custom LookAndFeels ===================

KnobLookAndFeel::KnobLookAndFeel()
{
    knobImage = juce::ImageCache::getFromMemory(BinaryData::Knob_png, BinaryData::Knob_pngSize);
}

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPosProportional, float rotaryStartAngle,
                                       float rotaryEndAngle, juce::Slider& slider)
{
    if (knobImage.isValid())
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        juce::Graphics::ScopedSaveState ss(g);
        g.addTransform(juce::AffineTransform::rotation(angle, bounds.getCentreX(), bounds.getCentreY()));
        g.drawImage(knobImage, bounds);
    }
    else
    {
        LookAndFeel_V4::drawRotarySlider(g, x, y, width, height,
                                         sliderPosProportional, rotaryStartAngle, rotaryEndAngle, slider);
    }
}

SwitchLookAndFeel::SwitchLookAndFeel()
{
    switchImage = juce::ImageCache::getFromMemory(BinaryData::Switch_png, BinaryData::Switch_pngSize);
}

void SwitchLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();

    if (switchImage.isValid())
    {
        int stateWidth = switchImage.getWidth() / 2;
        juce::Rectangle<int> srcRect = button.getToggleState()
            ? juce::Rectangle<int>(stateWidth, 0, stateWidth, switchImage.getHeight())
            : juce::Rectangle<int>(0, 0, stateWidth, switchImage.getHeight());

        g.drawImage(switchImage,
                    bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                    srcRect.getX(), srcRect.getY(), srcRect.getWidth(), srcRect.getHeight());
    }
    else
    {
        LookAndFeel_V4::drawToggleButton(g, button, shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);
    }
}

//==============================================================================
PlucksAudioProcessorEditor::PlucksAudioProcessorEditor (PlucksAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), tooltipWindow(this, 500)
{
    setSize (600, 400);

    // 1. Sliders: use graphical knob
    decaySlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    decaySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    decaySlider.setLookAndFeel(&knobLNF);
    addAndMakeVisible(decaySlider);

    decaySlider.setMouseDragSensitivity(150.0f);
    decaySlider.setVelocityBasedMode(false);
    // decaySlider.setVelocityModeParameters(1.0, 0.2, 0.5);


    dampSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    dampSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    dampSlider.setLookAndFeel(&knobLNF);
    addAndMakeVisible(dampSlider);

    colorSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    colorSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    colorSlider.setLookAndFeel(&knobLNF);
    addAndMakeVisible(colorSlider);

    finetuneSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    finetuneSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    finetuneSlider.setRange(-100.0, 100.0, 1.0);  // cents range
    finetuneSlider.setValue(0);
    // Add custom image:
    finetuneSlider.setImage(juce::ImageCache::getFromMemory(BinaryData::Logo_png, BinaryData::Logo_pngSize));
    addAndMakeVisible(finetuneSlider);

    // 2. Toggle switches: use switch look and feel
    gateButton.setClickingTogglesState(true);
    gateButton.setToggleState(false, juce::dontSendNotification);
    gateButton.setLookAndFeel(&switchLNF);
    addAndMakeVisible(gateButton);

    stereoButton.setClickingTogglesState(true);
    stereoButton.setToggleState(false, juce::dontSendNotification);
    stereoButton.setLookAndFeel(&switchLNF);
    addAndMakeVisible(stereoButton);

    // 3. Background image
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::Background_png, BinaryData::Background_pngSize);

    // 4. Attachments
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "DECAY", decaySlider);
    dampAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "DAMP", dampSlider);
    colorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "COLOR", colorSlider);

    finetuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, "FINETUNE", finetuneSlider);

    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "GATE", gateButton);
    stereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.parameters, "STEREO", stereoButton);

    startTimerHz(10); // repaint 10 times per second

    InfoOverlayImage = juce::ImageCache::getFromMemory(BinaryData::Info_png, BinaryData::Info_pngSize);
    buttonOverlayImage = juce::ImageCache::getFromMemory(BinaryData::Button_png, BinaryData::Button_pngSize);

    // Set the button rectangle size (dynamic from button image size)
    buttonBounds = juce::Rectangle<int>(10, 10, buttonOverlayImage.getWidth(), buttonOverlayImage.getHeight());
    overlayActive = false;

    finetuneSlider.setTooltip("FineTune Cents");




}

void PlucksAudioProcessorEditor::timerCallback()
{
    repaint();
}


PlucksAudioProcessorEditor::~PlucksAudioProcessorEditor()
{
    decaySlider.setLookAndFeel(nullptr);
    dampSlider.setLookAndFeel(nullptr);
    colorSlider.setLookAndFeel(nullptr);

    gateButton.setLookAndFeel(nullptr);
    stereoButton.setLookAndFeel(nullptr);
}


//==============================================================================
void PlucksAudioProcessorEditor::paint(juce::Graphics& g)
{
    if (!overlayActive)
    {
        g.drawImage(backgroundImage, getLocalBounds().toFloat());

        if (buttonOverlayImage.isValid())
            g.drawImage(buttonOverlayImage, buttonBounds.toFloat());
        // Draw active voices count
        g.setColour(juce::Colours::black);
        g.setFont(18.0f);
        int activeVoices = audioProcessor.getNumActiveVoices();
        g.drawText("Voices: " + juce::String(activeVoices), 200, 10, 150, 30, juce::Justification::left);
    }
    else
    {
        if (InfoOverlayImage.isValid())
            g.drawImage(InfoOverlayImage, getLocalBounds().toFloat());

    }
}


void PlucksAudioProcessorEditor::resized()
{
    // Knobs (150x150), spaced horizontally with 30px gap and 20 px from left/top edges
    decaySlider.setBounds(89, 109, 100, 100);
    dampSlider.setBounds(250, 152, 100, 100);
    colorSlider.setBounds(410, 194, 100, 100);

    finetuneSlider.setBounds(0, 303, 450, 88);

    // Toggle switches at top right, stacked vertically
    int toggleWidth = 25;   // reasonable size given spacing
    int toggleHeight = 25;
    int rightMargin = 10;
    int topMargin = 10;
    int spacing = 10;

    // int xPos = getWidth() - toggleWidth - rightMargin;

    gateButton.setBounds(550, 25, toggleWidth, toggleHeight);
    stereoButton.setBounds(550, 350, toggleWidth, toggleHeight);
}

void PlucksAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!overlayActive)
    {
        if (buttonBounds.contains(event.getPosition()))
        {
            overlayActive = true;

            // Hide sliders and toggle buttons
            decaySlider.setVisible(false);
            dampSlider.setVisible(false);
            colorSlider.setVisible(false);
            finetuneSlider.setVisible(false); 

            gateButton.setVisible(false);
            stereoButton.setVisible(false);

            repaint();
            return;
        }
    }
    else
    {
        overlayActive = false;

        // Show sliders and toggle buttons
        decaySlider.setVisible(true);
        dampSlider.setVisible(true);
        colorSlider.setVisible(true);

        finetuneSlider.setVisible(true);

        gateButton.setVisible(true);
        stereoButton.setVisible(true);

        repaint();
    }

    AudioProcessorEditor::mouseDown(event);
}



