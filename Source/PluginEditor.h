#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "TuningSystem.h"



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

class FaderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FaderLookAndFeel()
    {
        faderTrack = juce::ImageCache::getFromMemory(BinaryData::FaderTrack_png, BinaryData::FaderTrack_pngSize);
        faderKnob  = juce::ImageCache::getFromMemory(BinaryData::FaderKnob_png, BinaryData::FaderKnob_pngSize);
        
        // Debug output to verify images loaded
        DBG("FaderTrack valid: " << (faderTrack.isValid() ? "Yes" : "No"));
        DBG("FaderKnob valid: " << (faderKnob.isValid() ? "Yes" : "No"));
        if (faderTrack.isValid())
            DBG("FaderTrack size: " << faderTrack.getWidth() << "x" << faderTrack.getHeight());
        if (faderKnob.isValid())
            DBG("FaderKnob size: " << faderKnob.getWidth() << "x" << faderKnob.getHeight());
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle, juce::Slider& slider) override
    {
        // Draw fader track background (scaled to full slider bounds)
        if (faderTrack.isValid())
        {
            g.drawImage(faderTrack, (float)x, (float)y, (float)width, (float)height, 
                       0, 0, faderTrack.getWidth(), faderTrack.getHeight());
        }
        else
        {
            // Fallback: draw simple track if image didn't load
            g.setColour(juce::Colours::darkgrey);
            g.fillRect(x, y, width, height);
        }

        // Draw knob/wiper at the slider position
        if (faderKnob.isValid())
        {
            const int knobW = faderKnob.getWidth();
            const int knobH = faderKnob.getHeight();
            
            // For horizontal slider: knob moves horizontally based on sliderPos
            float knobX = sliderPos - (knobW / 2.0f);
            float knobY = y + (height - knobH) / 2.0f;  // Center vertically in track
            
            // Clamp knob position to stay within slider bounds
            knobX = juce::jlimit((float)x, (float)(x + width - knobW), knobX);
            
            g.drawImage(faderKnob, knobX, knobY, (float)knobW, (float)knobH, 
                       0, 0, knobW, knobH);
        }
        else
        {
            // Fallback: draw simple wiper knob if image didn't load
            g.setColour(juce::Colours::white);
            const int knobSize = height - 4; // Slightly smaller than track height
            float knobX = sliderPos - (knobSize / 2.0f);
            float knobY = y + 2; // Small margin from track edge
            
            // Clamp position
            knobX = juce::jlimit((float)x, (float)(x + width - knobSize), knobX);
            
            g.fillRoundedRectangle(knobX, knobY, (float)knobSize, (float)knobSize, 3.0f);
        }
    }

private:
    juce::Image faderTrack, faderKnob;
};

struct ImageFader
{
    juce::Slider slider;
    juce::Label nameLabel;
    juce::Label valueLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    ImageFader(juce::AudioProcessorValueTreeState& apvts,
               const juce::String& paramID,
               const juce::String& displayName,
               FaderLookAndFeel& lnf)
    {
        // Setup slider
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setLookAndFeel(&lnf);
        slider.setMouseDragSensitivity(200);
        
        // Setup name label (right-aligned)
        nameLabel.setText(displayName, juce::dontSendNotification);
        nameLabel.setJustificationType(juce::Justification::centredRight);
        nameLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f).withStyle("Bold")));
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::black);
        
        // Setup value label (left-aligned)
        valueLabel.setJustificationType(juce::Justification::centredLeft);
        valueLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
        valueLabel.setColour(juce::Label::textColourId, juce::Colours::black);
        
        // Create parameter attachment
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, paramID, slider);
        
        // Update value label when slider changes
        slider.onValueChange = [this]()
        {
            updateValueLabel();
        };
        
        // Set initial value
        updateValueLabel();
        
        DBG("Created ImageFader for parameter: " << paramID);
        slider.setComponentID("FaderSlider_" + paramID);
    }

    ~ImageFader()
    {
        slider.setLookAndFeel(nullptr);
    }
    
    void updateValueLabel()
    {
        // Format the value nicely
        double value = slider.getValue();
        juce::String valueText;
        
        // Format based on the range - if it's likely cents, show 1 decimal place
        if (slider.getMaximum() <= 10.0) // Likely the stereo microtune (0-5 range)
            valueText = juce::String(value, 2);
        else // Likely the fine tune (-100 to 100 range)
            valueText = juce::String(value, 1);
            
        valueLabel.setText(valueText, juce::dontSendNotification);
    }
    
    void setVisible(bool shouldBeVisible)
    {
        slider.setVisible(shouldBeVisible);
        nameLabel.setVisible(shouldBeVisible);
        valueLabel.setVisible(shouldBeVisible);
    }
    
    void setBounds(int x, int y, int width, int height)
    {
        // Layout: [Label] [====Slider====] [Value]
        int labelWidth = 80;
        int valueWidth = 50;
        int spacing = 5;
        
        nameLabel.setBounds(x, y, labelWidth, height);
        slider.setBounds(x + labelWidth + spacing, y, width - labelWidth - valueWidth - (spacing * 2), height);
        valueLabel.setBounds(x + width - valueWidth, y, valueWidth, height);
    }
};


//==============================================================================

class PlucksAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                      private juce::Timer
{

public:
    PlucksAudioProcessorEditor (PlucksAudioProcessor&);
    ~PlucksAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void showSecondPageControls(bool show);

    void setTuningSystem(const TuningSystem* tuning) { tuningSystem = tuning; }

    enum class GuiPage
    {
        Main,
        SecondPage
    };
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.

    void timerCallback() override; // now overrides juce::Timer's pure virtual function

    KnobLookAndFeel knobLNF;
    SwitchLookAndFeel switchLNF;
    FaderLookAndFeel faderLNF;

    juce::Image InfoOverlayImage;
    juce::Image buttonOverlayImage;
    bool overlayActive = true; // start showing overlay by default
    void mouseDown(const juce::MouseEvent& event) override;

    juce::Rectangle<int> buttonBounds; // for click detection

    juce::TooltipWindow tooltipWindow;

    PlucksAudioProcessor& audioProcessor;

    GuiPage currentPage = GuiPage::Main;
    void updatePageVisibility();

    // Tuning system UI
    juce::ComboBox tuningSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> tuningAttachment;
    void setupTuningSelector();
    void tuningSelectionChanged();

    const TuningSystem* tuningSystem = nullptr;
    int lastSelectedTuningId = 1; // or whatever initial tuning ID you have

    std::unique_ptr<ImageFader> fineTuneFader;
    std::unique_ptr<ImageFader> stereoMicrotuneFader;
    std::unique_ptr<ImageFader> gateDampingFader;
    std::unique_ptr<ImageFader> exciterSlewRateFader;

    juce::Slider decaySlider;
    juce::Slider dampSlider;
    juce::Slider colorSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttachment;

    
    juce::ToggleButton gateButton { "GATE" };
    juce::ToggleButton stereoButton { "STEREO" };
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stereoAttachment;
    
    juce::Image backgroundImage;
    juce::Image background2Image;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlucksAudioProcessorEditor)
};



