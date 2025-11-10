# Basic Frendo C

Version C haute performance du séquenceur MIDI **Basic Frendo**, conçue pour des performances live de batterie avec latence minimale.

## Vue d'ensemble

Basic Frendo C est un séquenceur MIDI en temps réel optimisé pour les performances live. Il transforme les déclencheurs de batterie en séquences musicales complexes via une logique de mapping MIDI avancée.

## Architecture du système

### Flux complet du signal

```
Batterie physique (pièzos) 
    ↓ Audio
Carte son (amplification)
    ↓ Audio  
Bitwig Studio (détection Replacer + HW Instrument)
    ↓ MIDI
Port virtuel VirMIDI Linux
    ↓ MIDI
Basic Frendo C (séquençage)
    ↓ MIDI  
Port virtuel VirMIDI Linux
    ↓ MIDI
Bitwig Studio (lecture des séquences)
    ↓ MIDI
Synthés ou lecture sampler
    ↓ Audio
Carte son
    ↓ Audio
Bitwig Studio
```

## Structure du projet

```
basic-frendo/
├── src/                        # Code source C
│   ├── main.c                  # Programme principal
│   ├── basic_frendo.h          # Déclarations communes
│   ├── frendo_parser.c         # Parseur format .frendo
│   ├── midi_handler.c          # Interface ALSA
│   ├── frendo_core.c           # Logique métier
│   └── utils.c                 # Utilitaires
├── docs/                       # Documentation
│   ├── INSTALL-arch.md         # Installation Arch
│   └── BITWIG-WORKFLOW.md      # Workflow Bitwig
├── sets/                       # Fichiers de sets .frendo
│   ├── ido-entroido-2025/      # Set concert Entroido
│   │   ├── song1.frendo
│   │   ├── song2.frendo
│   │   └── song3.frendo
│   └── tuning/                 # Set accordage
│       └── a440.frendo
├── build/                      # Fichiers de compilation
├── Makefile                    # Build system
└── README.md                   # Cette documentation
```

## Installation et configuration

### Prérequis système (Arch Linux)

```bash
# Mise à jour du système
sudo pacman -Syu

# Outils de développement (si pas déjà installés)
sudo pacman -S base-devel

# Bibliothèque ALSA
sudo pacman -S alsa-lib

# Utilitaires MIDI (pour tests et debug)
sudo pacman -S alsa-utils
```

### Configuration VirMIDI

```bash
# Créer un port MIDI virtuel
sudo modprobe snd-virmidi midi_devs=1

# Vérifier que le port est créé
aseqdump -l
# Sortie attendue : 24:0 Virtual Raw MIDI 2-0 VirMIDI 2-0

# (Optionnel) Rendre le chargement permanent
echo "snd-virmidi midi_devs=1" | sudo tee -a /etc/modules-load.d/virmidi.conf
```

### Compilation du projet

```bash
# Vérifier les dépendances
make deps

# Compilation optimisée
make

# suppression build précédent
make clean

# Ou compilation debug (avec symbols)
make debug


```

## 🎵 Lancement et utilisation

### Démarrage du système complet

#### 1. Préparation des ports MIDI

```bash
# Terminal 1 : Vérifier les ports disponibles
aseqdump -l

# Créer VirMIDI si nécessaire  
sudo modprobe snd-virmidi midi_devs=1

# Surveiller le trafic MIDI (optionnel)
aseqdump -p 20:0
```

#### 2. Configuration Bitwig Studio

```bash
# Ouvrir Bitwig Studio
bitwig-studio
```

- Charger le projet `basic frendo.bwproject`
- Vérifier la configuration des modules Replacer et HW Instrument
- Ajuster les seuils de détection audio
- Configurer les canaux MIDI de sortie (1-4 dans Bitwig)

#### 3. Lancement de Basic Frendo C

```bash
# Terminal 2 : Lancer Basic Frendo
./basic-frendo

# Le programme affiche un menu interactif :
Available sets:
  1. ido-entroido-2025 (3 songs)
  2. tuning (1 song)

Select set [1-2]: 1

# Le set complet est chargé en mémoire
```

### Mapping des canaux MIDI

#### Canaux d'entrée (triggers)

| Canal MIDI | Déclencheur | Usage | Description |
|------------|-------------|-------|-------------|
| **0** | Grosse caisse (kick) | Tracks configurées avec `[kick]` | Déclenche les séquences assignées à ce trigger |
| **1** | Caisse claire (snare) | Tracks configurées avec `[snare]` | Déclenche les séquences assignées à ce trigger |
| **2** | MS20 | Note 48 = chanson suivante<br>Note 49 = partie suivante | Commandes de navigation |

**Note** : Le routing est **dynamique** ! Un même trigger peut déclencher plusieurs tracks simultanément selon la configuration du fichier .frendo.

#### Canaux de sortie (vers synthés)

| Track | Canal MIDI out | Description |
|-------|----------------|-------------|
| **CAT** | 3 | Synthé CAT |
| **MS20** | 4 | Synthé MS20 |
| **HAPINESTRIANGLE** | 5 | Synthé Triangle |
| **HAPINESSQUARE** | 6 | Synthé Square |
| **SAMPLERVOICE** | 7 | Sampler voix |
| **SAMPLERFX** | 8 | Sampler effets |

### Format des fichiers .frendo

Basic Frendo utilise un format custom `.frendo` optimisé pour la lisibilité et la performance :

```frendo
INPUTS
  kick = channel:0
  snare = channel:1

PART
  CAT[kick]: 48 50 52 55 | 60 62 64 67
  MS20[snare]: 36 38 40 42
  HAPINESTRIANGLE[snare]: 72 74 76 79
  HAPINESSQUARE[kick]: 84 86 88 91

PART
  CAT[kick]: 36 38 40 42
  MS20[snare]: 48 50 52 55
```

**Caractéristiques** :
- **Un fichier par chanson** : `sets/nom-du-set/chanson.frendo`
- **Section INPUTS** : Définit le mapping canal MIDI → nom symbolique
  - Format : `nom = channel:N` (où N = 0-15)
  - Exemple : `kick = channel:0` crée un trigger nommé "kick" qui écoute le canal MIDI 0
- **Section PART** : Chaque partie peut avoir plusieurs tracks jouant en parallèle
- **Routing dynamique** : Format `TRACKNAME[input]: notes`
  - Exemple : `CAT[kick]: 48 50 52` = la track CAT est déclenchée par le trigger "kick"
  - Un trigger peut déclencher plusieurs tracks : `CAT[kick]` et `MS20[kick]` jouent ensemble si kick est activé
- **Syntaxe range** : `[start..end]` pour générer des séquences automatiquement
  - `[1..16]` génère : `1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16`
  - `[60..72]` génère une octave chromatique (DO4 à DO5)
  - `[72..60]` génère la même chose en descendant
  - Mix possible : `48 [50..55] 60` → `48 50 51 52 53 54 55 60`
- **Syntaxe random** : `R[start..end]` pour générer des séquences randomisées (sans répétition)
  - `R[1..16]` génère : toutes les valeurs 1 à 16 dans un ordre aléatoire
  - `R[60..72]` génère : une octave chromatique dans un ordre aléatoire
  - Mix possible : `48 R[50..55] 60` → `48` suivi de 50-55 shufflés, puis `60`
  - **L'ordre change à chaque lancement** du programme (nouveau shuffle à chaque démarrage)
- Les **`|`** sont optionnels (séparateurs visuels pour la lisibilité, ignorés par le parser)
- **Valeurs MIDI directes** (0-127) : `0` = silence (pas de note jouée), `60` = DO central (C4)
- Les séquences **bouclent automatiquement** : arrivé à la fin, on recommence au début
- **Pas de commentaires** supportés (pour garder le parser léger)

**Les 6 tracks disponibles** (canaux de sortie fixes) :
- **CAT** → Canal MIDI sortie 3
- **MS20** → Canal MIDI sortie 4
- **HAPINESTRIANGLE** → Canal MIDI sortie 5
- **HAPINESSQUARE** → Canal MIDI sortie 6
- **SAMPLERVOICE** → Canal MIDI sortie 7
- **SAMPLERFX** → Canal MIDI sortie 8

**Exemple de routing complexe** :
```frendo
INPUTS
  kick = channel:0
  snare = channel:1

PART
  CAT[kick]: 48 50 52               # Kick déclenche CAT
  MS20[kick]: 60 62 64              # Kick déclenche AUSSI MS20
  HAPINESTRIANGLE[snare]: 72 74 76  # Snare déclenche Triangle
```
Dans cet exemple, un coup de kick joue simultanément une note de CAT ET une note de MS20 !

**Exemple avec syntaxe range** :
```frendo
INPUTS
  kick = channel:0
  snare = channel:1

PART
  # Avant (fastidieux) :
  # SAMPLERVOICE[snare]: 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16

  # Après (avec range) :
  SAMPLERVOICE[snare]: [1..16]

  # Mix de syntaxes :
  CAT[kick]: 48 [50..55] 48 [55..50]  # → 48 50 51 52 53 54 55 48 55 54 53 52 51 50

  # Range descendant :
  MS20[snare]: [72..60]  # Octave descendante
```

**Exemple avec syntaxe random** :
```frendo
INPUTS
  kick = channel:0
  snare = channel:1

PART
  # Random shuffle d'un range complet :
  SAMPLERVOICE[snare]: R[1..16]  # → 1 à 16 dans un ordre aléatoire (ex: 7 3 12 1 15 8 ...)

  # Random sur une octave chromatique :
  CAT[kick]: R[60..72]  # → DO4 à DO5 dans un ordre aléatoire

  # Mix avec notes fixes et random :
  MS20[snare]: 48 R[50..55] 60  # → 48, puis 50-55 shufflés, puis 60

  # Combinaison range normal + random :
  HAPINESTRIANGLE[kick]: [36..42] R[60..72]  # → 36-42 en ordre, puis 60-72 shufflés
```

## 🧪 Tests et validation

### Commandes de test intégrées

```bash
make test-compile      # Test de compilation
make test-midi         # Test MIDI interactif  
make midi-ports        # Lister tous les ports MIDI
make check-virmidi     # Vérifier le statut VirMIDI
make clean             # Nettoyer les fichiers de build
make help              # Aide complète du Makefile
```

### Tests fonctionnels manuels

1. **Test Track CAT (Canal 0)** : Frapper la grosse caisse (kick)
   - ✅ Doit jouer les notes de la séquence CAT configurée (ex: `42, 42, 42, 46, ...`)
   - ✅ Retour automatique au début de séquence après la dernière note
   - ✅ Vérifie que le canal MIDI out 3 reçoit bien les notes

2. **Test Track MS20 (Canal 1)** : Frapper la caisse claire (snare)
   - ✅ Doit jouer les notes de la séquence MS20 configurée (ex: `66, 70, 68, 75`)
   - ✅ Bouclage automatique
   - ✅ Vérifie que le canal MIDI out 4 reçoit bien les notes

3. **Test Routing multiple** : Si kick déclenche CAT ET MS20
   - ✅ Un seul coup de kick doit jouer une note de CAT et une note de MS20 simultanément

4. **Test Song Change (Canal 2, Note 48)** : Trigger de contrôle
   - ✅ Affichage :
     ```
     ═══════════════════════════════════════════
     [SONG CHANGE] → song2 (2/3)
     [PART]        → Part 1/2
     ═══════════════════════════════════════════
     ```
   - ✅ Reset de tous les indices de notes (CAT, MS20, etc.)

5. **Test Part Change (Canal 2, Note 49)** : Trigger de contrôle
   - ✅ Affichage :
     ```
     ═══════════════════════════════════════════
     [PART CHANGE] → song2 - Part 2/2
     ═══════════════════════════════════════════
     ```
   - ✅ Reset de tous les indices de notes

### Debug et dépannage

#### Problèmes courants

| Problème | Diagnostic | Solution |
|----------|------------|----------|
| "VirMIDI 1-0 not found" | `make midi-ports` | `sudo modprobe snd-virmidi midi_devs=1` |
| "Cannot open ALSA sequencer" | Permissions | `sudo usermod -a -G audio $USER` puis logout/login |
| Pas de réaction aux notes MIDI | Bitwig mal configuré | Vérifier canaux MIDI de sortie Bitwig (0-1 pour triggers, 2 pour contrôles) |
| Notes MIDI incorrectes | Fichier .frendo invalide | Vérifier format INPUTS et PART, valeurs 0-127 |
| Séquences ne bouclent pas | Index non resetté | Vérifier que `count > 0` dans la séquence |
| Erreur "Unknown track name" | Nom de track invalide | Utiliser uniquement CAT, MS20, HAPINESTRIANGLE, HAPINESSQUARE, SAMPLERVOICE, SAMPLERFX |
