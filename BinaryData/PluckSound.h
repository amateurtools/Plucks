/*
  ==============================================================================

    PluckSound.h
    Created: 16 Apr 2025 6:39:37pm
    Author:  Riley Knybel

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class PluckSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int /*midiNoteNumber*/) override
    {
        return true; // Accept all notes
    }

    bool appliesToChannel(int /*midiChannel*/) override
    {
        return true; // Accept all channels
    }
};
