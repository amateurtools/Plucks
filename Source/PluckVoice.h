#pragma once
class PluckVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override
    {
        return true;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override
    {
        auto freq = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        auto sampleRate = getSampleRate();
        delaySamples = std::clamp(static_cast<int>(sampleRate / freq), 1, maxBufferSize - 1);

        
        bufferIndex = 0;
        delayBuffer.clear();
        delayBuffer.resize(maxBufferSize, 0.0f);

        for (int i = 0; i < delaySamples; ++i)
            delayBuffer[i] = (juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f) * velocity;

        previousSample = 0.0f;
        
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = getSampleRate();
        spec.maximumBlockSize = 1; // one sample at a time
        spec.numChannels = 1;
        colorFilter.prepare(spec);
        colorFilter.reset();

        leftDelayLine.reset();
        rightDelayLine.reset();

        leftDelayLine.setDelay(0.0f);
        rightDelayLine.setDelay(30.0f); // 5ms delay for widening

        leftDelayLine.prepare(spec);
        rightDelayLine.prepare(spec);
    }

    void stopNote(float, bool) override
    {
        if (gateEnabled)
        {
            std::fill(delayBuffer.begin(), delayBuffer.end(), 0.0f); // Mute the string
            previousSample = 0.0f;
        }

        clearCurrentNote();
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (delaySamples < 1 || delayBuffer.empty())
            return;

        for (int i = 0; i < numSamples; ++i)
        {
            bufferIndex %= delaySamples;

            auto current = delayBuffer[bufferIndex];
            auto next = delayBuffer[(bufferIndex + 1) % delaySamples];
            auto avg = 0.5f * (current + next);

            // Apply damping
            float damped = previousSample + damping * (avg - previousSample);
            // Apply decay compensation
            float decayed = damped * decayCompensation;

            // Handle NaNs or infs
            if (!std::isfinite(decayed))
                decayed = 0.0f;

            float filtered = 0.5f * (decayed + colorFilter.processSample(decayed));

            delayBuffer[bufferIndex] = filtered;
            previousSample = filtered;

            if (stereoEnabled && outputBuffer.getNumChannels() >= 2)
            {
                // Left = direct signal
                float leftSample = filtered;

                // Right = delayed version
                float rightSample = rightDelayLine.popSample(0);

                outputBuffer.addSample(0, startSample + i, leftSample);
                outputBuffer.addSample(1, startSample + i, rightSample);

                // Push new signal into the delay line *after* sampling
                rightDelayLine.pushSample(0, filtered);
            }
            else
            {
                for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                    outputBuffer.addSample(ch, startSample + i, filtered);
            }

            ++bufferIndex;
        }
    }

    void setParameters(float decay, float damp, float color)
    {
        decayTimeSeconds = std::pow(10.0f, juce::jmap(decay, 0.0f, 1.0f, -4.0f, std::log10(2.0f))); // maps to 0.0001 to 2.0 seconds
        damping = juce::jmap(damp, 0.0f, 1.0f, 0.99f, 0.01f);

        // Apply decay compensation based on loop rate
        decayCompensation = std::pow(0.001f, 1.0f / (decayTimeSeconds * getSampleRate()));
        
        float colorFreq = juce::jmap(color, 0.0f, 1.0f, 20.0f, 12000.0f); // center frequency of the peak
        float q = 0.01;

        colorFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(getSampleRate(), colorFreq, q);
    }
    
    void setGateEnabled(bool enabled)
    {
        gateEnabled = enabled;
    }
    
    void setStereoEnabled(bool enabled)
    {
        stereoEnabled = enabled;
    }

private:
    std::vector<float> delayBuffer;
    int delaySamples = 0;
    int bufferIndex = 0;
    float feedbackGain = 0.97f;
    float damping = 0.5f;
    float previousSample = 0.0f;
    static constexpr int maxBufferSize = 44100;
    float decayCompensation = 1.0f;
    float decayTimeSeconds = 0.0f;
    bool gateEnabled = false;
    bool stereoEnabled = false;
    
    juce::dsp::IIR::Filter<float> colorFilter;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> leftDelayLine { 4410 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> rightDelayLine { 4410 };
};
