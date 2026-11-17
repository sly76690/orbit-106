# Orbit 106 — synthétiseur polyphonique

Instrument original dans l'esprit des polysynthés analogiques du début des années 80. Aucun code, sample, graphisme ou preset d'un autre fabricant n'est incorporé. Le moteur est un modèle original réglé à l'oreille : voir `docs/FIDELITY.md` pour ce qui est modélisé et ce qui ne l'est pas.

## La façade

Tout est sur une seule face, redimensionnable par le coin (70 % à 150 %).

- **En-tête** : afficheur du patch (clic = menu des presets), banques A / B / C, touches 1 à 8, SAVE, LOAD.
- **Rangée de faders** : JUNO / DCO, SOURCE MIX, FILTER A avec HPF continu, ENV / AMP et ENV / FILTER.
- **Rangée de potards** : JUPITER / DUAL VCO (formes, plages, largeurs, X-MOD, FINE), LFO / MODULATION, FILTER B, routage, chorus et drive. TUNE, SYNC et LOW sont regroupés dans SOURCE MIX.
- **Rangée de jeu** : voix, glide et bend, arpège, drive, delay, réverbe, volume.
- **Clavier** 61 touches, OCT − / OCT +.

Double-clic sur un fader ou un potard : retour à la valeur par défaut. Survol : affichage de la valeur.

## Moteur

- **Architecture** : trois oscillateurs par voix, répartis en deux sections indépendantes. JUNO et JUPITER sont mélangés avant le HPF, les filtres A/B et le VCA communs. Les effets sont ensuite appliqués à la somme des voix.
- **JUNO / DCO** : oscillateur stable à 8', mélange SAW/PLS, WIDTH, PWM, DCO LEVEL, sub une octave plus bas et bruit. AGE et les réglages des VCO ne modifient pas cette source.
- **JUPITER / DUAL VCO** : deux oscillateurs indépendants avec saw, pulse, triangle, sine, square et noise ; plage 64', 32', 16', 8', 4', 2'. Chaque VCO a sa propre largeur d'impulsion. VCO PWM dose leur modulation commune, indépendamment du PWM du DCO, avec la source choisie dans LFO / MODULATION. Square reste à 50 %, sans PWM.
- **VCO-2** : TUNE ±12 demi-tons et FINE ±50 cents. LOW remplace le suivi du clavier par 2 Hz à 8' ; RANGE, TUNE, FINE et AGE restent actifs. SYNC reste disponible en LOW et peut donc réinitialiser sa phase à chaque cycle du VCO-1.
- **X-MOD / SYNC** : VCO-2 module la fréquence de VCO-1 ; VCO-1 synchronise VCO-2. Ces connexions fonctionnent même lorsque leur source est inaudible dans SOURCE MIX. Avec SYNC, ENV>PITCH balaie VCO-2 seul dans la section Jupiter ; le DCO conserve sa modulation d'enveloppe.
- **Deux mélanges** : JUN / JUP passe du DCO + sub + bruit aux deux VCO ; VCO 1 / 2 règle leur équilibre interne. Mélanges linéaires lissés sur environ 5 ms, sans renfort artificiel au centre.
- **AGE** : écarts d'accord et dérives indépendantes des VCO par voix. En 8 voix, AGE dose aussi le caractère vintage : chaque voix a sa coupure de filtre, ses temps d'enveloppe et sa vitesse de LFO légèrement différents.
- **8 voix vintage** : les enveloppes suivent le clavier (plus rapides dans l'aigu, plus lentes dans le grave) et chaque note démarre par une courte attaque plus brillante. Le mode 6 voix reste net.
- **Voix** : 6 ou 8 voix du même moteur ; POLY, MONO (priorité à la dernière note) ou UNI (toutes les voix empilées, désaccord réglable par SPREAD). LEGATO conserve les enveloppes entre notes liées. WIDTH et PRE DRIVE fonctionnent aux deux nombres de voix. Changer le nombre de voix coupe les notes en cours.
- **LFO** : un seul pour tout l'instrument, quatre formes, délai d'apparition par note. La molette de modulation n'est pas retardée.
- **HPF** : fader continu ; renfort de basses en bas, zone neutre, puis coupe-bas de 40 Hz à 1 kHz.
- **Filtres A / B** : modèle en échelle ou à état variable, 12 ou 24 dB, sans lissage interne. Routage A, B, A>B, B>A ou parallèle. Enveloppe du filtre A inversable ; le pitch bend peut aussi déplacer le cutoff.
- **Enveloppes** à courbes (charge et décharge), une pour l'ampli, une pour le filtre. **SOURCE** choisit pour le filtre ses propres faders (OWN) ou une copie de l'ampli (AMP) ; saisir un fader ENV / FILTER bascule sur OWN.
- **VCA** : ENV, GATE ou enveloppe de filtre.
- **HOLD** : les notes tiennent après relâchement ; un nouvel accord remplace l'ancien.
- **Arpège** : UP, DOWN, U&D, RND ; 1 à 3 octaves ; pas 1/4, 1/8, 1/16, 1/32. Avec SYNC et le DAW en lecture, les pas se calent sur la grille du projet.
- **Chorus d'ensemble** : OFF, I, II, I+II. Deux têtes de lecture balayées en opposition sur une ligne à retard courte.
- **Effets** : chorus (OFF / I / II / I+II), **phaser** (ON, RATE, DEPTH, FDBK, PH MIX, après le chorus ; éteint par défaut, les anciens sons ne changent pas), drive PRE (voix) et POST (sortie), delay (TIME libre jusqu'à 1,5 s ; en SYNC, la valeur de note choisie est jouée exactement jusqu'à 4 s, soit une blanche jusqu'à 30 BPM), réverbe VINT (hall profond et modulé), PLATE ou SPACE (très grand espace, halo à l'octave en option).

## Midnight Signals

Banque additionnelle intégrée : **16 nouveaux presets**, quatre pads, quatre basses, quatre brass et quatre leads, conçus pour le DCO + deux VCO. Dans le menu du nom de patch, choisir **MIDNIGHT SIGNALS**. Les banques A/B/C et les anciens sons restent disponibles. Voir [le guide de jeu](Presets/MidnightSignals/README.md). Aucun sample ni patch tiers ; inspirations cinématographiques originales.

## Presets

24 patches d'usine (banques A, B, C). Un changement de patch ne touche pas au volume, à l'arpège, à HOLD ni à la plage de bend.

**Version d’état 5 :** nouvelle architecture d’oscillateurs. Les anciens fichiers restent chargeables, mais leur son n’est pas conservé. Les 24 patches intégrés sont des points de départ ; la banque de 20 sons sur disque reste à refaire.

Les presets personnels (`.orbitpreset`) sont enregistrés dans `~/Library/Application Support/SF Instruments/Orbit 106/Presets/User`. Nom de 1 à 64 caractères : lettres sans accents, chiffres, espaces, tirets, underscores. Un nom déjà pris n'est remplacé qu'après confirmation (« Remplacer le preset ? »). Les réglages sont aussi sauvegardés dans la session du DAW ; les sessions et presets des versions précédentes sont convertis au chargement.

## Clavier de l'ordinateur (standalone)

Disposition AZERTY : Q S D F G H J K L M pour les touches blanches à partir de do, Z E T Y U O P pour les noires. Dans un DAW, cliquer sur le clavier d'Orbit pour lui donner les touches.

## Compilation

JUCE 8.0.6 doit être dans `vendor/JUCE-8.0.6` (consulter sa licence avant toute distribution).

```sh
mkdir -p .local-cache
TMPDIR="$PWD/.local-cache" cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
TMPDIR="$PWD/.local-cache" cmake --build build --parallel 4
TMPDIR="$PWD/.local-cache" ctest --test-dir build --output-on-failure
```

La compilation n’installe rien hors du projet (`COPY_PLUGIN_AFTER_BUILD FALSE`). Les AU, VST3 et standalone sont dans `build/Orbit106_artefacts/Release/`. Pour compiler seulement les tests : `cmake --build build --target OrbitSmoke --parallel 4`.

Compilation Apple Silicon arm64. Pas de signature Developer ID ni de notarisation : ce n'est pas encore un installateur distribuable.

## Sources

| Fichier | Rôle |
|---|---|
| `Source/Plugin.cpp` | processeur, paramètres, presets, état |
| `Source/OscillatorBank.h` | DCO + deux VCO, dérive, cross-mod, sync, mélanges |
| `Source/Voice.h` | une voix : modulation, filtres, enveloppes, VCA |
| `Source/VoiceAllocator.h` | poly, mono, unisson |
| `Source/Arpeggiator.h`, `Source/NoteHold.h` | arpège et HOLD |
| `Source/EnsembleChorus.h`, `Source/Reverbs.h` | chorus et réverbes |
| `Source/Editor.h` | façade |
| `Tests/Smoke.cpp`, `Tests/OscillatorArchitecture.h`, `Tests/AuditRegression.h` | tests audio, architecture, régressions et rendu de la façade |

## Corrections moteur (17 novembre 2026)

- Un bloc audio plus grand que prévu par le DAW est joué en morceaux : aucune allocation de mémoire pendant la lecture.
- Passer de 6 à 8 voix (ou l'inverse) laisse les notes en cours finir sur leur release au lieu de les couper.
- Delay synchronisé : la valeur de note choisie est jouée exactement jusqu'à 4 s (ligne de retard allongée).
- Tests : `Tests/EngineFixes.h`.
