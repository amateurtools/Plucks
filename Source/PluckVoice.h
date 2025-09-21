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
        
        // these need to be considered global
        setFineTuneCents(apvts.getRawParameterValue("FINETUNE")->load());
        setCurrentDecay(apvts.getRawParameterValue("DECAY")->load());
        setCurrentDamp(apvts.getRawParameterValue("DAMP")->load());
        setCurrentColor(apvts.getRawParameterValue("COLOR")->load());
        setStereoEnabled(apvts.getRawParameterValue("STEREO")->load());
        
        // float mappedValue = minimumExciterVelocity + velocity * (1.0f - minimumExciterVelocity);
        currentVelocity = velocity; 
        
        smoothedDelayLength.reset(currentSampleRate, 0.02);
        initializeDelayLineAndParameters(midiNoteNumber, currentVelocity);
        smoothedDelayLength.setCurrentAndTargetValue(baseExactDelayFrac);
        
        hasStartedNote = true;
        reExciterIndex = -1;
    }

    void stopNote(float, bool allowTailOff) override
    {
        bool gateEnabled = apvts.getRawParameterValue("GATE")->load() > 0.5f;
        if (gateEnabled && allowTailOff)
        {
            fadeOut = true;
            fadeCounter = 0;
        }
    }

    void setDelayTimes()
    {
        currentFineTuneMultiplier = std::pow(2.0f, currentFineTuneCents / 1200.0f);
        if (currentMidiNote >= 0 && currentSampleRate > 0.0)
        {
            const auto freq = juce::MidiMessage::getMidiNoteInHertz(currentMidiNote);
            const float exactDelay = currentSampleRate / freq;
            baseExactDelayFrac = exactDelay / currentFineTuneMultiplier;
            
            // ADD SAFETY BOUNDS CHECK
            baseExactDelayInt = static_cast<int>(std::round(baseExactDelayFrac));
            baseExactDelayInt = juce::jlimit(1, maxBufferSize - 1, baseExactDelayInt);
            
            smoothedDelayLength.setCurrentAndTargetValue(baseExactDelayFrac);
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

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>& getLeftDelayLine() { return leftDelayLine; }
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>& getRightDelayLine() { return rightDelayLine; }
    juce::LinearSmoothedValue<float>& getSmoothedDelay() { return smoothedDelayLength; }
    static constexpr int getMaxBufferSize() { return maxBufferSize; }

    // ============================== RE EXCITER ====================================
    void setReExciterIndex(int index) { reExciterIndex = index; }
    std::vector<float>& getReExciterLeft() { return reExciterLeft; }
    std::vector<float>& getReExciterRight() { return reExciterRight; }

    void scheduleReExcite(int sampleOffset, float velocity)
    {
        pendingReExciteSample = sampleOffset;
        // float mappedValue = minimumExciterVelocity + velocity * (1.0f - minimumExciterVelocity);
        // pendingReExciteVelocity = velocity;
        currentVelocity = velocity;
    }

    void reExcite()
    {
        // float reExciteVelocity = pendingReExciteVelocity;
        
        setFineTuneCents(apvts.getRawParameterValue("FINETUNE")->load());
        setCurrentDecay(apvts.getRawParameterValue("DECAY")->load());
        setCurrentDamp(apvts.getRawParameterValue("DAMP")->load());
        setCurrentColor(apvts.getRawParameterValue("COLOR")->load());
        setStereoEnabled(apvts.getRawParameterValue("STEREO")->load());
        setDelayTimes();
        
        generateExciter(currentVelocity, reExciterLeft, reExciterRight);
        
        smoothedDelayLength.reset(currentSampleRate, 0.001);
        smoothedDelayLength.setCurrentAndTargetValue(baseExactDelayFrac);
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

    // =============================== DSP LOOP ===============================
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (baseExactDelayInt < 1 || !hasStartedNote)
            return;

        juce::ScopedNoDenormals noDenormals;

        float decayTimeSeconds = currentDecay;
        const float targetAmplitude = 0.001f;
        float feedbackGain = std::pow(targetAmplitude, 1.0f / (1.7f * decayTimeSeconds * cyclesPerSecond));
        feedbackGain = juce::jlimit(0.0f, 0.999f, feedbackGain);

        auto* outL = outputBuffer.getWritePointer(0, startSample);
        auto* outR = (outputBuffer.getNumChannels() >= 2) ? outputBuffer.getWritePointer(1, startSample) : outL;

        for (int i = 0; i < numSamples; ++i)
        {
            if (pendingReExciteSample == (startSample + i))
            {
                reExcite();
                pendingReExciteSample = -1;
            }

            float delayThisSample = juce::jlimit(1.0f, static_cast<float>(maxBufferSize), smoothedDelayLength.getNextValue());
            leftDelayLine.setDelay(delayThisSample);
            rightDelayLine.setDelay(delayThisSample);

            float delayedSampleL = leftDelayLine.popSample(0);
            float delayedSampleR = rightDelayLine.popSample(0);

            float dampingAmount;
            dampingAmount = juce::jmap(currentDamp, 0.0f, 1.0f, 0.99f, 0.01f);

            filteredSampleL = previousSampleL + dampingAmount * (delayedSampleL - previousSampleL);
            filteredSampleR = previousSampleR + dampingAmount * (delayedSampleR - previousSampleR);

            float addL = 0.0f;
            float addR = 0.0f;

            // SAFE ACCESS TO EXCITER BUFFERS
            if (activeSampleCounter >= 0 && activeSampleCounter < baseExactDelayInt && activeSampleCounter < static_cast<int>(exciterLeft.size()))
            {
                addL += exciterLeft[activeSampleCounter] * currentVelocity;
                addR += exciterRight[activeSampleCounter] * currentVelocity;
            }

            if (reExciterIndex >= 0 && reExciterIndex < baseExactDelayInt && reExciterIndex < static_cast<int>(reExciterLeft.size()))
            {
                addL += reExciterLeft[reExciterIndex] * currentVelocity;
                addR += reExciterRight[reExciterIndex] * currentVelocity;
                ++reExciterIndex;
                if (reExciterIndex >= baseExactDelayInt)
                    reExciterIndex = -1;
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

            float feedbackGainValue;
            if (activeSampleCounter < baseExactDelayInt)
            {
                feedbackGainValue = 1.0f;
            }
            else
            {
                feedbackGainValue = feedbackGain;
            }

            float outputL = filteredSampleL * feedbackGainValue;
            float outputR = filteredSampleR * feedbackGainValue;

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
    void setFineTuneCents(float newFineTuneCents)
    {
        if (auto* param = apvts.getRawParameterValue("FINETUNE"))
            currentFineTuneCents = param->load();
        else
            currentFineTuneCents = 0.0f;
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

    void setNoiseAmp(float newNoiseAmp)
    {
        noiseAmp = newNoiseAmp;
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
        reExciterIndex = -1;
    }

private:
    void initializeDelayLineAndParameters(int midiNoteNumber, float velocity)
    {
        setDelayTimes();
        cyclesPerSecond = currentSampleRate / baseExactDelayFrac;
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
        int safeDelayInt = juce::jlimit(1, maxBufferSize - 1, baseExactDelayInt);
        
        // Clear only the range we'll be using
        std::fill(exciterL.begin(), exciterL.begin() + safeDelayInt, 0.0f);
        std::fill(exciterR.begin(), exciterR.begin() + safeDelayInt, 0.0f);

        // constexpr float velocityThreshold = 0.1f;
        float pulseWidth = 2 * juce::jlimit(0.01f, 1.0f, (currentVelocity - minimumExciterVelocity) / (1.0f - minimumExciterVelocity));
        float halfPeriod = baseExactDelayFrac * 0.5f;
        
        for (int i = 0; i < safeDelayInt; ++i)
        {
            float squareSample = (i < halfPeriod) ?
                (((float)i / halfPeriod < pulseWidth) ? plainSquareAmp : 0.0f)
                : ((((float)(i - halfPeriod) / halfPeriod) < pulseWidth) ? -plainSquareAmp : 0.0f);

            float randomL = juce::Random::getSystemRandom().nextFloat();
            float noiseSampleL = ((float)i / safeDelayInt < pulseWidth) ? (randomL * 2.0f - 1.0f) : 0.0f;
            float excitationL = juce::jmap(currentColor, squareSample, noiseSampleL); // * currentVelocity; -- move to injection point instead

            if (stereoEnabled)
            {
                float randomR = juce::Random::getSystemRandom().nextFloat();
                float noiseSampleR = ((float)i / safeDelayInt < pulseWidth) ? (randomR * 2.0f - 1.0f) : 0.0f;
                exciterL[i] = excitationL;
                exciterR[i] = juce::jmap(currentColor, squareSample, noiseSampleR); // * currentVelocity;
            }
            else
            {
                exciterL[i] = excitationL;
                exciterR[i] = excitationL;
            }
        }

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

    juce::LinearSmoothedValue<float> smoothedDelayLength;

    // PRIVATE MEMBERS
    float currentDamp = 0.0f;
    float currentColor = 1.0f;
    float currentVelocity = 1.0f;
    float currentDecay = 0.0f;
    float currentFineTuneMultiplier = 0.0f;
    float currentFineTuneCents = 0.0f;
    bool stereoEnabled = false;

    float previousSampleL = 0.0f;
    float previousSampleR = 0.0f;
    float filteredSampleL, filteredSampleR;

    float noiseBias = 0.3f;
    float noiseAmp = 0.0f;
    float plainSquareAmp = 0.43f;
    float minimumExciterVelocity = 0.1f;
    
    // PRE-ALLOCATED EXCITER BUFFERS - NO MORE DYNAMIC RESIZING
    std::vector<float> exciterLeft, exciterRight;

    float baseExactDelayFrac;
    int baseExactDelayInt;
    static constexpr int maxBufferSize = 8192;
    static constexpr uint32_t maxBlockSize = 1024;
    int activeSampleCounter = 0;
    int maxSamplesAllowed = 0;
    double currentSampleRate = 44100.0;
    float cyclesPerSecond = 440.0f;
    int delaySamples = 0;
    int bufferIndex = 0;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> leftDelayLine { maxBufferSize };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> rightDelayLine { maxBufferSize };

    int currentMidiNote = -1;
    bool hasStartedNote = false;
    int pendingReExciteSample = -1;
    float pendingReExciteVelocity = 0.0f;
    bool fadeOut = false;
    int fadeCounter = 0;
    int reExciterIndex = -1;
    constexpr static float reExciteFactor = 0.5f;

    std::vector<float> leftDelayBuffer;
    std::vector<float> rightDelayBuffer;
    std::vector<float> reExciterLeft;
    std::vector<float> reExciterRight;

    juce::AudioProcessorValueTreeState& apvts;
};