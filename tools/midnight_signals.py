"""Generate only the additional Midnight Signals bank; never overwrite legacy sounds."""
from pathlib import Path
from xml.etree import ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
COMMON = dict(mode=1, envLink=0, gain=.72, arpOn=0, hold=0, lfoManual=0, lfoTrig=0,
    jupiterMix=.8, oscLayer=.5, osc1Shape=0, osc2Shape=0, osc1Octave=3, osc2Octave=3,
    osc1Level=1, sub=.15, noise=0, wave=.25, width=.5, pwm=.15, pwmSource=0,
    vco1Width=.5, osc2Width=.42, vcoPwm=.15, osc2Tune=0, detune=7, crossMod=0, oscSync=0, vco2Low=0,
    age=.4, voiceWidth=.65, voiceDrive=.15, fxDrive=0, hpfFreq=.25,
    filterRoute=0, filterAType=0, slope=1, keytrack=.4, resonance=.2,
    lfoShape=1, rate=.3, lfoPitch=0, lfoFilter=0, lfoDelay=0,
    velocity=.15, velFilter=.15, atFilter=.2, atVolume=.1, wheelDepth=.35, wheelFilter=.25,
    chorusMode=1, chorus=.25, delayOn=1, delayMix=0, delaySync=0, delayTime=.375, delayFeedback=.28,
    reverbOn=1, reverbType=0, reverbSize=.65, reverbDamp=.6, reverbColor=.4, reverbMix=.15,
    voiceMode=0, legato=0, portamento=0, bendRange=2)
PAD = dict(attack=1.1, decay=2.5, sustain=.82, release=3.5,
    fattack=1.7, fdecay=3, fsustain=.5, frelease=3.8, cutoff=1100, env=.25,
    chorusMode=2, chorus=.45, reverbMix=.25, reverbSize=.78)
BASS = dict(voiceMode=1, legato=0, attack=.003, decay=.35, sustain=.65, release=.12,
    fattack=.002, fdecay=.28, fsustain=.08, frelease=.12, cutoff=300, env=.4,
    voiceWidth=0, age=.15, chorusMode=0, chorus=0, reverbMix=.025, reverbSize=.35,
    detune=2, osc1Octave=2, osc2Octave=2, sub=.7, voiceDrive=.3, velocity=.2)
BRASS = dict(attack=.035, decay=.6, sustain=.72, release=.45,
    fattack=.055, fdecay=.5, fsustain=.3, frelease=.4, cutoff=1500, env=.35,
    jupiterMix=1, detune=10, resonance=.12, chorus=.15, reverbType=1, reverbMix=.15)
LEAD = dict(gain=.85, voiceMode=1, legato=1, portamento=.065, attack=.012, decay=.7, sustain=.8, release=.55,
    fattack=.02, fdecay=.8, fsustain=.4, frelease=.5, cutoff=1800, env=.2,
    jupiterMix=1, voiceWidth=0, detune=4, chorus=.15, delayMix=.15, reverbMix=.18,
    lfoDelay=.5, rate=5.1, lfoPitch=.08, wheelDepth=.6, atFilter=.3)

SOUNDS = [
 ("PAD - Hawkins Afterglow", PAD, dict(jupiterMix=.65, osc2Shape=1, osc2Width=.36, vcoPwm=.4,
    cutoff=850, env=.3, rate=.21, lfoFilter=.16, age=.5, delayMix=.08, reverbType=2, reverbMix=.22),
  "Nappe nostalgique et inquiétante ; accords mineurs ouverts, molette pour éclaircir.", "Stranger Things"),
 ("PAD - Neon Rain", PAD, dict(jupiterMix=1, detune=13, filterRoute=4, filterBlend=.3,
    cutoff=950, env=.22, filterBType=1, slopeB=0, cutoffB=2700, resonanceB=.24, envB=.12,
    attack=1.6, release=4.8, fattack=2.5, frelease=4.8, lfoFilter=.13, reverbMix=.28, atFilter=.4),
  "Deux filtres en parallèle, nappe ample sous une ville de néons ; pression pour ouvrir.", "Blade Runner"),
 ("PAD - Red Room", PAD, dict(gain=.53, jupiterMix=.4, wave=1, pwm=.45, osc1Shape=2, osc2Shape=1,
    osc2Width=.23, detune=-8, cutoff=620, resonance=.38, env=.22, attack=.7,
    filterRoute=2, filterBType=1, cutoffB=1900, resonanceB=.42, envB=-.1, slopeB=0,
    rate=.16, lfoFilter=.23, reverbType=0, reverbMix=.24),
  "Nappe feutrée, trouble et légèrement vocale ; jouer lentement autour de C3.", "Twin Peaks"),
 ("PAD - Desert Horizon", PAD, dict(jupiterMix=1, osc2Tune=7, oscLayer=.24, detune=4,
    cutoff=750, env=.4, attack=2.2, fattack=3.4, release=5, frelease=5,
    filterRoute=4, filterBlend=.22, cutoffB=2200, resonanceB=.32, envB=.25,
    age=.65, voiceWidth=.8, reverbType=2, reverbSize=.86, reverbMix=.23, atFilter=.45),
  "Drone ample avec une quinte discrète ; jouer des octaves plutôt que des accords chargés.", "Dune"),
 ("BASS - Lab Pulse", BASS, dict(gain=.95, jupiterMix=.55, wave=1, sub=.8, osc1Shape=1, osc2Shape=4,
    cutoff=340, env=.5, fdecay=.2, sustain=.65, resonance=.28, voiceDrive=.4),
  "Basse courte et tendue ; croches répétées vers C2, arpège laissé désactivé.", "Stranger Things"),
 ("BASS - Night Pursuit", BASS, dict(gain=.9, jupiterMix=1, osc2Shape=1, osc2Width=.32, oscLayer=.18,
    cutoff=480, env=.35, fdecay=.16, resonance=.38, sustain=.55, voiceDrive=.5,
    fxDrive=.08, delayTime=.1875, delayFeedback=.15, delayMix=.055, release=.09),
  "Basse de poursuite mordante ; doubles croches et vélocités alternées.", "Thrillers de John Carpenter"),
 ("BASS - Machine Room", BASS, dict(gain=.85, jupiterMix=1, osc2Octave=1, oscLayer=.28, crossMod=.13,
    cutoff=420, env=.22, resonance=.24, voiceDrive=.55, sustain=.8, fdecay=.5,
    fxDrive=.12, detune=-3, release=.18, reverbMix=.035),
  "Grave industriel, intervalle d’octave et cross-mod légère ; notes longues vers C2.", "The Terminator"),
 ("BASS - Black Signal", BASS, dict(jupiterMix=.2, wave=1, width=.5, sub=1,
    osc1Shape=3, osc2Shape=2, cutoff=180, resonance=.1, env=.18, hpfFreq=.12,
    sustain=.9, fdecay=.45, voiceDrive=.12, legato=1, portamento=.045, reverbMix=0),
  "Sub rond et discret pour soutenir les nappes ; jeu mono lié entre C2 et C3.", "Suspense de science-fiction"),
 ("BRASS - Midnight Titles", BRASS, dict(jupiterMix=.85, cutoff=1300, env=.38,
    attack=.025, fattack=.04, detune=9, chorus=.2, reverbMix=.18),
  "Cuivres analogiques de générique ; accords courts et octaves.", "Stranger Things"),
 ("BRASS - Neon Skyline", BRASS, dict(cutoff=2100, env=.28, slope=0, attack=.1, fattack=.16,
    release=1.2, frelease=1.1, detune=15, age=.55, voiceWidth=.8,
    chorusMode=2, chorus=.3, reverbType=0, reverbMix=.22, atFilter=.35),
  "Cuivres larges et lumineux ; accords tenus dans le médium.", "Blade Runner"),
 ("BRASS - Last Transmission", BRASS, dict(osc2Shape=1, osc2Width=.31, oscLayer=.35,
    cutoff=1050, resonance=.3, env=.48, fattack=.075, fdecay=.4,
    filterRoute=2, filterBType=1, slopeB=0, cutoffB=3400, resonanceB=.2, envB=.12,
    detune=-6, delayTime=.31, delayMix=.07, reverbMix=.16),
  "Cuivres serrés et nasaux, attaque expressive ; accents à la vélocité.", "Alien et le suspense spatial"),
 ("BRASS - Desert Sun", BRASS, dict(osc2Octave=2, oscLayer=.4, detune=5,
    cutoff=900, env=.45, attack=.28, fattack=.42, decay=1.4, release=1.8,
    frelease=1.8, voiceDrive=.35, chorusMode=0, reverbType=2, reverbSize=.8, reverbMix=.2),
  "Cuivres graves et lents avec octave inférieure ; octaves ouvertes, pas de clusters.", "Dune"),
 ("LEAD - Small Town Mystery", LEAD, dict(osc1Shape=1, vco1Width=.38, osc2Shape=2,
    oscLayer=.3, vcoPwm=.15, cutoff=1300, env=.22, detune=3, jupiterMix=.85,
    delayTime=.375, delayMix=.19, reverbType=2, reverbMix=.22, release=.8),
  "Mélodie fragile et nostalgique ; notes liées, molette pour le vibrato.", "Stranger Things"),
 ("LEAD - Arcade Runner", LEAD, dict(gain=.95, oscSync=1, osc2Tune=7, oscLayer=.8, envPitch=12,
    cutoff=2600, env=.2, fattack=.002, fdecay=.22, fsustain=0, legato=0,
    portamento=.025, attack=.003, release=.24, voiceDrive=.3, delayTime=.25,
    delayMix=.12, reverbType=1, reverbMix=.1),
  "Lead sync incisif ; phrases rapides, chaque note relance le balayage.", "TRON et l’imaginaire arcade"),
 ("LEAD - Distant Visitor", LEAD, dict(osc1Shape=3, osc2Shape=1, osc2Width=.24,
    oscLayer=.18, cutoff=2300, resonance=.1, env=.08, attack=.06,
    portamento=.12, lfoPitch=.12, rate=5.6, lfoDelay=.65, delayTime=.43,
    delayMix=.2, reverbType=0, reverbMix=.25, release=1.1),
  "Lead doux et flottant ; mélodies espacées dans l’aigu, glide mesuré.", "Rencontres du troisième type"),
 ("LEAD - Rift Whistle", LEAD, dict(gain=.9, osc1Shape=2, osc2Shape=3, vco2Low=1, crossMod=.055,
    oscLayer=0, osc2Octave=3, cutoff=2000, resonance=.25, env=.12,
    lfoPitch=0, delayTime=.32, delayMix=.14, reverbType=2, reverbMix=.19,
    attack=.025, portamento=.08, age=.25),
  "Lead instable : VCO-2 inaudible en LOW module VCO-1 ; pression pour éclaircir.", "Stranger Things, passages surnaturels"),
]

def generate():
    folder=ROOT/'Presets'/'MidnightSignals'
    folder.mkdir(parents=True, exist_ok=True)
    for name, family, settings, description, inspiration in SOUNDS:
        values=COMMON | family | settings
        root=ET.Element('OrbitState', stateVersion='5', program='0', modified='0', soundName=name)
        for key,value in sorted(values.items()): ET.SubElement(root,'PARAM',id=key,value=str(float(value)))
        ET.indent(root,space='  ')
        ET.ElementTree(root).write(folder/(name+'.orbitpreset'),encoding='utf-8',xml_declaration=True)
    text=['# Midnight Signals','', '16 presets originaux pour l’architecture DCO + deux VCO d’Orbit. Inspirations d’ambiance, sans samples ni copie de patches ou de thèmes musicaux.', '',
          'Ouvrir l’afficheur du preset, puis la section **MIDNIGHT SIGNALS**. Les banques A/B/C et les anciens sons sont conservés. La banque est embarquée dans le standalone, AU et VST3 : aucune copie dans Library nécessaire.', '',
          'Les presets règlent le niveau, les effets et le mode de jeu ; HOLD et l’arpège sont désactivés. Les pads et brass sont polyphoniques ; les basses et leads sont mono. Les basses Jupiter sont déjà transposées : commencer autour de C2–C3.', '',
          '| Son | Ambiance | Jeu conseillé |','|---|---|---|']
    for name,_,_,description,inspiration in SOUNDS:text.append(f'| {name} | {inspiration} | {description} |')
    text+=['','Les rendus de validation et la démo sont générés dans `build/MidnightSignals-preview/`. La vérification automatisée mesure niveaux, valeurs finies et extinction ; elle ne remplace pas une écoute sur tes enceintes.','']
    (folder/'README.md').write_text('\n'.join(text))
    print(f'{len(SOUNDS)} presets written to {folder}')

if __name__=='__main__':generate()
