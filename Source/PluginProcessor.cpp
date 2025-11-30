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
    for (int i = 0; i < 36; ++i) // don't need 36 voices because: Re-excitement
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
        juce::NormalisableRange<float>(0.25f, 60.0f, 0.01f, 0.5f), 3.0f)); // fourth arg is skew factor

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DAMP", 1 },
        "Damp",
        juce::NormalisableRange<float>(0.0f, 0.65f, 0.01f), 0.2f));
    
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "COLOR", 1 },
        "Color",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "GATE", 1 },
        "Gate",
        false));
        
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "STEREO", 1 },
        "Stereo",
        true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "FINETUNE", 1 },
        "FineTune",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f));

    // this is for finding a good sustain balance for high notes compared to low ones
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "DAMPINGCURVE", 1 },
        "Damping Curve",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "STEREOMICROTUNECENTS", 1 },
        "StereoMicrotuneCents",
        juce::NormalisableRange<float>(0.0f, 5.0f, 0.01f), 0.0f));

    // due to the re-excitement method, monophonic is not possible at this time.    
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID { "MAXVOICES", 1 },
        "Max Voices",
        4,    // minimum integer value
        36,   // maximum integer value
        16    // default integer value
    ));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "GATEDAMPING", 1 },
        "Gate Damping", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));  // 0 to 100ms

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "EXCITERSLEWRATE", 1 },
        "Exciter Slew", 
        juce::NormalisableRange<float>(0.1f, 1.0f, 0.001f), 1.0f));  // de-harsh the exciter

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "TUNING_TYPE",              // parameter ID
        "Tuning Type",              // parameter name
        juce::StringArray{ "Equal Temperament", "Well Temperament", "Just Intonation", "Pythagorean", "Meantone", "Custom" }, // choices
        0                          // default index (0-based)
    ));

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
            
            // Set tuning system reference
            voice->setTuningSystem(&tuningSystem);


            // Existing delay line preparation code...
            voice->getLeftDelayLine().reset();
            voice->getLeftDelayLine().prepare(spec);
            voice->getLeftDelayLine().setMaximumDelayInSamples(PluckVoice::getMaxBufferSize() - 1);

            voice->getRightDelayLine().reset();
            voice->getRightDelayLine().prepare(spec);
            voice->getRightDelayLine().setMaximumDelayInSamples(PluckVoice::getMaxBufferSize() - 1);

            voice->getSmoothedDelayL().reset(sampleRate, 0.02f);
            voice->getSmoothedDelayR().reset(sampleRate, 0.02f);

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
    bool gateEnabled = parameters.getRawParameterValue("GATE")->load();
    bool stereoEnabled = parameters.getRawParameterValue("STEREO")->load();
    float newFineTuneCents = parameters.getRawParameterValue("FINETUNE")->load(); 
    float newDecay = parameters.getRawParameterValue("DECAY")->load(); 
    float newDamp = parameters.getRawParameterValue("DAMP")->load(); 
    float newColor = parameters.getRawParameterValue("COLOR")->load(); 
    maxVoicesAllowed = parameters.getRawParameterValue("MAXVOICES")->load(); // used in processblock
    float newStereoMicrotuneCents = parameters.getRawParameterValue("STEREOMICROTUNECENTS")->load();
    float newExciterSlewRate = parameters.getRawParameterValue("EXCITERSLEWRATE")->load(); 
    float newDampingCurve = parameters.getRawParameterValue("DAMPINGCURVE")->load(); 

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* pluckVoice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
        {
            pluckVoice->setGateEnabled(gateEnabled);
            pluckVoice->setStereoEnabled(stereoEnabled);
            pluckVoice->setFineTuneCents(newFineTuneCents);
            pluckVoice->setCurrentDecay(newDecay);
            pluckVoice->setCurrentDamp(newDamp);
            pluckVoice->setCurrentColor(newColor);
            pluckVoice->setStereoMicrotuneCents(newStereoMicrotuneCents);
            pluckVoice->setExciterSlewRate(newExciterSlewRate);
            pluckVoice->setDampingCurve(newDampingCurve);
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
                        if (gateEnabled)
                        {
                            // Gate mode: immediately clear this voice and retrigger fresh
                            pluckVoice->clearCurrentNote();
                            pluckVoice->resetBuffers();

                            filteredMidi.addEvent(message, metadata.samplePosition); // add new note event to retrigger
                        }
                        else
                        {
                            // Non-gate mode: schedule re-excite
                            pluckVoice->scheduleReExcite(metadata.samplePosition, velocity);
                        }

                        voiceAges[i] = ++voiceCounter;
                        voiceFound = true;
                        break;
                    }
                }
            }
            
            // ==== probably not perfect. may be edge cases of missed notes. let's keep our eyes peeled.
            if (!voiceFound)
            {
                // Check if we're at max polyphony
                int activeVoices = getNumActiveVoices();
                if (activeVoices >= maxVoicesAllowed)  // Max polyphony reached
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

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = totalNumOutputChannels > 1 ? buffer.getWritePointer(1) : leftChannel;

    float gain = 0.3f;
    buffer.applyGain(gain);
}

void PlucksAudioProcessor::setMaxVoicesAllowed(int newMax)
{
    maxVoicesAllowed = std::clamp(newMax, 1, (int)synth.getNumVoices());
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

void PlucksAudioProcessor::stopAllVoicesGracefully()
{
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<PluckVoice*>(synth.getVoice(i)))
        {
            voice->stopNote (0.0f, false);    // immediate stop
            voice->clearCurrentNote();         // clear note state
            voice->resetBuffers();             // reset any internal buffers/delays if applicable
        }
    }
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
    // Create XML from the current parameters state
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    // Copy the XML data to the destination memory block for host to save
    copyXmlToBinary (*xml, destData);
}

void PlucksAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore the XML from binary data from the host
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr)
    {
        // Verify the XML tag matches your parameters state type
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            // Replace your parameters state with restored data
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PlucksAudioProcessor();
}
