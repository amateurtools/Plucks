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
PlucksAudioProcessor::PlucksAudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, "PARAMETERS", createParameterLayout()),
    voiceCounter(0)  // Initialize voice counter for age tracking
{
    // Add voices
    for (int i = 0; i < 24; ++i) // don't need 36 voices because: Re-excitement
    {
        synth.addVoice(new PluckVoice(parameters));
        voiceAges.push_back(0);  // Initialize voice ages
    }
    
    // Add a dummy sound (required by JUCE to trigger voices)
    synth.addSound(new PluckSound());
}

PlucksAudioProcessor::~PlucksAudioProcessor()
{
}

//==============================================================================
const juce::String PlucksAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PlucksAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PlucksAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PlucksAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PlucksAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PlucksAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PlucksAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PlucksAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PlucksAudioProcessor::getProgramName (int index)
{
    return {};
}

void PlucksAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
// void PlucksAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
// {
//     currentSampleRate = sampleRate;
//     synth.setCurrentPlaybackSampleRate(sampleRate);

//     // Manually inform voices about the new sample rate
//     for (int i = 0; i < synth.getNumVoices(); ++i)
//     {
//         if (auto* voice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
//         {
//             voice->setCurrentPlaybackSampleRate(sampleRate);
//             // Optionally add samplesPerBlock and channel count if your voice needs it
//             // voice->prepareToPlay(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
//         }
//     }
// }



void PlucksAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

juce::AudioProcessorValueTreeState::ParameterLayout PlucksAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DECAY", 1 }, // version hint = 1
        "Decay",
        juce::NormalisableRange<float>(0.5f, 60.0f, 0.01f), 15.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DAMP", 1 },
        "Damp",
        juce::NormalisableRange<float>(0.02f, 0.8f, 0.01f), 0.05f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "COLOR", 1 },
        "Color",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.85f));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "GATE", 1 },
        "Gate",
        false));
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "STEREO", 1 },
        "Stereo",
        false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "FILTERMODE", 1 },
        "FilterMode",
        false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "FINETUNE", 1 },
        "FineTune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f));

    // this is for finding a good sustain balance for high notes compared to low ones
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "K", 1 },
        "highNotesSustain",
        juce::NormalisableRange<float>(0.25f, 2.0f, 0.01f), 1.0f));

    return { params.begin(), params.end() };
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PlucksAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void PlucksAudioProcessor::prepareToPlay(double sampleRate, int maxBlockSize)
{
    currentSampleRate = sampleRate;
    synth.setCurrentPlaybackSampleRate(sampleRate);

    juce::dsp::ProcessSpec spec{ sampleRate, static_cast<juce::uint32>(maxBlockSize), 1 };

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
        {
            voice->setCurrentPlaybackSampleRate(sampleRate);

            // Prepare delay lines
            voice->getLeftDelayLine().reset();
            voice->getLeftDelayLine().prepare(spec);
            voice->getLeftDelayLine().setMaximumDelayInSamples(PluckVoice::getMaxBufferSize() - 1);

            voice->getRightDelayLine().reset();
            voice->getRightDelayLine().prepare(spec);
            voice->getRightDelayLine().setMaximumDelayInSamples(PluckVoice::getMaxBufferSize() - 1);

            // Prepare smoothed delay ramp
            voice->getSmoothedDelay().reset(sampleRate, 0.02f);

            // Prepare filters inside voice
            voice->prepareFilters(spec);

            voice->resetBuffers();
            voice->clearCurrentNote();


        }
    }
}

void PlucksAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update parameters for all voices (keep your existing parameter code)
    bool stereoEnabled = parameters.getRawParameterValue("STEREO")->load();
    float newFineTuneCents = parameters.getRawParameterValue("FINETUNE")->load(); 
    float newDecay = parameters.getRawParameterValue("DECAY")->load(); 
    float newDamp = parameters.getRawParameterValue("DAMP")->load(); 
    float newColor = parameters.getRawParameterValue("COLOR")->load(); 
    float newFilterMode = parameters.getRawParameterValue("FILTERMODE")->load(); 
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* pluckVoice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
        {
            pluckVoice->setStereoEnabled(stereoEnabled);
            pluckVoice->setFineTuneCents(newFineTuneCents);
            pluckVoice->setCurrentDecay(newDecay);
            pluckVoice->setCurrentDamp(newDamp);
            pluckVoice->setCurrentColor(newColor);
            pluckVoice->setCurrentColor(newFilterMode);
        }
    }

    juce::MidiBuffer filteredMidi;
    
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            int midiNote = message.getNoteNumber();
            
            // Reject notes below C1 (MIDI 24)
            if (midiNote < 12 || midiNote > 108)
                continue;

            float velocity = message.getFloatVelocity();
            bool voiceFound = false;
            
            // First, check for re-excitation of existing notes (keep your existing logic)
            for (int i = 0; i < synth.getNumVoices(); ++i)
            {
                if (auto* pluckVoice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
                {
                    if (pluckVoice->isPlayingNote() && pluckVoice->getCurrentlyPlayingNote() == midiNote)
                    {
                        int samplePos = metadata.samplePosition;
                        pluckVoice->scheduleReExcite(samplePos, velocity);
                        
                        // Update voice age for re-excited voice
                        voiceAges[i] = ++voiceCounter;
                        voiceFound = true;
                        break;
                    }
                }
            }
            
            if (!voiceFound)
            {
                // Check if we're at max polyphony
                int activeVoices = getNumActiveVoices();
                
                if (activeVoices >= 36)  // Max polyphony reached
                {
                    // Find oldest voice to steal
                    int voiceToSteal = findOldestVoice();
                    
                    if (voiceToSteal != -1)
                    {
                        if (auto* voice = dynamic_cast<PluckVoice*>(synth.getVoice(voiceToSteal)))
                        {
                            // Hard stop the old voice
                            voice->clearCurrentNote();
                            voice->resetBuffers();
                        }
                    }
                }
                
                // Let JUCE handle the new note normally
                filteredMidi.addEvent(message, metadata.samplePosition);
                
                // Find which voice will get this note and update its age
                updateVoiceAgeForNewNote(midiNote);
            }
        }
        else
        {
            // Pass other messages untouched (keep your existing logic)
            filteredMidi.addEvent(message, metadata.samplePosition);
        }
    }

    synth.renderNextBlock(buffer, filteredMidi, 0, buffer.getNumSamples());
    
    float gain = 0.5f;
    buffer.applyGain(gain);
}

int PlucksAudioProcessor::findOldestVoice()
{
    int oldestVoice = -1;
    uint64_t oldestAge = UINT64_MAX;
    
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
        {
            if (voice->isPlayingNote())  // Only consider active voices
            {
                if (voiceAges[i] < oldestAge)
                {
                    oldestAge = voiceAges[i];
                    oldestVoice = i;
                }
            }
        }
    }
    
    return oldestVoice;
}

// Update age when a new note starts
void PlucksAudioProcessor::updateVoiceAgeForNewNote(int midiNote)
{
    // Find which voice will handle this note and update its age
    // This is called after adding the note to filteredMidi but before renderNextBlock
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
        {
            // Check if this voice just started this note
            if (!voice->isPlayingNote())  // Voice was free, will take this note
            {
                voiceAges[i] = ++voiceCounter;
                break;  // Found the voice that will take it
            }
        }
    }
}

int PlucksAudioProcessor::getNumActiveVoices() const
{
    int count = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
        {
            if (voice->isVoiceActive())
                ++count;
        }
    }
    return count;
}



//==============================================================================
bool PlucksAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PlucksAudioProcessor::createEditor()
{
    return new PlucksAudioProcessorEditor (*this);
}

//==============================================================================
void PlucksAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PlucksAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlucksAudioProcessor();
}
