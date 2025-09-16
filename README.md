Introducing: "Plucks" (present progessive verb form of Pluck)

How it sounds: https://soundcloud.com/florianhertz/plucks_test_9-13-2025

<img width="600" alt="image" src="https://raw.githubusercontent.com/amateurtools/Plucks/refs/heads/main/PLUCKS_GUI.jpg" />

FORK of LuckyPlucker by SuperRiley64

Notes and innovations:

I've made too many changes really but I still want to give credit to SuperRiley64 for the inspiration,
And I'm sure I retained many of their ideas and some of their code. So it's forever known as a Fork, and
open source free software.

Low velocity on notes doesn't merely affect volume, but also the character of the notes in the form of pulse width reduction.

DECAY has been bestowed with an eggregious 60 seconds worth of decay (more evident in the low notes)

DAMP retains SuperRiley64's original filter scheme, which sounded better than a SVF or other Low Pass.

COLOR adjusts the Karplus Strong exciter to be either a square wave or a noisy square wave, instead of an EQ boost.

GATE might be the same method, or at least has the same effect.

STEREO randomizes both output waveforms of the noise impulse/waveform.

If an already active MIDI note is played again, instead of additively multplying voices it re-excites that voice's delay line.
This means that it behaves more like a physical piano or harpsichord, etc. (i believe it might be a cpu saving feature)

TODO: 
currently working on the tuning accuracy of higher notes. Such is the bane of Karplus Strong. 

the second page will probably be a menu for advanced parameters, if there are enough useful ones to justify it.

DISCLAIMER:
As-is, user accepts all responsibility. No affiliation with anyone. Not endorsed by anyone.
Admittedly this is heavily inspired by a famous synth from Fruity Loops, but it is an independent implementation.

GNU License and/or MIT, see also also JUCE license, see also VST license.

