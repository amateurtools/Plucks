PRE-RElEASE NOTE:

    TODO: second page of interesting parameters, pitch varied stereo expansion.

Plucks

    Form: Third-person singular present tense

Karplus-Strong Plucked String Synthesizer

<img width="600" alt="Plucks GUI" src="https://raw.githubusercontent.com/amateurtools/Plucks/refs/heads/main/PLUCKS.jpg" />

🎧 Listen to a demo: https://soundcloud.com/florianhertz/plucks_test_9-13-2025

Plucks is a retro, open-source Karplus-Strong plucked string synth, forked from LuckyPlucker by SuperRiley64.
Credit is due: much inspiration (and some code) remains from the original, but Plucks has evolved with extensive changes and new features.
Features & Innovations

    Expressive Velocity
    Low velocity does more than soften the note—it shapes the tone by narrowing the exciter "pulse width".

    Extra-long Decay
    The Decay parameter allows up to 60 seconds release (especially noticeable on low notes).

    Signature Damping
    The Damp stage uses SuperRiley64’s custom low-pass scheme, preferred over typical SVFs.

    Flexible Exciter Color
    Color morphs the exciter from a square wave to pure noise as opposed to an EQ boost.

    Physical Re-excitation
    Playing a note that’s still "active" re-excites the voice’s delay line instead of layering voices.
    This models a piano or harpsichord style and saves CPU. This is accomplished by injecting the
    exciter into a timed delayline.

    Interpolated Delayline
    in an attempt for better tuning in the high notes, an interpolated delay line is used to
    try and achieve this as well as the possibility for fine tuning +- 100 cents.

    Randomized Stereo
    Stereo mode randomizes the impulse for left and right, giving a lively, wide character.

    Gate mode
    Gate preserves a familiar method from the original.

Technical Notes

    WIP: The sound of Plucks may change over time. the goal is to aim for sonic improvements
    while reducing CPU use. (unless I add more features)

    A future "advanced" page is planned for more parameter tweaks.

    Forked under GNU or MIT license(s); uses JUCE and VST frameworks.

    Disclaimer: Provided as-is, no affiliation or endorsement. Project is independent but inspired by a famous Fruity Loops synth.

Credits

    Forked from LuckyPlucker by SuperRiley64.

    Open source, forever free.

    Plucks: modern plucking, classic bite.
    Progressive synthesis, open inspiration.
