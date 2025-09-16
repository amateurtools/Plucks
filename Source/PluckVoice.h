#pragma once
class PluckVoice : public juce::SynthesiserVoice
{
public:
    PluckVoice(juce::AudioProcessorValueTreeState& params) 
        : apvts(params)
    {
        // Pre-allocate buffers to max size on construction to avoid reallocations during audio
        leftDelayBuffer.resize(maxBufferSize, 0.0f);
        rightDelayBuffer.resize(maxBufferSize, 0.0f);
        // leftDelayLine.prepare({currentSampleRate, maxBlockSize, 1}); // TESTING -- re-added these to constructor, didn't change anything with gaps problem
        // rightDelayLine.prepare({currentSampleRate, maxBlockSize, 1});
    }

    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return true;
    }

    bool isPlayingNote() const { return hasStartedNote; }
    int getCurrentlyPlayingNote() const { return currentMidiNote; }
    
    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        // re-exciter section -- add energy back to already sounding note if active and retriggered.
        // if (currentMidiNote != midiNoteNumber)  // New note or different note
        // {

        // note: removed the above conditional because the same thing is already done in the processblock.

            currentMidiNote = midiNoteNumber; // first of all, in case a calculation needs it

            // these need to be considered global
            setFineTuneCents(apvts.getRawParameterValue("FINETUNE")->load());
            setCurrentDecay(apvts.getRawParameterValue("DECAY")->load());
            setCurrentDamp(apvts.getRawParameterValue("DAMP")->load());
            setCurrentColor(apvts.getRawParameterValue("COLOR")->load());
            setStereoEnabled(apvts.getRawParameterValue("STEREO")->load());
            // setFilterMode(apvts.getRawParameterValue("FILTERMODE")->load());

            // don't need super tiny noises, that's what pulse width is for
            // TODO: to remap the pulse width maker in the exciter generator!
            float mappedValue = 0.25f + velocity * (1.0f - 0.25f);
            currentVelocity = mappedValue; 

            // Reset fade/fade counter just in case
            fadeOut = false;
            fadeCounter = 0;

            // for finetune smoothing
            smoothedDelayLength.reset(currentSampleRate, 0.02); // re-init with 20ms ramp time
    
            // Initialize buffers and exciters fully for this note (mono or stereo)

            // initialize is also for re-excite so don't put voice-creation level stuff in it
            initializeDelayLineAndParameters(midiNoteNumber, currentVelocity);

            smoothedDelayLength.setCurrentAndTargetValue(baseExactDelayFrac);

            hasStartedNote = true;
            reExciterIndex = -1; // No re-excite for new note
        // }
    }

    void stopNote(float, bool allowTailOff) override
    {
        bool gateEnabled = apvts.getRawParameterValue("GATE")->load() > 0.5f;
        if (gateEnabled && allowTailOff)
        {
            fadeOut = true;
            fadeCounter = 0;
            // note will fade out, clearCurrentNote called later
        }
    }

    // Setter to update fine tune in cents and recalculate related delays
    // sets the all important baseExactDelayInt!
    void setDelayTimes()
    {
        currentFineTuneMultiplier = std::pow(2.0f, currentFineTuneCents / 1200.0f);
        
        if (currentMidiNote >= 0 && currentSampleRate > 0.0)
        {
            const auto freq = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote);
            const float exactDelay = currentSampleRate / freq;
            
            // Apply fine tuning by dividing delay by the multiplier
            baseExactDelayFrac = exactDelay / currentFineTuneMultiplier;
            baseExactDelayInt = static_cast<int>(std::round(baseExactDelayFrac));
            
            smoothedDelayLength.setCurrentAndTargetValue(baseExactDelayFrac);
        }
    }

        void clearCurrentNote()
    {
        hasStartedNote = false;
        currentMidiNote = -1;  // Reset the note tracker
        juce::SynthesiserVoice::clearCurrentNote();
    }
    
    void pitchWheelMoved(int) override {}      // must be in here!
    void controllerMoved(int, int) override {} // must be in here!

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>& getLeftDelayLine() { return leftDelayLine; }
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>& getRightDelayLine() { return rightDelayLine; }
    juce::LinearSmoothedValue<float>& getSmoothedDelay() { return smoothedDelayLength; }
    static constexpr int getMaxBufferSize() { return maxBufferSize; }

    // ============================== RE EXCITER ====================================

    void setReExciterIndex(int index) { reExciterIndex = index; }
    std::vector<float>& getReExciterLeft() { return reExciterLeft; }
    std::vector<float>& getReExciterRight() { return reExciterRight; }

    // scheduling the re-excitement is vital due to otherwise asynchronous notes (usually playing too early)
    void scheduleReExcite(int sampleOffset, float velocity)
    {
        pendingReExciteSample = sampleOffset; // flag scanned in rendernextblock
        float mappedValue = 0.25f + velocity * (1.0f - 0.25f);
        pendingReExciteVelocity = mappedValue;
    }

    void reExcite()
    {
        // Use current up-to-date tuning multiplier
        float reExciteVelocity = pendingReExciteVelocity;

        // may or may not be helpful since these are set globally at main processblock
        setFineTuneCents(apvts.getRawParameterValue("FINETUNE")->load());
        setCurrentDecay(apvts.getRawParameterValue("DECAY")->load());
        setCurrentDamp(apvts.getRawParameterValue("DAMP")->load());
        setCurrentColor(apvts.getRawParameterValue("COLOR")->load());
        setStereoEnabled(apvts.getRawParameterValue("STEREO")->load());
        // setFilterMode(apvts.getRawParameterValue("FILTERMODE")->load());

        setDelayTimes();
        generateExciter(reExciteVelocity, reExciterLeft, reExciterRight);

        // Reset smoothing and set value to current fine tuned delay immediately
        smoothedDelayLength.reset(currentSampleRate, 0.001);
        smoothedDelayLength.setCurrentAndTargetValue(baseExactDelayFrac);

        fadeOut = false;
        fadeCounter = 0;
        reExciterIndex = 0;
    }

    void setCurrentPlaybackSampleRate(double newRate) override
    {
        currentSampleRate = newRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = newRate;
        spec.maximumBlockSize = maxBlockSize;
        spec.numChannels = 1;

        leftDelayLine.prepare(spec);
        rightDelayLine.prepare(spec);
    }

    // void prepareFilters(const juce::dsp::ProcessSpec& spec)
    // {
    //     lowpassFilterL.reset();
    //     lowpassFilterR.reset();

    //     // Initial cutoff frequency (e.g., Nyquist)
    //     float nyquist = static_cast<float>(currentSampleRate * 0.5f);
    //     float cutoffFreq = juce::jmin(20000.0f, nyquist - 100.0f);

    //     // Set filter types to lowpass
    //     lowpassFilterL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    //     lowpassFilterR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    //     // Set initial cutoff frequency
    //     lowpassFilterL.setCutoffFrequency(cutoffFreq);
    //     lowpassFilterR.setCutoffFrequency(cutoffFreq);

    //     // Set low resonance (Q)
    //     float lowQ = 0.7f;  // Typical low resonance value giving smooth but slightly colored tone
    //     lowpassFilterL.setResonance(lowQ);
    //     lowpassFilterR.setResonance(lowQ);

    //     // Prepare filters
    //     lowpassFilterL.prepare(spec);
    //     lowpassFilterR.prepare(spec);
    // }

    // =============================== DSP LOOP ===============================

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (baseExactDelayInt < 1 || !hasStartedNote)
            return;

        juce::ScopedNoDenormals noDenormals;

        // Playing with the prospect of extending the decay of higher notes, optionally.
        float decayTimeSeconds = currentDecay; // same cached for re-excite
        float kValueParam = *apvts.getRawParameterValue("K"); // K is a multiplier for decay scaling. will eventually hard code.

        // const float referenceFreq = 220.0f; // roughly A1, a low note for scaling the decay time of higher notes.
        // float freqScale = std::pow(referenceFreq / cyclesPerSecond, kValueParam); // k=1 here, experiment with it to expand the decay of higher notes
        // float scaledDecayTime = decayTimeSeconds * freqScale;

        // Scale based on note number instead of frequency ratio
        float noteScale = std::pow(0.9f, (currentMidiNote - 57) * kValueParam / 12.0f); // A4 = MIDI 69, reference
        float scaledDecayTime = decayTimeSeconds * noteScale;

        // calculate feedback gain from scaledDecayTime
        // maybe move this to the delaytime calculation section
        const float targetAmplitude = 0.001f; // amplitude target after decay time
        float feedbackGain = std::pow(targetAmplitude, 0.999f / (scaledDecayTime * cyclesPerSecond));
        feedbackGain = juce::jlimit(0.0f, 0.999f, feedbackGain);

        // DBG("Decay param: " + juce::String(decayTimeSeconds) + 
        //     " Scaled decay: " + juce::String(scaledDecayTime) + 
        //     " Cycles/sec: " + juce::String(cyclesPerSecond) + 
        //     " Total cycles: " + juce::String(scaledDecayTime * cyclesPerSecond) + 
        //     " Feedback gain: " + juce::String(feedbackGain));

        // Always process as stereo (dual mono if needed)
        auto* outL = outputBuffer.getWritePointer(0, startSample);
        auto* outR = (outputBuffer.getNumChannels() >= 2) ? outputBuffer.getWritePointer(1, startSample) : outL;

        // =========================== LPF related stuff ================================
        // think i will lose this filter, doesn't play well with physical model imo.
        // float minCutoff = 155.0f;    // muffled
        // float nyquist = static_cast<float>(currentSampleRate * 0.5);
        // float maxCutoff = juce::jmin(20000.0f, nyquist - 100.0f);
        // float cutoffFreq = juce::jmap(currentDamp, 0.0f, 1.0f, maxCutoff, minCutoff);
        // // Update cutoff frequency on the filters
        // lowpassFilterL.setCutoffFrequency(cutoffFreq);
        // lowpassFilterR.setCutoffFrequency(cutoffFreq);
        // float lowQ = 0.7f;  // Typical low resonance value giving smooth but slightly colored tone
        // lowpassFilterL.setResonance(lowQ);
        // lowpassFilterR.setResonance(lowQ);
           
        for (int i = 0; i < numSamples; ++i)
        {
            // assure that re-excited samples don't occur asynchronously to midi timing!!
            // otherwise you will render notes before the midi note hits. it's weird. 
            if (pendingReExciteSample ==  (startSample + i))
            {
                reExcite();
                pendingReExciteSample = -1;
            }

            float delayedSampleL = leftDelayLine.popSample(0);
            float delayedSampleR = rightDelayLine.popSample(0);

            // if (filterMode == 1) {
            //     // juce SVF lowpass
            //     filteredSampleL = lowpassFilterL.processSample(0, delayedSampleL);
            //     filteredSampleR = lowpassFilterR.processSample(0, delayedSampleR);   
            // }
            // else
            // {

            // Invert damping to match original behavior
            float dampingAmount = juce::jmap(currentDamp, 0.0f, 1.0f, 0.99f, 0.01f);
            
            filteredSampleL = previousSampleL + dampingAmount * (delayedSampleL - previousSampleL);
            filteredSampleR = previousSampleR + dampingAmount * (delayedSampleR - previousSampleR);

            // }

            // Add re-exciter if active
            float addL = 0.0f;
            float addR = 0.0f;

            if (reExciterIndex >= 0 && reExciterIndex < static_cast<int>(reExciterLeft.size()))
            {
                addL = reExciterLeft[reExciterIndex];
                addR = reExciterRight[reExciterIndex];
                ++reExciterIndex;
                if (reExciterIndex >= static_cast<int>(reExciterLeft.size()))
                    reExciterIndex = -1;
            }
            
            filteredSampleL += addL; //* reExciteFactor; -- scaling should not be needed.
            filteredSampleR += addR; //* reExciteFactor;

            // Fade logic
            ++activeSampleCounter;
            if (!fadeOut && activeSampleCounter >= maxSamplesAllowed)
            {
                fadeOut = true;
                fadeCounter = 0;
            }
            
            if (fadeOut)
            {
                fadeCounter++;
                float fadeMultiplier = std::max(0.0f, 1.0f - fadeCounter / 64.0f);
                filteredSampleL *= fadeMultiplier;
                filteredSampleR *= fadeMultiplier;
                if (fadeMultiplier <= 0.0f)
                {
                    clearCurrentNote();
                    return;
                }
            }
            
            if (!std::isfinite(filteredSampleL)) filteredSampleL = 0.0f;
            if (!std::isfinite(filteredSampleR)) filteredSampleR = 0.0f;
            
            float outputL = filteredSampleL * feedbackGain;
            float outputR = filteredSampleR * feedbackGain;

            leftDelayLine.pushSample(0, outputL);
            rightDelayLine.pushSample(0, outputR);

            // before or after feedback gain doesn't seem to make much of a difference
            // previousSampleL = filteredSampleL;
            // previousSampleR = filteredSampleR;

            previousSampleL = outputL;
            previousSampleR = outputR;

            outL[i] += outputL;
            outR[i] += outputR;

        }
    }

    // ====================== PARAMETER SETTERS ==============================================

    void setFineTuneCents(float newFineTuneCents) //handled in realtime from processblock
    {
        currentFineTuneCents = newFineTuneCents;
    }

        void setCurrentDecay(float newDecay) //handled in realtime from processblock
    {
        currentDecay = newDecay;
    }

        void setCurrentDamp(float newDamp) //handled in realtime from processblock
    {
        currentDamp = newDamp;
    }

        void setCurrentColor(float newColor) //handled in realtime from processblock
    {
        currentColor = newColor;
    }

    //     void setFilterMode(float newFilterMode) //handled in realtime from processblock
    // {
    //     filterMode = newFilterMode;
    // }

    void setStereoEnabled(bool enabled)
    {
        if (stereoEnabled != enabled)
        {
            stereoEnabled = enabled;
            // Clear buffers associated with new mode to avoid leftover noise/artifacts
            if (stereoEnabled)
            {
                std::fill(leftDelayBuffer.begin(), leftDelayBuffer.end(), 0.f);
                std::fill(rightDelayBuffer.begin(), rightDelayBuffer.end(), 0.f);
            }
            else
            {
                std::fill(leftDelayBuffer.begin(), leftDelayBuffer.end(), 0.f);   // no longer using a separate mono buffer
                std::fill(rightDelayBuffer.begin(), rightDelayBuffer.end(), 0.f);
            }
            bufferIndex = 0; // reset index on mode switch for safety
        }
    }

    // ======= MISC TESTING SETTERS ============00

    void setNoiseAmp(float newNoiseAmp) //handled in realtime from processblock
    {
        noiseAmp = newNoiseAmp;
    }

    void setOutputRoute(float newOutputRoute) //handled in realtime from processblock
    {
        outputRoute = newOutputRoute;
    }
    // ======================================================================================

    void resetBuffers()
    {
        std::fill(leftDelayBuffer.begin(), leftDelayBuffer.end(), 0.0f);
        std::fill(rightDelayBuffer.begin(), rightDelayBuffer.end(), 0.0f);
        bufferIndex = 0;
        previousSampleL = 0.0f;
        previousSampleR = 0.0f;
        fadeOut = false;
        fadeCounter = 0;
        activeSampleCounter = 0;
        currentMidiNote = -1;
        hasStartedNote = false;
        leftDelayLine.reset();
        rightDelayLine.reset();
        reExciterIndex = -1;
        reExciterLeft.clear();
        reExciterRight.clear();
    }

private:

    // ==================================== INITIALIZE ===============================

    void initializeDelayLineAndParameters(int midiNoteNumber, float velocity)
    {
        setDelayTimes();
        
        cyclesPerSecond = currentSampleRate / baseExactDelayFrac;
        bufferIndex = 0;

        // Generate exciter
        std::vector<float> exciterLeft, exciterRight;
        generateExciter(velocity, exciterLeft, exciterRight);

        // Reset delay lines to clear any previous data
        leftDelayLine.reset();
        rightDelayLine.reset();

        // Set the write pointer to baseExactDelayInt samples *behind* the start,
        // so popSample(0, baseExactDelay) reads the first exciter sample immediately
        leftDelayLine.setDelay(0); // Reset write pointer to 0
        rightDelayLine.setDelay(0);
  
        // Push exciter samples
        for (int i = 0; i < baseExactDelayInt; ++i)
        {
            leftDelayLine.pushSample(0, exciterLeft[i]);
            rightDelayLine.pushSample(0, exciterRight[i]);
        }

        previousSampleL = 0.0f;
        previousSampleR = 0.0f;
    }
    
    // ============================= EXCITER GENERATOR =========================================

    // to-do: create two more buffers for separate exciter waveforms and blend in real time.
    void generateExciter(float currentVelocity, std::vector<float>& exciterL, std::vector<float>& exciterR)
    {
        exciterL.resize(baseExactDelayInt); // TESTING try using finetuned instead of plain delaySamples
        exciterR.resize(baseExactDelayInt);

        // pulse width scaling for square wave
        constexpr float velocityThreshold = 0.15f;
        float pulseWidth = 2 * juce::jlimit(0.01f, 1.0f, (currentVelocity - velocityThreshold) / (1.0f - velocityThreshold));
        float halfPeriod = baseExactDelayFrac * 0.5f; 
        float squareSample = 0.0f;
        float randomSampleL = 0.0f;
        float randomSampleR = 0.0f;
        float excitationL = 0.0f;
        float excitationR = 0.0f;

        for (int i = 0; i < baseExactDelayInt; ++i) // TESTING try using finetuned instead of plain delaySamples
        {
            // plain square wave, MUST fill exact karplus delay tune period!
            squareSample = (i < halfPeriod) ?
                (((float)i / halfPeriod < pulseWidth) ? plainSquareAmp : 0.0f) // plainSquareAmp directly sets gain
                : ((((float)(i - halfPeriod) / halfPeriod) < pulseWidth) ? -plainSquareAmp : 0.0f);

            // noise square wave, MUST fill exact karplus delay tune period! Apply pulse width
            float randomL = juce::Random::getSystemRandom().nextFloat();

            randomL *= (1 + noiseAmp); // TESTING -- boost the dynamic range of the noise before biasing

            randomSampleL = (i < halfPeriod) ?
                (((float)i / halfPeriod < pulseWidth) ? randomL - noiseBias : 0.0f)
                : ((((float)(i - halfPeriod) / halfPeriod) < pulseWidth) ? -(randomL - noiseBias) : 0.0f);

            excitationL = juce::jmap(currentColor, squareSample, randomSampleL) * currentVelocity;

            if (stereoEnabled)
            {
                // in stereo mode, each noise channel needs to be randomly generated independently    
                float randomR = juce::Random::getSystemRandom().nextFloat();

                randomR *= (1 + noiseAmp); // TESTING -- boost the dynamic range of the noise before biasing

                randomSampleR = (i < halfPeriod) ?
                    (((float)i / halfPeriod < pulseWidth) ? randomR - noiseBias : 0.0f)
                    : ((((float)(i - halfPeriod) / halfPeriod) < pulseWidth) ? -(randomR - noiseBias) : 0.0f);

                excitationR = juce::jmap(currentColor, squareSample, randomSampleR) * currentVelocity;
            }
            else
            {
                excitationR = excitationL; // in mono, just duplicate one of the exciters
            }

            exciterL[i] = excitationL;
            exciterR[i] = excitationR;
        }

        // for (int i = 0; i < baseExactDelayInt; ++i)
        // {
        //     // Generate base square wave pattern
        //     bool isPositiveHalf = (i < halfPeriod);
        //     bool isWithinPulseWidth = isPositiveHalf ? 
        //         ((float)i / halfPeriod < pulseWidth) : 
        //         (((float)(i - halfPeriod) / halfPeriod) < pulseWidth);
            
        //     // Base square amplitude (positive or negative)
        //     float baseSquareAmp = isWithinPulseWidth ? 
        //         (isPositiveHalf ? plainSquareAmp : -plainSquareAmp) : 0.0f;
            
        //     // Generate randomized version for left channel
        //     float randomFactorL = juce::Random::getSystemRandom().nextFloat();
        //     randomFactorL *= (1 + noiseAmp);
        //     float randomizedSquareL = isWithinPulseWidth ?
        //         (isPositiveHalf ? (randomFactorL - noiseBias) : -(randomFactorL - noiseBias)) : 0.0f;
            
        //     // Interpolate between square and randomized square based on color
        //     excitationL = juce::jmap(currentColor, baseSquareAmp, randomizedSquareL) * currentVelocity;
            
        //     if (stereoEnabled)
        //     {
        //         // Generate independent randomization for right channel
        //         float randomFactorR = juce::Random::getSystemRandom().nextFloat();
        //         randomFactorR *= (1 + noiseAmp);
        //         float randomizedSquareR = isWithinPulseWidth ?
        //             (isPositiveHalf ? (randomFactorR - noiseBias) : -(randomFactorR - noiseBias)) : 0.0f;
                
        //         excitationR = juce::jmap(currentColor, baseSquareAmp, randomizedSquareR) * currentVelocity;
        //     }
        //     else
        //     {
        //         excitationR = excitationL;
        //     }
            
        //     exciterL[i] = excitationL;
        //     exciterR[i] = excitationR;
        // }

        updateNoteTimer(currentMidiNote, currentVelocity);
    }

    void updateNoteTimer(int midiNoteNumber, float velocity)
    {
        float decayTimeSeconds = currentDecay;
        float dampCompensation = 1.0f - currentDamp * 0.618;

        double freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        double freqScale = 55.0 / freq;
        freqScale = juce::jlimit(0.01, 1.0, freqScale);

        float timeLimitScaler = 4.0f;

        maxSamplesAllowed = static_cast<int>(currentSampleRate * decayTimeSeconds * freqScale * dampCompensation * velocity * timeLimitScaler);
        activeSampleCounter = 0;
    }

    juce::LinearSmoothedValue<float> smoothedDelayLength; // juce's smoothing method for fixing finetune glitching

    // =============== MAIN PARAMETERS ======================================================
    float currentDamp = 0.0f; 
    float currentColor = 1.0f; // pulled from startnote and re-excite scheduler
    float currentVelocity = 1.0f;
    float currentDecay = 0.0f;
    float currentFineTuneMultiplier = 0.0f;
    float currentFineTuneCents = 0.0f;
    bool stereoEnabled = false;
    // bool filterMode = false; 
    float previousSampleL = 0.0f; 
    float previousSampleR = 0.0f;

    // =============== FILTER STUFF ==========================================================
    // float cutoffFreq = 20000.0f;
    float filteredSampleL, filteredSampleR; // these were in renderNextBlock before the loop
    // juce::dsp::StateVariableTPTFilter<float> lowpassFilterL;
    // juce::dsp::StateVariableTPTFilter<float> lowpassFilterR;

    // =================== VOICE SHAPING =====================================================
    float noiseBias = 0.3f;
    float noiseAmp = 0.0f;
    float plainSquareAmp = 0.45f;
    bool outputRoute = false;

    // =================== DELAY STUFF =======================================================
    float baseExactDelayFrac;  // most accurate delay period after finetuning, fractional.
    int baseExactDelayInt;     // rounded baseExactDelayFrac
    static constexpr int maxBufferSize = 8192; // no need for notes lower than c1
    static constexpr uint32_t maxBlockSize = 1024; // Reasonable max block size for prepare
    int activeSampleCounter = 0;
    int maxSamplesAllowed = 0; // initialize in startNote, sets note duration timer used in renderNextBlock
    double currentSampleRate = 44100.0; 
    float cyclesPerSecond = 440.0f;  // Calculated once per note (frequency-dependent)
    int delaySamples = 0;
    int bufferIndex = 0;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> leftDelayLine { maxBufferSize };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> rightDelayLine { maxBufferSize };

    // =================== NOTE MANAGEMENT ====================================================
    int currentMidiNote = -1;  // -1 means no note playing
    bool hasStartedNote = false;
    int pendingReExciteSample = -1;
    float pendingReExciteVelocity = 0.0f;
    bool fadeOut = false;
    int fadeCounter = 0;
    int reExciterIndex = -1;
    constexpr static float reExciteFactor = 0.5f;

    // initial exciter goes in this, timing may differ from subsequent re-exciters
    std::vector<float> leftDelayBuffer;
    std::vector<float> rightDelayBuffer;

    // For re-excitation -- not sure why this is needed since the same generator code is used
    std::vector<float> reExciterLeft;
    std::vector<float> reExciterRight;

    juce::AudioProcessorValueTreeState& apvts;
};