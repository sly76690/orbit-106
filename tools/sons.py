# Orbit - writes the sounds shipped with the installer (menu SOUNDS) into Presets/Sounds.
# Each sound only lists what differs from the default settings: Orbit fills in the rest when it loads it.
# Usage:  python3 tools/sons.py      then  bash tools/paquet.sh
# A sound saved with SAVE can join them too: copy its .orbitpreset file from
# ~/Library/Application Support/SF Instruments/Orbit 106/Presets/User into Presets/Sounds (and rename it).
import os
from xml.sax.saxutils import quoteattr

# Shared starting point of the dual-filter sounds: eight voices, each filter with its own envelope settings.
DUAL = {"mode": 1, "envLink": 0, "oscLayer": 0, "age": .45, "voiceWidth": .7}

SOUNDS = {
    # Two resonant peaks in series: a pad that sounds like a vowel, the second peak closing as the first opens.
    "PAD - Twin Peaks": dict(DUAL, filterRoute=2, filterAType=0, slope=1, cutoff=850, resonance=.55, env=.3,
        filterBType=1, slopeB=0, cutoffB=2300, resonanceB=.72, envB=-.35,
        wave=.55, pwm=.35, sub=.35, detune=10, attack=.9, decay=2.5, sustain=.85, release=3.2,
        fattack=1.4, fdecay=3.5, fsustain=.45, frelease=3.0, lfoShape=1, rate=.22, lfoFilter=.35,
        chorusMode=2, chorus=.55, reverbType=2, reverbSize=.75, reverbMix=.22),
    # A talking lead pad: the LFO moves both peaks together, restarted at each note.
    "VOX - Talking Formant": dict(DUAL, filterRoute=2, filterAType=1, slope=0, cutoff=650, resonance=.68, env=.2,
        filterBType=1, slopeB=0, cutoffB=1900, resonanceB=.76, envB=.15,
        wave=.8, sub=.2, detune=7, attack=.08, decay=1.2, sustain=.8, release=1.1,
        fattack=.05, fdecay=1.0, fsustain=.5, frelease=1.0, lfoShape=1, rate=1.6, lfoFilter=.9, lfoKeyTrig=1,
        chorusMode=1, chorus=.4, delaySync=6, delayFeedback=.35, delayMix=.18, reverbMix=.12),
    # Two filters side by side: a dark ladder and a bright, singing state-variable, opened by the pressure of the keys.
    "PAD - Parallel Breath": dict(DUAL, filterRoute=4, filterBlend=.5, filterAType=0, slope=1, cutoff=520, resonance=.3, env=.35,
        filterBType=1, slopeB=0, cutoffB=3200, resonanceB=.55, envB=.2,
        wave=.4, pwm=.5, sub=.45, detune=14, attack=1.4, decay=3.0, sustain=.9, release=4.0,
        fattack=2.2, fdecay=4.0, fsustain=.6, frelease=4.0, atFilter=.4, rate=.35, lfoFilter=.2,
        chorusMode=3, chorus=.6, reverbType=2, reverbSize=.8, reverbMix=.25),
    # A bass struck through a ringing filter, then rounded by the second one.
    "BASS - Double Tok": dict(DUAL, filterRoute=3, filterAType=0, slope=1, cutoff=1300, resonance=.2, env=.15,
        filterBType=1, slopeB=1, cutoffB=380, resonanceB=.8, envB=.7,
        wave=.3, sub=.85, detune=4, attack=.002, decay=.45, sustain=.7, release=.15,
        fattack=.001, fdecay=.22, fsustain=0, frelease=.15, keytrack=.6, velFilter=.35, voiceMode=1, legato=1, portamento=.04,
        chorusMode=0, age=.3, voiceWidth=.2),
    # A lead that growls: PRE drive after the two filters, a short glide.
    "LEAD - Growl Pair": dict(DUAL, filterRoute=2, filterAType=0, slope=1, cutoff=1500, resonance=.55, env=.45,
        filterBType=1, slopeB=0, cutoffB=2600, resonanceB=.5, envB=.25,
        wave=.9, sub=.3, detune=9, attack=.005, decay=.6, sustain=.75, release=.35,
        fattack=.004, fdecay=.7, fsustain=.4, frelease=.4, voiceDrive=.7, voiceMode=1, legato=1, portamento=.08,
        wheelDepth=.5, chorusMode=1, chorus=.3, delaySync=5, delayFeedback=.3, delayMix=.15),
    # Electric-piano-like keys; hold LFO TRIG (LFO MAN) to bring in the shimmer.
    "KEYS - Manual Shimmer": dict(DUAL, filterRoute=4, filterBlend=.4, filterAType=0, slope=1, cutoff=1100, resonance=.15, env=.4,
        filterBType=1, slopeB=0, cutoffB=4200, resonanceB=.45, envB=.3,
        wave=.15, pwm=.2, sub=.3, detune=6, attack=.003, decay=1.6, sustain=.35, release=.9,
        fattack=.002, fdecay=.9, fsustain=.2, frelease=.8, velFilter=.5, lfoManual=1, lfoDelay=.4, rate=5.5, lfoPitch=.18, lfoFilter=.4,
        chorusMode=2, chorus=.45, delaySync=5, delayFeedback=.3, delayMix=.15, reverbMix=.15),
    # A fast arpeggio in thirty-second notes through two filters: hold a chord.
    "ARP - Glass Runner": dict(DUAL, filterRoute=3, filterAType=0, slope=1, cutoff=2400, resonance=.25, env=.3,
        filterBType=1, slopeB=0, cutoffB=900, resonanceB=.7, envB=.55,
        wave=.7, sub=.2, detune=6, attack=.002, decay=.25, sustain=.2, release=.2,
        fattack=.001, fdecay=.18, fsustain=0, frelease=.15, arpOn=1, arpDivision=3, arpTempo=100, arpPattern=2, arpOctaves=1, arpGate=.5,
        chorusMode=1, chorus=.35, delaySync=6, delayFeedback=.45, delayMix=.2, reverbMix=.15),
}

# Standards of the eighties, programmed from scratch on Orbit in the spirit of famous records and of the classic
# Juno / Jupiter sound types: no factory data from any instrument is copied, only the well-known recipe of each sound.
CLASSIC = {"mode": 1, "envLink": 0, "osc1Shape": 1, "osc2Shape": 1, "oscLayer": 1}
JUNO = {"mode": 0, "osc1Shape": 0, "oscLayer": 0}
SOUNDS.update({
    # The big detuned saw brass stab of 1984 (an Oberheim on the record): two saws per voice, every voice a bit off.
    "BRASS - Rooftop Jump": dict(CLASSIC, detune=9, sub=0, filterAType=0, slope=1, cutoff=3000, resonance=.1, env=.25, keytrack=.5,
        attack=.003, decay=.4, sustain=.9, release=.25, fattack=.005, fdecay=.6, fsustain=.6, frelease=.3,
        age=.7, voiceWidth=.8, chorusMode=0, reverbType=1, reverbMix=.1),
    # The dark plucked arpeggio of an eighties-style sci-fi title: hold C E G B, at 84 BPM.
    "ARP - Upside Down": dict(CLASSIC, oscLayer=.6, detune=5, sub=0, filterAType=0, slope=1, cutoff=500, resonance=.45, env=.55,
        attack=.002, decay=.5, sustain=.4, release=.3, fattack=.001, fdecay=.35, fsustain=.1, frelease=.3,
        arpOn=1, arpTempo=84, arpDivision=1, arpPattern=0, arpOctaves=1, arpGate=.6,
        delaySync=6, delayFeedback=.3, delayMix=.15, reverbType=2, reverbSize=.8, reverbMix=.2, chorusMode=0),
    # The haunting mono lead over it: saw and narrow pulse, delayed vibrato, glide, a long space.
    "LEAD - Upside Lead": dict(CLASSIC, osc2Shape=2, osc2Width=.3, oscLayer=.7, detune=6, filterAType=0, slope=1, cutoff=1400, resonance=.25, env=.35,
        attack=.03, decay=.8, sustain=.8, release=.6, fattack=.02, fdecay=1.0, fsustain=.5, frelease=.6,
        voiceMode=1, legato=1, portamento=.06, lfoDelay=.6, rate=5, lfoPitch=.12,
        delaySync=5, delayFeedback=.35, delayMix=.2, reverbType=2, reverbSize=.85, reverbMix=.3, chorusMode=0),
    # The classic string machine of the eighties polys: a pulse that breathes with the LFO, chorus II.
    "PAD - Juno Strings": dict(JUNO, wave=.5, width=.5, pwm=.55, pwmSource=0, rate=.8, sub=.2, cutoff=3500, resonance=.1, env=.1, keytrack=.5,
        attack=.45, decay=1.0, sustain=.9, release=1.2, chorusMode=2, chorus=.7, hpfFreq=.4, reverbMix=.15),
    # Square and sub, the bass of house and synth-pop.
    "BASS - Juno Sub": dict(JUNO, wave=1, width=.5, sub=1, cutoff=380, resonance=.25, env=.45, keytrack=.3, envLink=1,
        attack=.002, decay=.35, sustain=.4, release=.08, chorusMode=1, chorus=.25, hpfFreq=0),
    # The rave hoover: fast-moving pulse an octave down, a pitch swoop, glide and every chorus at once.
    "LEAD - Hoover": dict(JUNO, osc1Octave=1, wave=.5, width=.3, pwm=.8, pwmSource=0, rate=6, sub=.8, cutoff=6000, resonance=0, env=0,
        envLink=0, fattack=.001, fdecay=.25, fsustain=0, frelease=.2, envPitch=-3, attack=.005, decay=.5, sustain=1, release=.3,
        voiceMode=1, portamento=.12, chorusMode=3, chorus=.9),
    # The lush eight-voice saw strings: two detuned saws per voice, slow everything.
    "PAD - Jupiter Saw Strings": dict(CLASSIC, detune=14, filterAType=0, slope=1, cutoff=2200, resonance=.15, env=.2,
        attack=1.1, decay=2.0, sustain=.9, release=2.5, fattack=1.2, fdecay=2.0, fsustain=.7, frelease=2.5,
        lfoPitch=.06, rate=5, lfoDelay=1.0, age=.6, voiceWidth=.9, chorusMode=1, chorus=.35, reverbType=2, reverbSize=.8, reverbMix=.25),
    # A synced oscillator swept by the envelope: the metallic bell-lead of the big poly synths.
    "BELL - Sync Bell": dict(CLASSIC, oscSync=1, osc2Tune=7, envPitch=12, osc1Level=.2, oscLayer=1, filterAType=1, slope=0, cutoff=4000, resonance=.1, env=.2,
        attack=.001, decay=2.5, sustain=0, release=2.0, fattack=.001, fdecay=.6, fsustain=0, frelease=1.5,
        chorusMode=1, chorus=.3, delaySync=6, delayFeedback=.3, delayMix=.15, reverbMix=.25),
    # Bright pop chords: saw and sub through a quick filter envelope, chorus I.
    "KEYS - Poly 84": dict(JUNO, wave=.2, sub=.4, cutoff=1600, resonance=.15, env=.45, envLink=0, velFilter=.3,
        attack=.002, decay=.8, sustain=.7, release=.4, fattack=.002, fdecay=.55, fsustain=.25, frelease=.4, chorusMode=1, chorus=.5),
    # A resonant plucked bass for funk lines.
    "BASS - Funk Pluck": dict(JUNO, wave=.4, sub=.6, cutoff=280, resonance=.55, env=.7, envLink=0, keytrack=.4, velFilter=.4,
        attack=.002, decay=.3, sustain=.6, release=.08, fattack=.001, fdecay=.18, fsustain=0, frelease=.1, voiceMode=1, chorusMode=0),
    # The slow filter sweep pad: closed, then opening over seconds.
    "PAD - Sweep 82": dict(CLASSIC, oscLayer=.8, detune=10, filterAType=0, slope=1, cutoff=300, resonance=.45, env=.8,
        attack=.6, decay=3.0, sustain=.9, release=3.0, fattack=2.5, fdecay=4.0, fsustain=.4, frelease=3.0,
        chorusMode=2, chorus=.5, reverbType=2, reverbSize=.75, reverbMix=.2),
    # Synth brass of the Juno kind: one saw, a soft swell of the filter, delayed vibrato.
    "BRASS - Juno Brass": dict(JUNO, wave=0, sub=.3, cutoff=900, resonance=.15, env=.55, envLink=0,
        attack=.04, decay=.6, sustain=.8, release=.3, fattack=.06, fdecay=.7, fsustain=.45, frelease=.3,
        lfoDelay=.5, lfoPitch=.08, rate=5.5, chorusMode=1, chorus=.45),
})

# Key pressure (aftertouch) in every sound: how hard a held key is pressed opens, lifts, moves or roughens it.
# Pads and brass also get a gentler mod wheel vibrato (WHEEL).
PRESSURE = {
    "PAD - Twin Peaks": dict(atFilter=.3, atLfo=.3, atVolume=.25, wheelDepth=.25),
    "VOX - Talking Formant": dict(atLfo=.5, atFilter=.2),
    "PAD - Parallel Breath": dict(atFilter=.4, atVolume=.4, wheelDepth=.25),
    "BASS - Double Tok": dict(atFilter=.25, atDrive=.5),
    "LEAD - Growl Pair": dict(atDrive=.6, atFilter=.3),
    "KEYS - Manual Shimmer": dict(atFilter=.2, atVolume=.2),
    "ARP - Glass Runner": dict(atFilter=.35),
    "BRASS - Rooftop Jump": dict(atFilter=.35, atVolume=.3, wheelDepth=.3),
    "ARP - Upside Down": dict(atFilter=.4),
    "LEAD - Upside Lead": dict(atFilter=.25, atVolume=.2),
    "PAD - Juno Strings": dict(atFilter=.25, atVolume=.25, wheelDepth=.25),
    "BASS - Juno Sub": dict(atDrive=.4, gain=.55),
    "LEAD - Hoover": dict(atDrive=.5, atLfo=.2),
    "PAD - Jupiter Saw Strings": dict(atFilter=.3, atVolume=.3, wheelDepth=.25),
    "BELL - Sync Bell": dict(atLfo=.3),
    "KEYS - Poly 84": dict(atFilter=.3),
    "BASS - Funk Pluck": dict(atFilter=.3, atDrive=.3),
    "PAD - Sweep 82": dict(atFilter=.3, atLfo=.25, wheelDepth=.25),
    "BRASS - Juno Brass": dict(atFilter=.35, atVolume=.3, wheelDepth=.3),
}
for name, extra in PRESSURE.items():
    SOUNDS[name].update(extra)

folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Presets", "Sounds")
os.makedirs(folder, exist_ok=True)
for name, settings in SOUNDS.items():
    lines = ['<?xml version="1.0" encoding="UTF-8"?>', '<OrbitState stateVersion="4" program="0" modified="0">']
    lines += ['  <PARAM id=%s value="%s"/>' % (quoteattr(key), repr(float(value))) for key, value in settings.items()]
    lines.append('</OrbitState>')
    with open(os.path.join(folder, name + ".orbitpreset"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    print("écrit :", name)
