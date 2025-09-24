// TuningSystem.h
#pragma once
#include <JuceHeader.h>
#include <array>
#include <string>

class TuningSystem
{
public:
    TuningSystem() 
    {
        // Initialize with equal temperament (all zeros = no deviation)
        resetToEqualTemperament();
    }
    
    // Load .tun file (Scala format or simple cent deviations)
    bool loadTuningFile(const juce::File& file);
    
    // Get cent deviation for a MIDI note (0-127)
    float getCentDeviationForNote(int midiNote) const;
    
    // Preset tunings
    void setWellTemperament();
    void setJustIntonation();
    void setPythagorean();
    void setMeantone();
    void resetToEqualTemperament();
    
    // Get tuning name for UI
    const juce::String& getCurrentTuningName() const { return currentTuningName; }
    
    // Check if custom tuning is loaded
    bool hasCustomTuning() const { return !currentTuningName.isEmpty(); }

private:
    // Store cent deviations for each of the 12 pitch classes
    std::array<float, 12> centDeviations;
    juce::String currentTuningName;
    
    void setCentDeviations(const std::array<float, 12>& deviations, const juce::String& name);
};

// TuningSystem.cpp implementation
inline bool TuningSystem::loadTuningFile(const juce::File& file)
{
    if (!file.exists())
        return false;
        
    auto content = file.loadFileAsString();
    auto lines = juce::StringArray::fromLines(content);
    
    std::array<float, 12> newDeviations = {};
    int noteIndex = 0;
    
    for (const auto& line : lines)
    {
        auto trimmed = line.trim();
        
        // Skip comments and empty lines
        if (trimmed.isEmpty() || trimmed.startsWith("!") || trimmed.startsWith("#"))
            continue;
            
        // Parse cent value
        float centValue = trimmed.getFloatValue();
        
        if (noteIndex < 12)
        {
            newDeviations[noteIndex] = centValue;
            noteIndex++;
        }
    }
    
    if (noteIndex >= 12)
    {
        setCentDeviations(newDeviations, file.getFileNameWithoutExtension());
        return true;
    }
    
    return false;
}

inline float TuningSystem::getCentDeviationForNote(int midiNote) const
{
    if (midiNote < 0 || midiNote > 127)
        return 0.0f;
        
    int pitchClass = midiNote % 12;
    return centDeviations[pitchClass];
}

inline void TuningSystem::setWellTemperament()
{
    // Bach/Kirnberger III well temperament (approximate)
    setCentDeviations({
        0.0f,    // C
        -5.9f,   // C#
        -7.8f,   // D
        -3.9f,   // D#
        -9.8f,   // E
        -2.0f,   // F
        -7.8f,   // F#
        -5.9f,   // G
        -11.7f,  // G#
        -3.9f,   // A
        -7.8f,   // A#
        -5.9f    // B
    }, "Well Temperament");
}

inline void TuningSystem::setJustIntonation()
{
    // Just intonation in C major
    setCentDeviations({
        0.0f,     // C (1/1)
        70.7f,    // C# (16/15)
        3.9f,     // D (9/8)
        15.6f,    // D# (6/5)
        -13.7f,   // E (5/4)
        -2.0f,    // F (4/3)
        68.8f,    // F# (45/32)
        2.0f,     // G (3/2)
        82.4f,    // G# (8/5)
        -15.6f,   // A (5/3)
        17.6f,    // A# (9/5)
        -31.2f    // B (15/8)
    }, "Just Intonation");
}

inline void TuningSystem::setPythagorean()
{
    // Pythagorean tuning
    setCentDeviations({
        0.0f,     // C
        13.7f,    // C#
        3.9f,     // D
        17.6f,    // D#
        7.8f,     // E
        -2.0f,    // F
        11.7f,    // F#
        2.0f,     // G
        15.6f,    // G#
        5.9f,     // A
        19.6f,    // A#
        9.8f      // B
    }, "Pythagorean");
}

inline void TuningSystem::setMeantone()
{
    // Quarter-comma meantone
    setCentDeviations({
        0.0f,     // C
        -24.3f,   // C#
        -6.8f,    // D
        -31.2f,   // D#
        -13.7f,   // E
        3.4f,     // F
        -20.9f,   // F#
        -3.4f,    // G
        -27.7f,   // G#
        -10.3f,   // A
        -34.5f,   // A#
        -17.1f    // B
    }, "Meantone");
}

inline void TuningSystem::resetToEqualTemperament()
{
    centDeviations.fill(0.0f);
    currentTuningName = "Equal Temperament";
}

inline void TuningSystem::setCentDeviations(const std::array<float, 12>& deviations, const juce::String& name)
{
    centDeviations = deviations;
    currentTuningName = name;
}