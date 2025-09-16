Plucks
Karplus-Strong Plucked String Synthesizer

<img width="600" alt="Plucks GUI" src="https://raw.githubusercontent.com/amateurtools/Plucks/refs/heads/main/PLUCKS_GUI.jpg" />
About

🎧 Listen to a demo: https://soundcloud.com/florianhertz/plucks_test_9-13-2025

Plucks is a modern, open-source Karplus-Strong plucked string synth, forked from LuckyPlucker by SuperRiley64.
Credit is due: much inspiration (and some code) remains from the original, but Plucks has evolved with extensive changes and new features.
Features & Innovations

    Expressive Velocity
    Low velocity does more than soften the note—it shapes the tone by narrowing the exciter "pulse width".

    Extra-long Decay
    The Decay parameter allows up to 60 seconds release (especially notable on low notes).

    Signature Damping
    The Damp stage uses SuperRiley64’s custom low-pass scheme, preferred over typical SVFs.

    Flexible Exciter Color
    Color morphs the exciter from a square wave to a noisy variant—not just a generic EQ boost.

    Physical Re-excitation
    Playing a note that’s still "active" re-excites the voice’s delay line instead of layering voices.
    This models a piano or harpsichord style and saves CPU.

    Randomized Stereo
    Stereo mode randomizes the impulse for left and right, giving a lively, wide character.

    Gating
    Gate preserves a familiar method from the original.

Technical Notes

    WIP: Tuning for higher notes is being improved (a classic Karplus-Strong challenge).

    A future "advanced" page is planned for more parameter tweaks.

    Forked under GNU or MIT license(s); uses JUCE and VST frameworks.

    Disclaimer: Provided as-is, no affiliation or endorsement. Project is independent but inspired by a famous Fruity Loops synth.

Credits

    Forked from LuckyPlucker by SuperRiley64.

    Open source, forever free.

    Plucks: modern plucking, classic bite.
    Progressive synthesis, open inspiration.
