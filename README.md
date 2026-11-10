# Orbit 106 — synthétiseur polyphonique

Instrument original dans l'esprit des polysynthés analogiques du début des années 80. Aucun code, sample, graphisme ou preset d'un autre fabricant n'est incorporé. Le moteur est un modèle original réglé à l'oreille : voir `docs/FIDELITY.md` pour ce qui est modélisé et ce qui ne l'est pas.

## La façade

Tout est sur une seule face, redimensionnable par le coin (70 % à 150 %).

- **En-tête** : afficheur du patch (clic = menu des presets), banques A / B / C, touches 1 à 8, SAVE, LOAD.
- **Rangée de faders** : LFO (RATE, DELAY, PITCH), OSC I, OSC II, HPF à quatre positions, FILTER, AMP, ENV / AMP, ENV / FILTER, CHORUS.
- **Rangée de potards** : formes d'onde et octaves des deux oscillateurs, X-MOD, SYNC, forme du LFO, source du PWM, mode d'enveloppe, VCA, HOLD, filtre A (modèle, polarité, bend), filtre B, routage.
- **Rangée de jeu** : voix, glide et bend, arpège, drive, delay, réverbe, volume.
- **Clavier** 61 touches, OCT − / OCT +.

Double-clic sur un fader ou un potard : retour à la valeur par défaut. Survol : affichage de la valeur.

## Moteur

- **Voix** : 6 ou 8 voix ; POLY, MONO (priorité à la dernière note) ou UNI (toutes les voix empilées, désaccord réglable par SPREAD). LEGATO conserve les enveloppes entre notes liées.
- **Oscillateurs** : deux oscillateurs indépendants (mélange scie / impulsion, scie, impulsion, triangle, sinus), octave −2 à +2, sous-oscillateur et bruit. PWM par le LFO, manuel ou par l'enveloppe. **SYNC** resynchronise l'oscillateur II sur le I (l'enveloppe de hauteur ne balaie alors que le II). **X-MOD** : le II module la fréquence du I.
- **LFO** : un seul pour tout l'instrument, quatre formes, délai d'apparition par note. La molette de modulation n'est pas retardée.
- **HPF** : 0 renfort de basses, 1 neutre, 2 et 3 coupe-bas.
- **Filtres A / B** : modèle en échelle ou à état variable, 12 ou 24 dB, sans lissage interne. Routage A, B, A>B, B>A ou parallèle. Enveloppe du filtre A inversable ; le pitch bend peut aussi déplacer le cutoff.
- **Enveloppes** à courbes (charge et décharge), une pour l'ampli, une pour le filtre. **SOURCE** choisit pour le filtre ses propres faders (OWN) ou une copie de l'ampli (AMP) ; saisir un fader ENV / FILTER bascule sur OWN.
- **VCA** : ENV ou GATE.
- **HOLD** : les notes tiennent après relâchement ; un nouvel accord remplace l'ancien.
- **Arpège** : UP, DOWN, U&D, RND ; 1 à 3 octaves ; pas 1/4, 1/8, 1/16. Avec SYNC et le DAW en lecture, les pas se calent sur la grille du projet.
- **Chorus d'ensemble** : OFF, I, II, I+II. Deux têtes de lecture balayées en opposition sur une ligne à retard courte.
- **Effets** : drive, delay, réverbe VINT (hall profond et modulé) ou PLATE.

## Presets

24 patches d'usine (banques A, B, C). Un changement de patch ne touche pas au volume, à l'arpège, à HOLD ni à la plage de bend.

Les presets personnels (`.orbitpreset`) sont enregistrés dans `~/Library/Application Support/SF Instruments/Orbit 106/Presets/User`. Nom de 1 à 64 caractères : lettres sans accents, chiffres, espaces, tirets, underscores. Un nom existant n'est jamais écrasé. Les réglages sont aussi sauvegardés dans la session du DAW ; les sessions et presets des versions précédentes sont convertis au chargement.

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

Chaque compilation installe l'AU dans `~/Library/Audio/Plug-Ins/Components` et le VST3 dans `~/Library/Audio/Plug-Ins/VST3`. Fermer le DAW avant de compiler. Le standalone est dans `build/Orbit106_artefacts/Release/Standalone`.

Compilation Apple Silicon arm64. Pas de signature Developer ID ni de notarisation : ce n'est pas encore un installateur distribuable.

## Sources

| Fichier | Rôle |
|---|---|
| `Source/Plugin.cpp` | processeur, paramètres, presets, état |
| `Source/Voice.h` | une voix : oscillateurs, filtres, enveloppes |
| `Source/VoiceAllocator.h` | poly, mono, unisson |
| `Source/Arpeggiator.h`, `Source/NoteHold.h` | arpège et HOLD |
| `Source/EnsembleChorus.h`, `Source/Reverbs.h` | chorus et réverbes |
| `Source/Editor.h` | façade |
| `Tests/Smoke.cpp` | tests audio et rendu de la façade |
