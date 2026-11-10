# Fidélité : état des preuves et prochaine étape

## État actuel

Prototype original compilé AU, VST3 et autonome. Tests audio à 44,1 / 48 / 96 kHz, extinction des notes, restauration de l'état et rendu de l'éditeur réussis. Pas encore validé dans le DAW. Aucune mesure comparative du matériel original ; aucune fidélité sonore démontrée.

## Références identifiées

- Roland, spécifications JUNO-106 : https://support.roland.com/hc/en-us/articles/201966419-JUNO-106-Technical-Specifications
- Manuel original JUNO-106 : https://cdn.roland.com/assets/media/pdf/JUNO-106_OM.pdf
- Roland, description du chorus BBD et du LFO : https://rolandcorp.com.au/blog/roland-icon-series-juno-106-synthesizer
- Roland, spécifications JUPITER-8 : https://support.roland.com/hc/en-us/articles/201945109-JUPITER-8-Technical-Specifications
- Roland, architecture et enveloppes JUPITER-8 : https://rolandcorp.com.au/blog/roland-icon-series-the-jupiter-8-synthesizer

Ces références établissent des caractéristiques fonctionnelles ; elles ne donnent pas à elles seules les valeurs de calibration du modèle DSP.

## Écarts connus du prototype

- Deux topologies génériques JUCE (ladder et état variable), librement assignables à A/B : aucune identification aux circuits Roland ni calibration matérielle.
- Chorus JUCE générique : aucune modélisation mesurée du BBD.
- Enveloppes amp/filtre indépendantes ou liées ; mêmes algorithmes pour les deux allocations de voix.
- Oscillateurs indépendants, bruit, HPF, quatre formes de LFO, modulation de hauteur/filtre/PWM, molette MIDI, mono/unison/legato présents. Hard sync et cross-modulation audio absents.
- Courbes des potentiomètres, saturation, dérive, comportement de déclenchement et allocation non calibrés sur le matériel. Les formes triangle/sinus ne constituent pas un modèle analogique.

## Usage de REA

Skill lu ; outils disponibles. Aucun firmware ou binaire de référence fourni ou présent initialement dans ce projet. Aucune analyse REA du synthétiseur original n'a donc encore été effectuée, et aucun Evidence ID n'est revendiqué. Ne pas analyser notre propre compilation comme preuve de fidélité au Roland.

Cible à préciser : modèle Jupiter exact, firmware ou émulation précise. Tous les artefacts et rapports doivent rester dans le projet. Vérifier les destinations d'écriture du fournisseur REA avant analyse pour respecter cette limite.

Un firmware peut renseigner les tables, commandes et temporisations selon ce qu'il contient ; il ne décrit pas intégralement la réponse des circuits analogiques. Une émulation logicielle renseigne son propre comportement, sans prouver l'identité au matériel.

## Protocole de calibration proposé

Relever modèle, révision, accordage, fréquence d'échantillonnage, niveaux et positions exactes des commandes. Capturer séparément les formes d'onde, balayages du filtre à plusieurs résonances, enveloppes, chorus I / II, puis accords et fins de notes. Comparer fréquence, spectre, temps d'enveloppe, réponse du filtre et modulation stéréo. Séparer les observations, hypothèses et paramètres encore inconnus. Aucune valeur arbitraire ne doit être présentée comme une mesure du matériel.

## Première inspection REA du 31 octobre 2026

Le DMG fourni contient un installateur multi-produits. Extraction ciblée des exécutables VST3 uniquement, sans exécution des scripts ni installation. Les PackageInfo annoncent JUNO-106 et JUPITER-8 2.0.2 (build 20102). Les deux exécutables contiennent des tranches Intel x86_64 et Apple arm64.

La provenance tierce du paquet ne permet pas d'attester son identité avec les distributions officielles Roland. Les conclusions portent uniquement sur les octets fournis.

- JUNO SHA-256 : 06876f63d405ef4de6398e173a47d8af33353b21a0f08511f17e3a80d363ee2d
- Jupiter SHA-256 : 4c849ba61203f06722601c13cec27b11738f04fda8ff2abd0853162c5fa6fb2c
- REA JUNO : ev_cbc05bfb4940a1023e4c9504f7c3e48fedfb6c4a26b3b9d1703e61ed780533ac
- REA Jupiter : ev_ba4cb897c3413e9718911fcaac24939c5985cdbf40bb03643fcd0fe01cc85434

Snapshots dans reference/juno-metadata.rea.json et reference/jupiter-metadata.rea.json. Symboles observés : CAssignJu106 (allocation de voix), CEnveloper et CPrmDSPJp8::PRM_DSP_VcfEnv / VcfKey / VcfReso. Leur présence fournit des points d'entrée pour une analyse ultérieure, mais ne démontre ni leurs équations ni leur rôle effectif dans le signal audio. Aucun algorithme n'a encore été décompilé ou intégré au prototype. Les imports/exports peuvent être incomplets selon les symboles conservés.

## Développement et blocage de décompilation

Ajouts testés : enveloppe filtre indépendante Jupiter, passe-haut, suivi clavier, bruit, vélocité réglable, LFO triangulaire, filtre Jupiter 12/24 dB et deux vitesses de chorus. Ces choix sont originaux et non calibrés ; le filtre reste le ladder JUCE.

L'analyse REA de PRM_DSP_VcfEnv à 0x1b1e20 a échoué au démarrage du pont Hopper (provider_timeout, puis cleanup_incomplete). Aucun pseudocode retourné. La tentative de fermeture a également signalé cleanup_incomplete. REA a créé automatiquement /tmp/rea-TUnR8f hors du projet ; ce chemin n'était pas sélectionnable dans cet appel. Ne pas réutiliser ce fournisseur avant résolution du démarrage et maîtrise de ses chemins temporaires. Aucune suppression ni modification manuelle hors projet effectuée.

Validation : compilation AU/VST3/autonome arm64, tests audio à 44,1/48/96 kHz, extinction des notes, état, paramètres extrêmes, dépassement de polyphonie et blocs de 1/17/256 échantillons. Signature ad hoc VST3 vérifiée. DAW non validé ; VST2 non produit faute de SDK.

## Extension hybride du 9 novembre 2026

Oscillateurs I/II indépendants, modulation de hauteur et filtre, molette CC1, priorité dernière note avec retour en mono/unison, legato et sustain, deux emplacements de filtre avec modèles librement assignables et routages A/B/série/parallèle. Implémentation originale à partir des primitives JUCE locales, sans nouvelle extraction REA. Les anciens paramètres conservent leurs identifiants ; les paramètres absents d'un ancien état prennent leurs valeurs par défaut.
