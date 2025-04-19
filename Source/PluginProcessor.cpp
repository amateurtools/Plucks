/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluckVoice.h"
#include "PluckSound.h"

//==============================================================================
LuckyPluckerAudioProcessor::LuckyPluckerAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Add voices
    for (int i = 0; i < 16; ++i)
        synth.addVoice(new PluckVoice());

    // Add a dummy sound (required by JUCE to trigger voices)
    synth.addSound(new PluckSound());
    
    juce::File logFile = juce::File::getSpecialLocation(juce::File::userDesktopDirectory).getChildFile("plugin_debug_log.txt");
        juce::Logger::setCurrentLogger(new juce::FileLogger(logFile, "JUCE Plugin Debug Log", 100000));
        juce::Logger::writeToLog("Logger initialized.");
}

LuckyPluckerAudioProcessor::~LuckyPluckerAudioProcessor()
{
}

//==============================================================================
const juce::String LuckyPluckerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LuckyPluckerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LuckyPluckerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LuckyPluckerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LuckyPluckerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LuckyPluckerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int LuckyPluckerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void LuckyPluckerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String LuckyPluckerAudioProcessor::getProgramName (int index)
{
    return {};
}

void LuckyPluckerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void LuckyPluckerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate(sampleRate);
}

void LuckyPluckerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

juce::AudioProcessorValueTreeState::ParameterLayout LuckyPluckerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DECAY", 1 }, // version hint = 1
        "Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DAMP", 1 },
        "Damp",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "TONE", 1 },
        "Tone",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "GATE", 1 },
        "Gate",
        false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "STEREO", 1 },
        "Stereo",
        false));

    return { params.begin(), params.end() };
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LuckyPluckerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void LuckyPluckerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    // Set the pluck parameters
    float decay = *parameters.getRawParameterValue("DECAY");
    float damp = *parameters.getRawParameterValue("DAMP");
    float color = *parameters.getRawParameterValue("TONE");
    
    bool gateEnabled = parameters.getRawParameterValue("GATE")->load();
    bool stereoEnabled = parameters.getRawParameterValue("STEREO")->load();

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* pluckVoice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
        {
            pluckVoice->setParameters(decay, damp, color);
            pluckVoice->setGateEnabled(gateEnabled);
            pluckVoice->setStereoEnabled(stereoEnabled);
        }
    }
    
    buffer.clear();
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}

//==============================================================================
bool LuckyPluckerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* LuckyPluckerAudioProcessor::createEditor()
{
    return new LuckyPluckerAudioProcessorEditor (*this);
}

//==============================================================================
void LuckyPluckerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void LuckyPluckerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LuckyPluckerAudioProcessor();
}
