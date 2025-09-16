Introducing: "Plucks" (present progessive verb form of Plucked)

I've made too many changes really but I still want to give credit to SuperRiley64 for the inspiration,
And I'm sure I retained many of their ideas and some of their code. So it's forever known as a Fork, and
open source free software.

Notes and innovations:

DECAY has been bestowed with an eggregious 60 seconds worth of decay (more evident in the low notes)

DAMP retains SuperRiley64's original filter scheme, which sounded better than a SVF or Low Pass.

COLOR adjusts the Karplus Strong exciter to be either a square wave or a noisy square wave, instead of an EQ boost.

STEREO randomizes both output waveforms of the noise impulse/waveform.

If an already active MIDI note is played again, instead of additively multplying voices it re-excites that voice's delay line.
This means that it behaves more like a physical piano or harpsichord, etc.

TODO: 
currently working on the tuning accuracy of higher notes.

<img width="302" alt="image" src="https://raw.githubusercontent.com/amateurtools/Plucks/refs/heads/main/PLUCKS_GUI.jpg" />
