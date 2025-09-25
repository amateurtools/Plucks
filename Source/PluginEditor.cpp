#include "PluginProcessor.h"
#include "PluginEditor.h"


static juce::Font getcomicFont(float size)
{
    static juce::Typeface::Ptr comicTypeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::comic_ttf, BinaryData::comic_ttfSize);

    juce::FontOptions options;
    options = options.withTypeface(comicTypeface);
    options = options.withHeight(size);

    return juce::Font(options);
}

void PlucksAudioProcessorEditor::updatePageVisibility()
{
    bool isSecondPage = (currentPage == GuiPage::SecondPage);
    
    // Main page controls
    decaySlider.setVisible(!isSecondPage);
    dampSlider.setVisible(!isSecondPage);
    colorSlider.setVisible(!isSecondPage);
    gateButton.setVisible(!isSecondPage);
    stereoButton.setVisible(!isSecondPage);
    
    // Second page controls
    if (fineTuneFader) fineTuneFader->setVisible(isSecondPage);
    if (stereoMicrotuneFader) stereoMicrotuneFader->setVisible(isSecondPage);
    if (gateDampingFader) gateDampingFader->setVisible(isSecondPage);
    if (exciterSlewRateFader) exciterSlewRateFader->setVisible(isSecondPage);

    tuningSelector.setVisible(isSecondPage);
}


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

void PlucksAudioProcessorEditor::setupTuningSelector()
{
    // Populate the combo box with tuning options
    tuningSelector.addItem("Equal Temperament", 1);
    tuningSelector.addItem("Well Temperament", 2);
    tuningSelector.addItem("Just Intonation", 3);
    tuningSelector.addItem("Pythagorean", 4);
    tuningSelector.addItem("Meantone", 5);
    tuningSelector.addSeparator();
    tuningSelector.addItem("Load Custom .tun File...", 6);
    
    tuningSelector.setSelectedId(1); // Default to Equal Temperament
    tuningSelector.setVisible(false); // Initially hidden (page 2 only)
    
    // Set callback for when selection changes
    tuningSelector.onChange = [this] { tuningSelectionChanged(); };
    
    addAndMakeVisible(tuningSelector);
}

void PlucksAudioProcessorEditor::tuningSelectionChanged()
{
    int selectedId = tuningSelector.getSelectedId();

    if (selectedId != lastSelectedTuningId)
    {
        lastSelectedTuningId = selectedId;

        // Only stop voices and change tuning if different
        audioProcessor.stopAllVoicesGracefully();

        switch (selectedId)
        {
            case 1: audioProcessor.getTuningSystem()->resetToEqualTemperament(); break;
            case 2: audioProcessor.getTuningSystem()->setWellTemperament(); break;
            case 3: audioProcessor.getTuningSystem()->setJustIntonation(); break;
            case 4: audioProcessor.getTuningSystem()->setPythagorean(); break;
            case 5: audioProcessor.getTuningSystem()->setMeantone(); break;
            case 6:
                {
                    juce::FileChooser chooser ("Select a .tun tuning file",
                                              juce::File::getSpecialLocation(juce::File::userHomeDirectory),
                                              "*.tun");
                    chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [this](const juce::FileChooser& fc)
                    {
                        auto file = fc.getResult();
                        if (file.exists())
                        {
                            if (audioProcessor.getTuningSystem()->loadTuningFile(file))
                            {
                                tuningSelector.setText(audioProcessor.getTuningSystem()->getCurrentTuningName());
                            }
                            else
                            {
                                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                                       "Error",
                                                                       "Failed to load tuning file. Please check the file format.");
                                tuningSelector.setSelectedId(1);
                            }
                        }
                        else
                        {
                            tuningSelector.setSelectedId(1);
                        }
                    });
                }
                break;
        }
    }
}


//==============================================================================
PlucksAudioProcessorEditor::PlucksAudioProcessorEditor (PlucksAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), tooltipWindow(this, 500)
{
    setSize (600, 400);
    currentPage = GuiPage::Main;  // explicit initialization

    // 1. Sliders: use graphical knob
    decaySlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    decaySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    decaySlider.setLookAndFeel(&knobLNF);
    addAndMakeVisible(decaySlider);

    // change the speed of the dial like this:
    // decaySlider.setMouseDragSensitivity(150.0f);
    // decaySlider.setVelocityBasedMode(false);
    // decaySlider.setVelocityModeParameters(1.0, 0.2, 0.5);

    dampSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    dampSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    dampSlider.setLookAndFeel(&knobLNF);
    addAndMakeVisible(dampSlider);

    colorSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    colorSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    colorSlider.setLookAndFeel(&knobLNF);
    addAndMakeVisible(colorSlider);

    // 2. Toggle switches: use switch look and feel
    gateButton.setClickingTogglesState(true);
    gateButton.setToggleState(false, juce::dontSendNotification);
    gateButton.setLookAndFeel(&switchLNF);
    addAndMakeVisible(gateButton);

    stereoButton.setClickingTogglesState(true);
    stereoButton.setToggleState(false, juce::dontSendNotification);
    stereoButton.setLookAndFeel(&switchLNF);
    addAndMakeVisible(stereoButton);

    fineTuneFader = std::make_unique<ImageFader>(audioProcessor.parameters, "FINETUNE", "Fine Tune", faderLNF);
    stereoMicrotuneFader = std::make_unique<ImageFader>(audioProcessor.parameters, "STEREOMICROTUNECENTS", "Stereo Detune", faderLNF);
    gateDampingFader = std::make_unique<ImageFader>(audioProcessor.parameters, "GATEDAMPING", "Gate Tail", faderLNF);
    exciterSlewRateFader = std::make_unique<ImageFader>(audioProcessor.parameters, "EXCITERSLEWRATE", "Exciter Slew", faderLNF);

    setupTuningSelector();

    addAndMakeVisible(fineTuneFader->slider);
    addAndMakeVisible(fineTuneFader->nameLabel);
    addAndMakeVisible(fineTuneFader->valueLabel);
    
    addAndMakeVisible(stereoMicrotuneFader->slider);
    addAndMakeVisible(stereoMicrotuneFader->nameLabel);
    addAndMakeVisible(stereoMicrotuneFader->valueLabel);

    addAndMakeVisible(gateDampingFader->slider);
    addAndMakeVisible(gateDampingFader->nameLabel);
    addAndMakeVisible(gateDampingFader->valueLabel);

    addAndMakeVisible(exciterSlewRateFader->slider);
    addAndMakeVisible(exciterSlewRateFader->nameLabel);
    addAndMakeVisible(exciterSlewRateFader->valueLabel);

    fineTuneFader->setVisible(false);
    stereoMicrotuneFader->setVisible(false);
    gateDampingFader->setVisible(false);
    exciterSlewRateFader->setVisible(false);

    // 3. Background image
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::Background_png, BinaryData::Background_pngSize);
    background2Image = juce::ImageCache::getFromMemory(BinaryData::Background2_png, BinaryData::Background2_pngSize);
    
    // 4. Attachments
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


    startTimerHz(10); // repaint 10 times per second

    // InfoOverlayImage = juce::ImageCache::getFromMemory(BinaryData::Info_png, BinaryData::Info_pngSize);
    buttonOverlayImage = juce::ImageCache::getFromMemory(BinaryData::Button_png, BinaryData::Button_pngSize);

    // Set the button rectangle size (dynamic from button image size)
    buttonBounds = juce::Rectangle<int>(25, 25, buttonOverlayImage.getWidth(), buttonOverlayImage.getHeight());
    overlayActive = false;

    // finetuneSlider.setTooltip("FineTune Cents");
    updatePageVisibility();  // This ensures correct visibility on startup.
    resized();
}

void PlucksAudioProcessorEditor::timerCallback()
{
    repaint();
}

PlucksAudioProcessorEditor::~PlucksAudioProcessorEditor()
{
    // Reset tuning attachment
    tuningAttachment.reset();
    
    // Existing cleanup...
    fineTuneFader.reset();
    stereoMicrotuneFader.reset();
    gateDampingFader.reset();    
    exciterSlewRateFader.reset();  

    decaySlider.setLookAndFeel(nullptr);
    dampSlider.setLookAndFeel(nullptr);
    colorSlider.setLookAndFeel(nullptr);
    gateButton.setLookAndFeel(nullptr);
    stereoButton.setLookAndFeel(nullptr);
}

//==============================================================================
void PlucksAudioProcessorEditor::paint(juce::Graphics& g)
{
    switch (currentPage)
    {
        case GuiPage::Main:
            g.drawImage(backgroundImage, getLocalBounds().toFloat());
            if (buttonOverlayImage.isValid())
                g.drawImage(buttonOverlayImage, buttonBounds.toFloat());

            // Draw active voices info, etc
            break;

        case GuiPage::SecondPage:
            g.drawImage(background2Image, getLocalBounds().toFloat());
            if (buttonOverlayImage.isValid())
                g.drawImage(buttonOverlayImage, buttonBounds.toFloat());

            // Draw active voices info, etc
            break;

    }
}

void PlucksAudioProcessorEditor::resized()
{
    DBG("resized() called");

    // Existing knob positioning
    decaySlider.setBounds(89, 109, 100, 100);
    dampSlider.setBounds(250, 152, 100, 100);
    colorSlider.setBounds(410, 194, 100, 100);

    // Existing toggle positioning
    int toggleWidth = 25;
    int toggleHeight = 25;
    gateButton.setBounds(550, 25, toggleWidth, toggleHeight);
    stereoButton.setBounds(550, 350, toggleWidth, toggleHeight);

    // Position tuning selector to the left of the uppermost fader
    // Assuming fineTuneFader is at (50, 50, 500, 25)
    int tuningWidth = 180; // Reasonable width for tuning names
    int tuningHeight = 25;
    int tuningX = 50 - tuningWidth - 10; // 10px gap from fader
    int tuningY = 50; // Same Y as uppermost fader
    
    tuningSelector.setBounds(425, 325, 150, 25);

    int faderX = 150;

    // Existing fader positioning
    if (fineTuneFader)
        fineTuneFader->setBounds(faderX, 50, 450, 25);

    if (stereoMicrotuneFader)
        stereoMicrotuneFader->setBounds(faderX, 85, 450, 25);

    if (gateDampingFader)
        gateDampingFader->setBounds(faderX, 120, 450, 25);

    if (exciterSlewRateFader)
        exciterSlewRateFader->setBounds(faderX, 155, 450, 25);
}


void PlucksAudioProcessorEditor::showSecondPageControls(bool show)
{

    fineTuneFader->slider.setVisible(show);
    stereoMicrotuneFader->slider.setVisible(show);
    gateDampingFader->slider.setVisible(show);
    exciterSlewRateFader->slider.setVisible(show);
    // etc. for all second page controls
}

void PlucksAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (buttonBounds.contains(event.getPosition()))
    {
        if (currentPage == GuiPage::Main)
        {
            currentPage = GuiPage::SecondPage;
            
            // Hide main controls
            decaySlider.setVisible(false);
            dampSlider.setVisible(false);
            colorSlider.setVisible(false);
            gateButton.setVisible(false);
            stereoButton.setVisible(false);

            int faderX = 150;

            // Show page 2 controls
            if (fineTuneFader)
            {
                fineTuneFader->setVisible(true);
                fineTuneFader->setBounds(faderX, 50, 450, 25);
            }
            if (stereoMicrotuneFader)
            {
                stereoMicrotuneFader->setVisible(true);
                stereoMicrotuneFader->setBounds(faderX, 85, 450, 25);
            }
            if (gateDampingFader)
            {
                gateDampingFader->setVisible(true);
                gateDampingFader->setBounds(faderX, 120, 450, 25);
            } 
            if (exciterSlewRateFader)
            {
                exciterSlewRateFader->setVisible(true);
                exciterSlewRateFader->setBounds(faderX, 155, 450, 25);
            } 
            // Show tuning selector
            tuningSelector.setVisible(true);
        }
        else // SecondPage -> Main
        {
            currentPage = GuiPage::Main;
            
            // Hide page 2 controls
            if (fineTuneFader)
                fineTuneFader->setVisible(false);
            if (stereoMicrotuneFader)
                stereoMicrotuneFader->setVisible(false);
            if (gateDampingFader)
            gateDampingFader->setVisible(false);
            if (exciterSlewRateFader)
            exciterSlewRateFader->setVisible(false);

            // Hide tuning selector
            tuningSelector.setVisible(false);
                
            // Show main controls
            decaySlider.setVisible(true);
            dampSlider.setVisible(true);
            colorSlider.setVisible(true);
            gateButton.setVisible(true);
            stereoButton.setVisible(true);
        }

        updatePageVisibility();
        repaint();
        return;
    }

    AudioProcessorEditor::mouseDown(event);
}