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
    }

    void stopNote(float, bool) override
    {
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

            // Apply color filter
            float filtered = colorFilter.processSample(decayed);

            delayBuffer[bufferIndex] = filtered;
            previousSample = filtered;

            for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch)
                outputBuffer.addSample(ch, startSample + i, filtered);

            ++bufferIndex;
        }
    }

    void setParameters(float decay, float damp, float color)
    {
        decayTimeSeconds = std::pow(10.0f, juce::jmap(decay, 0.0f, 1.0f, -4.0f, std::log10(5.0f))); // maps to 0.0001 to 5.0 seconds
        damping = juce::jmap(damp, 0.0f, 1.0f, 0.01f, 0.99f);

        // Apply decay compensation based on loop rate
        decayCompensation = std::pow(0.001f, 1.0f / (decayTimeSeconds * getSampleRate()));
        
        float colorFreq = juce::jmap(color, 0.0f, 1.0f, 200.0f, 12000.0f); // center frequency of the peak
        float q = 0.5;

        colorFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), colorFreq, q, 1.05f);
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
    
    juce::dsp::IIR::Filter<float> colorFilter;
};
