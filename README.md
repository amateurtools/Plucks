OSX Silicon:

    AU is working in OSX latest, in Logic. But you need to go into your system settings
    and allow the plugin since AmateurTools is not a licensed Apple Developer at this time.
    For added peace of mind, scan such plugins at VirusTotal.com before you do things like that.

NOTE: 

    JUCE simd optimizations added to CMakeLists.txt

PLUCKS -- a Karplus-Strong Plucked String Synthesizer

Generic GUI:

<img width="600" alt="Plucks GUI" src="https://github.com/amateurtools/Plucks/blob/main/Screenshots/screenshot_w.jpg" />

Dark Mode GUI:

<img width="600" alt="Plucks GUI" src="https://github.com/amateurtools/Plucks/blob/main/Screenshots/screenshot_g.jpg" />

Purple GUI:

<img width="600" alt="Plucks GUI" src="https://github.com/amateurtools/Plucks/blob/main/Screenshots/screenshot_p.jpg" />

If you're compiling, It's easy to make your own GUI by replacing the png elements in BinaryData.

🎧 Demo 1: https://soundcloud.com/florianhertz/plucks_test_9-13-2025

🎧 Demo 2: https://soundcloud.com/florianhertz/plucks_25_sep

Plucks is a retro, open-source Karplus-Strong plucked string synth, forked from LuckyPlucker by SuperRiley64.
Credit is due: much inspiration (and some code) remains from the original, but Plucks has evolved with extensive changes and new features.

Features & Innovations

    Expressive Velocity
    Low velocity does more than soften the note—it shapes the tone by narrowing the exciter "pulse width".

    Extra-long Decay
    The Decay parameter allows up to 60 seconds release (especially noticeable on low notes).
    (I've also tried a decay mapping 'tilt' to extend the high notes' duration. I may put it back in.)

    Signature Damping
    The Damp stage uses SuperRiley64’s custom low-pass scheme, preferred over typical SVFs.
    Also the 3 knob configuration is retained from SuperRiley64's Lucky Plucker.
    simple 1 pole filter action.

    Historically accurate Exciter Color
    Color morphs the exciter from a square wave to pure noise as opposed to an EQ boost.
    Except there's a slew rate limiter to sculpt that if you want.

    Physical Re-excitation
    Playing a note that’s still "active" re-excites the voice’s delay line instead of layering voices.
    This models a piano or harpsichord style and saves CPU. This is accomplished by injecting the
    exciter into a delayline of the same size as the exciter which causes a granular delayish tail.

    Interpolated Delayline
    in an attempt for better tuning in the high notes, an interpolated delay line is used to
    try and achieve this as well as the possibility for fine tuning +- 100 cents.

    Tuning
    +/- 100 cents fine-tuning (updated at block rate, sorry no audio rate yet)
    
    .TUN file support
    There are a few 'most common' tunings, it's easy to add more.
    (Linux has a known issue where the custom .tun file loader times out weirdly on X11)
    
    Stereo Mode
    Stereo mode randomizes the impulse for left and right, giving a lively, wide character.
    additional Stereo method: micro pitch shift: up to 5 cents of stereo detune. (5c each way, so 10c spread)

    Gate mode
    Gate preserves a familiar method from the original, but with adjustable release.

    Smart Decay
    improves on the retro vintage original from early 2000 with smooth tapering decays and no abrupt cutoffs,
    while keeping good track of voice count.
    
Installation

    Building it Requires Juce 8+, Projucer may help. 
    
Technical Notes

    WIP: The sound of Plucks may change over time. the goal is to aim for sonic accuracy, improvements,
    and elegance, while optimizing on CPU.

    A secret "advanced" page is available in the hamburger menu

    Forked under GNU or MIT license(s); uses JUCE and VST frameworks.

    Disclaimer: Provided as-is, no affiliation or endorsement. Project is independent but inspired by a famous Fruity Loops synth.

Credits

    Forked from LuckyPlucker by SuperRiley64.

    Open source, forever free.

    VST is a trademark of Steinberg GMBH.
    Special thanks to JUCE, it's a great product.
    
    Plucks: modern plucking, classic bite.
    Progressive synthesis, open inspiration.

Please Note:

    While I may recognize inspirations or experiment with music
    that is not originally mine in various audio demos, this does not imply
    any official connection or endorsement. AmateurTools has no
    affiliations with anyone; I am simply a fan. I am not personally
    connected to, in partnership with, or seeking to capitalize on any
    famous individuals or companies. If it appears otherwise, that is
    unintentional. I am an ordinary individual on the internet, not a legal
    professional or medical expert. Please do not consider anything I say as
    financial, legal, or medical advice.
