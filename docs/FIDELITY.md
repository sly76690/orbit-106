# Fidélité : ce qui est modélisé, et comment

## Principe

Orbit est un instrument original. Son moteur s'inspire du comportement documenté des polysynthés analogiques du début des années 80, à partir de sources publiques : manuels d'utilisation et de service, schémas, littérature sur les circuits et sur le traitement du signal. Aucun logiciel tiers n'est analysé, décompilé ni repris.

Toutes les constantes du moteur sont des valeurs de départ réglées à l'oreille. Aucune n'est une mesure d'un instrument réel, et aucune fidélité sonore n'est démontrée.

## État par bloc

| Bloc | Modèle actuel | Limites |
|---|---|---|
| Oscillateurs | DCO stable + deux VCO indépendants, scie/impulsion polyBLEP, triangle/sinus, carré, bruit, sub DCO ; dérive et écarts VCO par AGE | architecture inspirée, pas une reproduction exacte d’un Jupiter matériel ; synchro dure et triangle non anti-repliés explicitement |
| LFO | global, quatre formes, délai par note | sur les instruments d'époque le délai ne repart qu'après relâchement de toutes les touches |
| HPF | continu : renfort, neutre, coupe-bas de 40 Hz à 1 kHz | fréquences et pente choisies à l'oreille |
| Filtre en échelle | quatre étages, résonance rebouclée sans retard, saturation douce | pas d'auto-oscillation ; non calibré |
| Filtre à état variable | primitive JUCE | générique |
| Enveloppes | courbes de charge et de décharge | constantes de temps non mesurées |
| Chorus | une ligne à retard, deux têtes en opposition, passe-bas en entrée et en sortie | vitesses et profondeurs de départ ; pas de bruit de ligne |
| Réverbes | hall modulé à huit lignes ; plaque à deux boucles croisées | effets ajoutés, sans équivalent sur les instruments d'époque |

## Valeurs à régler en priorité

- `Source/EnsembleChorus.h` : vitesses des modes, retard central, profondeur, passe-bas.
- `Source/Voice.h` : fréquences du HPF, renfort de basses, courbure des enveloppes (`Contour`), saturation du filtre.
- `Source/VoiceAllocator.h` : niveau de l'unisson (`unisonExponent`).
- `Source/Reverbs.h` : longueurs, modulation, durée des graves, pré-délai.

## Protocole de calibration proposé

Avec un instrument réel ou des enregistrements dont les réglages sont connus : relever modèle, accordage, fréquence d'échantillonnage, niveaux et positions exactes des commandes. Capturer séparément les formes d'onde, des balayages du filtre à plusieurs résonances, les enveloppes, chaque mode du chorus, puis des accords et des fins de notes. Comparer fréquence, spectre, temps d'enveloppe, réponse du filtre et modulation stéréo. Séparer observations, hypothèses et inconnues, et ne jamais présenter une valeur choisie comme une mesure.

## Validation logicielle

Tests automatiques (`Tests/Smoke.cpp`) : audio à 44,1 / 48 / 96 kHz, extinction des notes, paramètres extrêmes, vol de voix, indépendance à la taille de bloc, arpège, HOLD, mono / unisson / legato, routages de filtres, chorus, délai du LFO, rapidité de l'enveloppe de filtre, HPF, VCA GATE, synchro, cross-modulation, réverbes, rappel des 24 patches, conversion des anciens états, rendu de la façade.

## Architecture version 5

Les modes 6 et 8 voix partagent le même moteur : chaque voix possède un DCO avec sub et bruit et une paire de VCO ; SOURCE MIX mélange les VCO, JUN/JUP mélange les deux sections avant les filtres communs. AGE règle l'accord et la dérive des VCO. Le mode 8 voix ajoute son caractère vintage : écarts par voix de coupure, de temps d'enveloppe et de vitesse de LFO (dosés par AGE), enveloppes qui suivent le clavier et courte attaque brillante.

Le chemin oscillateurs/filtres tourne à 2× la fréquence de sortie puis passe par un filtre demi-bande. Cela réduit le repliement spectral sans garantir son élimination, particulièrement en hard sync et cross-modulation fortes. Les anciens sons ne sont pas préservés.

Les tests d’architecture couvrent l’isolation des sections, les deux mélanges, les six plages, LOW FREQ, AGE, les largeurs indépendantes, l’accord fin bipolaire, SYNC/X-MOD avec source inaudible, le rappel d’état et un benchmark indicatif en unisson.
