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
        
        // PRE-ALLOCATE EXCITER BUFFERS TO MAX SIZE - THIS FIXES THE CRASH
        exciterLeft.resize(maxBufferSize, 0.0f);
        exciterRight.resize(maxBufferSize, 0.0f);
        reExciterLeft.resize(maxBufferSize, 0.0f);
        reExciterRight.resize(maxBufferSize, 0.0f);
    }

    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return true;
    }

    bool isPlayingNote() const { return hasStartedNote; }
    int getCurrentlyPlayingNote() const { return currentMidiNote; }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        currentMidiNote = midiNoteNumber;
        
        // these need to be considered global for the lifetime of the voice
        setFineTuneCents(apvts.getRawParameterValue("FINETUNE")->load());
        setCurrentDecay(apvts.getRawParameterValue("DECAY")->load());
        setCurrentDamp(apvts.getRawParameterValue("DAMP")->load());
        setCurrentColor(apvts.getRawParameterValue("COLOR")->load());
        setStereoEnabled(apvts.getRawParameterValue("STEREO")->load());
        setStereoMicrotuneCents(apvts.getRawParameterValue("STEREOMICROTUNECENTS")->load());
        currentVelocity = velocity; 
        
        smoothedDelayLengthL.reset(currentSampleRate, 0.2);
        smoothedDelayLengthR.reset(currentSampleRate, 0.2);
        initializeDelayLineAndParameters(midiNoteNumber, currentVelocity);
        smoothedDelayLengthL.setCurrentAndTargetValue(baseExactDelayFracL);
        smoothedDelayLengthR.setCurrentAndTargetValue(baseExactDelayFracR);      

        hasStartedNote = true;
        reExciterIndexL = -1;
        reExciterIndexR = -1;
    }

    void stopNote(float, bool allowTailOff) override
    {
        if (gateEnabled && allowTailOff)
        {
            fadeOut = true;
            fadeCounter = 0;
            gateDampingSamples = static_cast<int>(currentSampleRate * apvts.getRawParameterValue("GATEDAMPING")->load());
        }
        // Remove the else clause - let notes decay naturally when gate is disabled
    }

    void setDelayTimes()
    {
        float stereoMicrotuneCached;

        if (stereoEnabled){
            stereoMicrotuneCached = stereoMicrotune;
        }
        else
        {
            stereoMicrotuneCached = 0.0f;
        }

        // Get tuning system deviation for this note
        float tuningDeviation = 0.0f;
        if (tuningSystem != nullptr)
        {
            tuningDeviation = tuningSystem->getCentDeviationForNote(currentMidiNote);
        }

        // Combine fine tune, stereo microtune, and tuning system deviation
        float totalCentsL = currentFineTuneCents - stereoMicrotuneCached + tuningDeviation;
        float totalCentsR = currentFineTuneCents + stereoMicrotuneCached + tuningDeviation;

        currentFineTuneMultiplierL = std::pow(2.0f, totalCentsL / 1200.0f);
        currentFineTuneMultiplierR = std::pow(2.0f, totalCentsR / 1200.0f);

        if (currentMidiNote >= 0 && currentSampleRate > 0.0)
        {
            const auto freq = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote);
            const float exactDelay = currentSampleRate / freq;

            baseExactDelayFracL = exactDelay / currentFineTuneMultiplierL;
            baseExactDelayFracR = exactDelay / currentFineTuneMultiplierR;

            // ADD SAFETY BOUNDS CHECK
            baseExactDelayIntL = static_cast<int>(std::round(baseExactDelayFracL));
            baseExactDelayIntL = juce::jlimit(1, maxBufferSize - 1, baseExactDelayIntL);
            baseExactDelayIntR = static_cast<int>(std::round(baseExactDelayFracR));
            baseExactDelayIntR = juce::jlimit(1, maxBufferSize - 1, baseExactDelayIntR);

            smoothedDelayLengthL.setCurrentAndTargetValue(baseExactDelayFracL);
            smoothedDelayLengthR.setCurrentAndTargetValue(baseExactDelayFracR);
        }
    }

    void clearCurrentNote()
    {
        hasStartedNote = false;
        currentMidiNote = -1;
        juce::SynthesiserVoice::clearCurrentNote();
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>& getLeftDelayLine() { return leftDelayLine; }
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>& getRightDelayLine() { return rightDelayLine; }
    juce::LinearSmoothedValue<float>& getSmoothedDelayL() { return smoothedDelayLengthL; }
    juce::LinearSmoothedValue<float>& getSmoothedDelayR() { return smoothedDelayLengthR; }

    static constexpr int getMaxBufferSize() { return maxBufferSize; }

    // ============================== RE EXCITER ====================================
    void setReExciterIndexL(int indexL) { reExciterIndexL = indexL; }
    void setReExciterIndexR(int indexR) { reExciterIndexR = indexR; }
    std::vector<float>& getReExciterLeft() { return reExciterLeft; }
    std::vector<float>& getReExciterRight() { return reExciterRight; }

    void scheduleReExcite(int sampleOffset, float velocity)
    {
        pendingReExciteSample = sampleOffset;
        currentVelocity = velocity;
    }

    void reExcite()
    {
        setFineTuneCents(apvts.getRawParameterValue("FINETUNE")->load());
        setCurrentDecay(apvts.getRawParameterValue("DECAY")->load());
        setCurrentDamp(apvts.getRawParameterValue("DAMP")->load());
        setCurrentColor(apvts.getRawParameterValue("COLOR")->load());
        setStereoEnabled(apvts.getRawParameterValue("STEREO")->load());
        setStereoMicrotuneCents(apvts.getRawParameterValue("STEREOMICROTUNECENTS")->load());

        setDelayTimes();
        
        generateExciter(currentVelocity, reExciterLeft, reExciterRight);
        
        smoothedDelayLengthL.reset(currentSampleRate, 0.2); // less glitchy reaction to finetune
        smoothedDelayLengthR.reset(currentSampleRate, 0.2); // less glitchy reaction to finetune
        smoothedDelayLengthL.setCurrentAndTargetValue(baseExactDelayFracL);
        smoothedDelayLengthR.setCurrentAndTargetValue(baseExactDelayFracR);

        reExciterIndexL = 0;
        reExciterIndexR = 0;
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

    // =============================== DSP LOOP ===============================
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (baseExactDelayIntL < 1 || baseExactDelayIntR < 1 || !hasStartedNote)
            return;

        juce::ScopedNoDenormals noDenormals;

        const float targetAmplitude = 0.001f;

        // 1.5f multiplier is used to try and match the expected decay time at a reasonable note range
        float feedbackGain = std::pow(targetAmplitude, 1.0f / (1.5f * currentDecay * cyclesPerSecondL));
        feedbackGain = juce::jlimit(0.0f, 0.999f, feedbackGain);

        auto* outL = outputBuffer.getWritePointer(0, startSample);
        auto* outR = (outputBuffer.getNumChannels() >= 2) ? outputBuffer.getWritePointer(1, startSample) : outL;

        float currentDelayValueL = smoothedDelayLengthL.getCurrentValue();
        float currentDelayValueR = smoothedDelayLengthR.getCurrentValue();

        // my delay calc is robust enough to not require jlimiting it. otherwise fix it there.
        // constexpr float minDelayTime = 0.01f; // or smaller, but > 0 -- VERY unlikely, probably impossible.
        // float clampedDelay = std::max(minDelayTime, currentDelayValue); // my lowest note won't hit max buffer size.
        leftDelayLine.setDelay(currentDelayValueL);
        rightDelayLine.setDelay(currentDelayValueR);

        for (int i = 0; i < numSamples; ++i)
        {
            if (pendingReExciteSample == (startSample + i))
            {
                reExcite();
                pendingReExciteSample = -1;
            }

            float delayedSampleL = leftDelayLine.popSample(0);
            float delayedSampleR = rightDelayLine.popSample(0);

            float dampingAmount;
            dampingAmount = juce::jmap(currentDamp, 0.0f, 1.0f, 0.99f, 0.01f);

            filteredSampleL = previousSampleL + dampingAmount * (delayedSampleL - previousSampleL);
            filteredSampleR = previousSampleR + dampingAmount * (delayedSampleR - previousSampleR);

            float addL = 0.0f;
            float addR = 0.0f;

            // optionally, set minimum velo
            // float mappedValue = minimumExciterVelocity + velocity * (1.0f - minimumExciterVelocity);

            // SAFE ACCESS TO EXCITER BUFFERS
            // inject exciter(s) into the delayline
            // if (activeSampleCounter >= 0 && activeSampleCounter < baseExactDelayInt && activeSampleCounter < currentExciterSize)
            if (activeSampleCounter < currentDelayValueL && activeSampleCounter <= currentExciterSizeL)
            {
                addL += exciterLeft[activeSampleCounter] * currentVelocity;
            }
            if (activeSampleCounter < currentDelayValueR && activeSampleCounter <= currentExciterSizeR)
            {
                addR += exciterRight[activeSampleCounter] * currentVelocity;
            }
            if (reExciterIndexL >= 0 && reExciterIndexL < baseExactDelayIntL && reExciterIndexL < currentExciterSizeL)
            {
                addL += reExciterLeft[reExciterIndexL] * currentVelocity;
                ++reExciterIndexL;
                if (reExciterIndexL >= baseExactDelayIntL)
                    reExciterIndexL = -1;
            }
            if (reExciterIndexR >= 0 && reExciterIndexR < baseExactDelayIntR && reExciterIndexR < currentExciterSizeR)
            {
                addR += reExciterRight[reExciterIndexR] * currentVelocity;
                ++reExciterIndexR;
                if (reExciterIndexR >= baseExactDelayIntR)
                    reExciterIndexR = -1;
            }

            filteredSampleL += addL;
            filteredSampleR += addR;

            if (!fadeOut && activeSampleCounter >= maxSamplesAllowed)
            {
                fadeOut = true;
                fadeCounter = 0;
            }

            if (fadeOut)
            {
                fadeCounter++;
                
                // Use the existing GATEDAMPING parameter for fadeout time
                float fadeoutTime = apvts.getRawParameterValue("GATEDAMPING")->load();
                int fadeoutSamples = static_cast<int>(currentSampleRate * fadeoutTime);
                
                // If fadeout time is 0, use the original 64 samples as fallback
                if (fadeoutSamples <= 0)
                    fadeoutSamples = 64;
                
                float fadeMultiplier = std::max(0.0f, 1.0f - (float)fadeCounter / (float)fadeoutSamples);
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

            previousSampleL = outputL;
            previousSampleR = outputR;

            outL[i] += outputL;
            outR[i] += outputR;

            ++activeSampleCounter;
        }
    }

    // ====================== PARAMETER SETTERS ==============================================
    // void setFineTuneCents(float newFineTuneCents)
    // {
    //     if (auto* param = apvts.getRawParameterValue("FINETUNE"))
    //         currentFineTuneCents = param->load();
    //     else
    //         currentFineTuneCents = 0.0f;
    // }

    void setFineTuneCents(float newFineTuneCents)
    {
        if (newFineTuneCents != currentFineTuneCents)
        {
            currentFineTuneCents = newFineTuneCents;

            // Recalculate delay times based on new finetune
            setDelayTimes();

            // Update smoothing targets (do NOT reset smoother)
            smoothedDelayLengthL.setTargetValue(baseExactDelayFracL);
            smoothedDelayLengthR.setTargetValue(baseExactDelayFracR);
        }
    }

    void setTuningSystem(const TuningSystem* tuning) 
    { 
        tuningSystem = tuning; 
    }

    void setCurrentDecay(float newDecay)
    {
        currentDecay = newDecay;
    }

    void setCurrentDamp(float newDamp)
    {
        currentDamp = newDamp;
    }

    void setCurrentColor(float newColor)
    {
        currentColor = newColor;
    }

    void setGateEnabled(bool enabled)
    {
        gateEnabled = enabled;
    }

    void setStereoEnabled(bool enabled)
    {
        if (stereoEnabled != enabled)
        {
            stereoEnabled = enabled;
            if (stereoEnabled)
            {
                std::fill(leftDelayBuffer.begin(), leftDelayBuffer.end(), 0.f);
                std::fill(rightDelayBuffer.begin(), rightDelayBuffer.end(), 0.f);
            }
            else
            {
                std::fill(leftDelayBuffer.begin(), leftDelayBuffer.end(), 0.f);
                std::fill(rightDelayBuffer.begin(), rightDelayBuffer.end(), 0.f);
            }
            bufferIndex = 0;
        }
    }

    void setStereoMicrotuneCents(float newStereoMicrotuneCents)
    {
        stereoMicrotune = newStereoMicrotuneCents;
    }

    void setNoiseAmp(float newNoiseAmp)
    {
        noiseAmp = newNoiseAmp;
    }

    void setCurrentExciterSlewRate(float newExciterSlewRate)
    {
        currentExciterSlewRate = newExciterSlewRate;
    }

    void resetBuffers()
    {
        std::fill(leftDelayBuffer.begin(), leftDelayBuffer.end(), 0.0f);
        std::fill(rightDelayBuffer.begin(), rightDelayBuffer.end(), 0.0f);
        
        // CLEAR EXCITER BUFFERS SAFELY
        std::fill(exciterLeft.begin(), exciterLeft.end(), 0.0f);
        std::fill(exciterRight.begin(), exciterRight.end(), 0.0f);
        std::fill(reExciterLeft.begin(), reExciterLeft.end(), 0.0f);
        std::fill(reExciterRight.begin(), reExciterRight.end(), 0.0f);
        
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
        reExciterIndexL = -1;
        reExciterIndexR = -1;
    }

private:
    void initializeDelayLineAndParameters(int midiNoteNumber, float velocity)
    {
        setDelayTimes();
        cyclesPerSecondL = currentSampleRate / baseExactDelayFracL;
        cyclesPerSecondR = currentSampleRate / baseExactDelayFracR;
        bufferIndex = 0;

        generateExciter(velocity, exciterLeft, exciterRight);

        leftDelayLine.reset();
        rightDelayLine.reset();
        leftDelayLine.setDelay(0);
        rightDelayLine.setDelay(0);

        fadeOut = false;
        fadeCounter = 0;
        previousSampleL = 0.0f;
        previousSampleR = 0.0f;
    }

    // FIXED EXCITER GENERATOR - NO MORE DYNAMIC RESIZING
    void generateExciter(float currentVelocity, std::vector<float>& exciterL, std::vector<float>& exciterR)
    {
        // NO MORE RESIZE! Buffers are pre-allocated to maxBufferSize
        // Just clear the portion we'll use
        int safeDelayIntL = juce::jlimit(1, maxBufferSize - 1, baseExactDelayIntL);
        int safeDelayIntR = juce::jlimit(1, maxBufferSize - 1, baseExactDelayIntR);  

        // Clear only the range we'll be using
        std::fill(exciterL.begin(), exciterL.begin() + safeDelayIntL, 0.0f);
        std::fill(exciterR.begin(), exciterR.begin() + safeDelayIntR, 0.0f);

        float pulseWidth = 2 * juce::jlimit(0.01f, 1.0f, (currentVelocity - minimumExciterVelocity) / (1.0f - minimumExciterVelocity));
        float halfPeriodL = baseExactDelayFracL * 0.5f;
        float halfPeriodR = baseExactDelayFracR * 0.5f;

        // for (int i = 0; i < safeDelayIntL; ++i)
        // {
        //     float squareSample = (i < halfPeriodL) ?
        //         (((float)i / halfPeriodL < pulseWidth) ? plainSquareAmp : 0.0f)
        //         : ((((float)(i - halfPeriodL) / halfPeriodL) < pulseWidth) ? -plainSquareAmp : 0.0f);

        //     float randomL = juce::Random::getSystemRandom().nextFloat();
        //     float noiseSampleL = ((float)i / safeDelayIntL < pulseWidth) ? (randomL * 2.0f - 1.0f) : 0.0f;
        //     float excitationL = juce::jmap(currentColor, squareSample, noiseSampleL);
        //     exciterL[i] = excitationL; 
        // }
        for (int i = 0; i < safeDelayIntL; ++i)
        {
            float squareSample = (i < halfPeriodL) ?
                (((float)i / halfPeriodL < pulseWidth) ? plainSquareAmp : 0.0f)
                : ((((float)(i - halfPeriodL) / halfPeriodL) < pulseWidth) ? -plainSquareAmp : 0.0f);

            float randomL = juce::Random::getSystemRandom().nextFloat();
            float noiseSampleL = ((float)i / safeDelayIntL < pulseWidth) ? (randomL * 2.0f - 1.0f) : 0.0f;

            // Apply slew limiting to noiseSampleL:
            float alpha = juce::jlimit(0.0f, 1.0f, currentExciterSlewRate); // ensure within [0,1]
            float slewedNoiseL = alpha * noiseSampleL + (1.0f - alpha) * prevNoiseL;
            prevNoiseL = slewedNoiseL;

            float excitationL = juce::jmap(currentColor, squareSample, slewedNoiseL);
            exciterL[i] = excitationL;
        }

        if (stereoEnabled)

        {
            for (int i = 0; i < safeDelayIntR; ++i)
            {
                float squareSample = (i < halfPeriodR) ?
                    (((float)i / halfPeriodR < pulseWidth) ? plainSquareAmp : 0.0f)
                    : ((((float)(i - halfPeriodR) / halfPeriodR) < pulseWidth) ? -plainSquareAmp : 0.0f);

                float randomR = juce::Random::getSystemRandom().nextFloat();
                float noiseSampleR = ((float)i / safeDelayIntR < pulseWidth) ? (randomR * 2.0f - 1.0f) : 0.0f;

                // Apply slew limiting to noiseSampleL:
                float alpha = juce::jlimit(0.0f, 1.0f, currentExciterSlewRate); // ensure within [0,1]
                float slewedNoiseR = alpha * noiseSampleR + (1.0f - alpha) * prevNoiseR;
                prevNoiseR = slewedNoiseR;

                float excitationR = juce::jmap(currentColor, squareSample, slewedNoiseR);
                exciterR[i] = excitationR;
            }
        }
        else
        {
            exciterR = exciterL;
        }
        
        currentExciterSizeL = exciterL.size(); // used in renderNextBlock
        currentExciterSizeR = exciterR.size(); // used in renderNextBlock

        updateNoteTimer(currentMidiNote, currentVelocity);
    }

    void updateNoteTimer(int midiNoteNumber, float velocity)
    {
        float minNote = 24.0f;
        float maxNote = 108.0f;
        float ratio = 60.0f;
        float clampedNote = juce::jlimit(minNote, maxNote, static_cast<float>(currentMidiNote));

        float decayMultiplier = std::pow(
            ratio,
            (maxNote - clampedNote) / (maxNote - minNote)
        );

        float baseDecayTime = juce::jlimit(0.05f, 60.0f, currentDecay);
        float normalizedDamp = juce::jmap(currentDamp, 0.0f, 0.65f, 0.0f, 1.0f);
        float dampMultiplier = juce::jmap(normalizedDamp, 0.0f, 1.0f, 1.0f, 0.5f);
        float totalDecayTime = baseDecayTime * dampMultiplier * decayMultiplier * 0.25f;

        maxSamplesAllowed = static_cast<int>(currentSampleRate * totalDecayTime);
        activeSampleCounter = 0;
    }

    juce::LinearSmoothedValue<float> smoothedDelayLengthL;
    juce::LinearSmoothedValue<float> smoothedDelayLengthR;

    // PRIVATE MEMBERS
    float currentDamp = 0.0f;
    float currentColor = 1.0f;
    float currentVelocity = 1.0f;
    float currentDecay = 0.0f;
    float currentFineTuneMultiplierL;
    float currentFineTuneMultiplierR;
    float currentFineTuneCents = 0.0f;
    bool stereoEnabled = false;
    float stereoMicrotune = 0.0f;

    float previousSampleL = 0.0f;
    float previousSampleR = 0.0f;
    float filteredSampleL, filteredSampleR;

    float noiseBias = 0.3f;
    float noiseAmp = 0.0f;
    float plainSquareAmp = 0.43f;
    float minimumExciterVelocity = 0.1f;
    
    // PRE-ALLOCATED EXCITER BUFFERS - NO MORE DYNAMIC RESIZING
    std::vector<float> exciterLeft, exciterRight;
    int currentExciterSizeL;
    int currentExciterSizeR;

    float baseExactDelayFracL;
    float baseExactDelayFracR;
    int baseExactDelayIntL;
    int baseExactDelayIntR;
    const TuningSystem* tuningSystem = nullptr;
    
    static constexpr int maxBufferSize = 8192;
    static constexpr uint32_t maxBlockSize = 1024;
    int activeSampleCounter = 0;
    int maxSamplesAllowed = 0;
    double currentSampleRate = 44100.0;
    float cyclesPerSecondL = 440.0f;
    float cyclesPerSecondR = 440.0f; 

    int bufferIndex = 0;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> leftDelayLine { maxBufferSize };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> rightDelayLine { maxBufferSize };

    int currentMidiNote = -1;
    bool hasStartedNote = false;
    int pendingReExciteSample = -1;
    float pendingReExciteVelocity = 0.0f;

    bool gateEnabled = false;
    int gateDampingSamples = 0;
    bool fadeOut = false;
    int fadeCounter = 0;
    
    int reExciterIndexL = -1;
    int reExciterIndexR = -1;
    constexpr static float reExciteFactor = 0.5f;
    float prevNoiseL = 0.0f;
    float prevNoiseR = 0.0f;
    float currentExciterSlewRate = 1.0f;

    std::vector<float> leftDelayBuffer;
    std::vector<float> rightDelayBuffer;
    std::vector<float> reExciterLeft;
    std::vector<float> reExciterRight;

    juce::AudioProcessorValueTreeState& apvts;
};