# Orbit 106 — synthétiseur hybride

Instrument original inspiré du Juno-106 et du Jupiter. Aucun code, sample, graphisme ou preset Roland n'est incorporé. La fidélité au matériel n'est pas mesurée : le filtre et les effets restent des modèles génériques/originaux. Voir `docs/FIDELITY.md` pour les observations REA et leurs limites.

## Jouer et combiner

- **VOICE ALLOCATION** : six voix (106) ou huit voix (Jupiter). Le choix ne verrouille plus le filtre ou les oscillateurs.
- **OSC II LAYER** : ajouter librement le deuxième oscillateur, de zéro à un. **DETUNE** règle son désaccord ; **SUB** fonctionne dans les deux modes.
- **FILTER SLOPE** : 12 ou 24 dB/octave dans les deux modes.
- **LINK AMP / FILTER ENV** : allumé, le filtre suit l'enveloppe d'amplitude ; éteint, ses quatre faders FILTER ENV deviennent indépendants. Les faders grisés sont seulement ceux de l'enveloppe liée.
- Clavier 61 touches, déplacement par octave ; double-clic sur un contrôle pour le réinitialiser, survol pour afficher sa valeur.
- Passe-haut, bruit, suivi clavier, PWM, vélocité, pitch bend et chorus stéréo.

## SYNTH EDIT : oscillateurs, modulation et filtres

Cliquer **SYNTH EDIT** pour ouvrir les trois panneaux OSCILLATORS, MODULATION / VOICES et DUAL FILTER. Le clavier, les banques, l’arpégiateur et les effets restent visibles. Cliquer ensuite **BACK / PLAY** pour retrouver les presets, les effets et le clavier.

- **Oscillateurs I / II** : forme indépendante (saw, pulse, triangle, sine ou mélange historique), octave de −2 à +2 et niveau. Oscillateur II : accord −12 à +12 demi-tons, désaccord fin et largeur d'impulsion. Le mode **Legacy mix** conserve le mélange saw/pulse et la largeur communs des anciens presets ; choisir Pulse sur II pour sa largeur indépendante.
- **Modulation** : LFO triangle/sinus/dent de scie/carré, profondeur vers la hauteur (demi-tons), les deux filtres (octaves) et PWM. L'enveloppe filtre module aussi la hauteur, avec profondeur bipolaire. CC1 ajoute du vibrato selon **CC1 > VIBRATO**, même si PITCH est à zéro. Le pitch bend possède sa plage réglable.
- **POLY** : six ou huit notes selon VOICE ALLOCATION. **MONO** : priorité à la dernière note, retour à la précédente tenue. **UNISON** : six ou huit voix sur la note prioritaire, désaccord symétrique réglable et niveau compensé. **LEGATO** conserve les enveloppes entre notes superposées en mono/unison ; éteint, elles redémarrent. Sustain CC64 pris en charge hors arpège. Changer de mode coupe les voix courantes : rejouer les notes.
- **Filtres A / B** : chaque emplacement propose indépendamment **Ladder** ou **State variable**, cutoff, résonance, pente 12/24 dB et profondeur d'enveloppe. Le suivi clavier, le LFO et la source d'enveloppe sont communs. B dispose d'une profondeur d'enveloppe bipolaire.
- **ROUTING** : A seul, B seul, A → B, B → A, parallèle. **PARALLEL A / B** règle la balance en parallèle seulement. Aucun modèle n'est verrouillé par le choix 106/Jupiter.

Les deux filtres sont des topologies DSP JUCE distinctes, pas des reproductions calibrées des circuits Juno/Jupiter. Hard sync et cross-modulation audio ne sont pas implémentés.

Tous les nouveaux paramètres sont automatisables et inclus dans les sauvegardes de presets et de sessions. Les anciens états utilisent les valeurs par défaut pour les paramètres absents (route A, mode poly, mélange historique).

## Presets d'usine et personnels

**24 patches originaux**, accessibles par le menu ou les banques **A / B / C**, chacune avec huit touches. PREV/NEXT parcourt les presets d'usine. Une LED identifie le patch actif ; les patches personnels n'allument pas de touche d'usine.

Les premiers huit patches existants sont conservés. La banque enrichie inclut Round Sub, Rubber Funk, Hybrid Drive, Pulse Runner, Deep Velvet, Golden Horns, Hybrid Stabs, Soft Ensemble, Silk Cloud, Hybrid Aurora, Jupiter Choir, Midnight Cinema, Glass Atmosphere et Warm Horizon.

- **BRASS / Jump - Tribute** : banque **B, touche 6**.
- **BRASS / Jump Wide** : banque **B, touche 7**.

Ces brass sont des créations inspirées du son de « Jump », sans garantie d'identité avec l'enregistrement.

### Sauvegarder

1. Régler le son, puis cliquer **SAVE**.
2. Saisir un nom de 1 à 64 caractères : lettres sans accents, chiffres, espaces, tirets ou underscores.
3. Le fichier `.orbitpreset` est écrit dans **`Presets/User` du projet VSTJUNO** et apparaît sous USER dans le menu des patches.

Un nom existant n'est jamais écrasé : utiliser un autre nom pour une nouvelle version. Les liens symboliques aux emplacements de presets sont refusés. Les fichiers sont propres à Orbit, pas au format de preset Roland.

Sélectionner un patch USER pour le charger. **LOAD** actualise le menu et recharge le patch actuel depuis son fichier (ou recharge le preset d'usine sélectionné) ; cette action remplace les modifications non sauvegardées. Les réglages et le nom du patch sont également sauvegardés dans la session du DAW.

La destination est fixée au dossier du projet au moment de la compilation pour respecter le périmètre demandé. Déplacer le projet nécessite une recompilation ; cette version n'est pas un installateur distribuable avec dossier utilisateur global.

## Performance et effets

- **PORTAMENTO** : glissement polyphonique simple de fréquence, 0 à 2 s, depuis la dernière note du moteur. En mono/unison, le glissement accompagne les changements de note prioritaires.
- **ARP ON** : UP, DOWN ou UP/DOWN, 1 à 3 octaves, pas 1/4, 1/8 ou 1/16 et gate. Maintenir les touches ; pas de latch. HOST SYNC suit le BPM du DAW lorsqu'il est disponible, sinon TEMPO. Le motif démarre avec les notes et n'est pas aligné sur les mesures du transport. Sustain ignoré en arpège.
- **EFFECTS** : drive, chorus et sortie. Les deux derniers sont les mêmes paramètres que ceux du panneau supérieur.
- **DELAY** : 30 ms à 1,5 s, feedback plafonné à 75 %, niveau ; pas de synchro tempo.
- **REVERB** : taille, amortissement des aigus et niveau.

## Compilation et sorties

JUCE 8.0.6 doit être dans `vendor/JUCE-8.0.6`. Dépendances et sorties sont exclues du versionnement ; consulter la licence JUCE avant distribution.

```sh
mkdir -p .local-cache
TMPDIR="$PWD/.local-cache" cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
TMPDIR="$PWD/.local-cache" cmake --build build --parallel 4
TMPDIR="$PWD/.local-cache" ctest --test-dir build --output-on-failure
```

Sorties locales dans `build/Orbit106_artefacts/Release/` :

- `VST3/Orbit 106.vst3`
- `AU/Orbit 106.component`
- `Standalone/Orbit 106.app`

Compilation actuelle Apple Silicon arm64. Aucune installation automatique. VST2 absent faute de SDK. Pas de signature Developer ID ou de notarisation pour distribution. Le chargement dans le DAW cible reste à valider.

## Vérifications

Tests audio à 44,1 / 48 / 96 kHz, extinction des notes, paramètres extrêmes, vol de voix, blocs courts, timing de l'arpégiateur, traînes delay/reverb, effet du portamento, différence audio des combinaisons hybrides, audio/rappel des 24 patches, aller-retour des presets sur disque, protection des noms existants et rejet de chemins sortants. Captures des deux façades dans `build/interface.png` et `build/interface-jupiter.png`.

Tests supplémentaires : indépendance des oscillateurs, modulation/CC1, priorité et retour de note mono/unison, legato, sustain et extinction, invariance du timing à 17/256 échantillons, combinaisons des deux modèles et cinq routages, migration des anciens états. Capture détaillée : `build/interface-synth-edit.png`.
