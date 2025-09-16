#pragma once
#include <cmath>

class StringBodyCoupling
{
public:
    void prepare(float sampleRate)
    {
        fs = sampleRate;
        
        // Simple 200Hz resonator
        float freq = 200.0f;
        float Q = 8.0f;
        float omega = 2.0f * float(M_PI) * freq / sampleRate;
        float alpha = std::sin(omega) / (2.0f * Q);
        
        float a0 = 1.0f + alpha;
        b0 = alpha / a0;
        b1 = 0.0f;
        b2 = -alpha / a0;
        a1 = -2.0f * std::cos(omega) / a0;
        a2 = (1.0f - alpha) / a0;
        
        reset();
    }

    explicit StringBodyCoupling(float sampleRate) : fs(sampleRate)
    {
        prepare(sampleRate);
    }

    void reset()
    {
        x1 = x2 = y1 = y2 = 0.0f;
    }

    void setStringFrequency(float freq) {}
    void setSoundholeProximity(float proximity) {}

    float process(float input, float couplingStrength)
    {
        // Simple bandpass filter
        float output = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        
        // Update states
        x2 = x1; x1 = input;
        y2 = y1; y1 = output;
        
        // Scale the resonant output and mix
        float resonantPart = output * 3.0f; // Make it obvious
        return input + (couplingStrength * resonantPart);
    }

private:
    float fs;
    float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
};