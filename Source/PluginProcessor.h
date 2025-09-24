#pragma once
#include <JuceHeader.h>
#include "TuningSystem.h"

//==============================================================================

class PlucksAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    PlucksAudioProcessor();
    ~PlucksAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;


    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    juce::AudioProcessorValueTreeState parameters;
    juce::Synthesiser synth;

    //========================= VOICE MANAGEMENT ===================================
    void setMaxVoicesAllowed(int newMax);
    int getMaxVoicesAllowed() const { return maxVoicesAllowed; }
    int getNumActiveVoices() const;

    TuningSystem* getTuningSystem() noexcept { return &tuningSystem; }
    void stopAllVoicesGracefully();

private:

    int maxVoicesAllowed = 16; // Default max polyphony
    
    float currentPitchBend = 0.0f; // TODO

    double currentSampleRate = 44100.0; // default fallback

    // Minimal voice stealing - only when max poly reached
    std::vector<uint64_t> voiceAges;
    uint64_t voiceCounter;
    
    // Simple helper methods
    int findOldestVoice();
    void updateVoiceAgeForNewNote(int midiNote);


    TuningSystem tuningSystem;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlucksAudioProcessor)
};
