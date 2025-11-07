# Basic Frendo C

Version C haute performance du séquenceur MIDI **Basic Frendo**, conçue pour des performances live de batterie avec latence minimale.

## 🎯 Vue d'ensemble

Basic Frendo C est un séquenceur MIDI en temps réel optimisé pour les performances live. Il transforme les déclencheurs de batterie en séquences musicales complexes via une logique de mappi## 🏠 Installation système (Arch Linux)g MIDI avancée.

### Obje## 📊 Performance et optimisationtifs de performance

- **Latence ultra-faible** : < 1ms entre réception et émission MIDI  
- **Fiabilité live** : Stable pour les performances en direct
- **Simplicité** : Code C maintenable et facilement extensible
- **Performance** : Interface ALSA directe, pas de couches d'abstraction

## 🏗️ Architecture du système

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
```

### Composants physiques

1. **Quatre pièzos** connectés aux éléments de batterie (grosse caisse, caisse claire, etc.)
2. **Carte son** qui amplifie et numérise les signaux audio
3. **Bitwig Studio** avec le projet `basic frendo.bwproject` :
   - Module **Replacer** : détection de seuil audio → déclencheur MIDI
   - Module **HW Instrument** : génération MIDI sur canaux configurés (1-16)
4. **Basic Frendo C** : traitement MIDI intelligent et génération de séquences

## 📁 Structure du projet

```
basic-frendo/
├── � src/                     # Code source C
│   ├── main.c                  # Programme principal
│   ├── basic_frendo.h          # Déclarations communes
│   ├── frendo_parser.c         # Parseur format .frendo
│   ├── midi_handler.c          # Interface ALSA
│   ├── frendo_core.c           # Logique métier
│   └── utils.c                 # Utilitaires
├── � docs/                    # Documentation
│   ├── INSTALL-arch.md         # Installation Arch
│   └── BITWIG-WORKFLOW.md      # Workflow Bitwig
├── 📂 sets/                    # Fichiers de sets .frendo
│   ├── ido-entroido-2025/      # Set concert Entroido
│   │   ├── song1.frendo
│   │   ├── song2.frendo
│   │   └── song3.frendo
│   └── tuning/                 # Set accordage
│       └── a440.frendo
├── � build/                   # Fichiers de compilation
├── � Makefile                 # Build system
└── 📖 README.md                # Cette documentation
```

### Description détaillée des fichiers

### Organisation modulaire

- **`src/`** : Code source C organisé en modules fonctionnels
- **`sets/`** : Fichiers .frendo de configuration musicale (un répertoire par set)
- **`build/`** : Artefacts de compilation (généré automatiquement)

## 🚀 Installation et configuration

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
aseqdump -p 24:0
```

#### 2. Configuration Bitwig Studio

```bash
# Ouvrir Bitwig Studio
bitwig-studio

# Ouvrir la console de debug (optionnel)
# Dans Bitwig : Ctrl+Shift+J
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

#### 4. Test du système

```bash
# Terminal 3 : Monitoring MIDI en temps réel
aseqdump -p 24:0

# Frapper la batterie et observer :
# - Réception MIDI de Bitwig → Basic Frendo  
# - Émission MIDI de Basic Frendo → Bitwig
```

### Mapping des canaux MIDI

| Canal Bitwig | Canal interne | Déclencheur | Action | Description |
|--------------|---------------|-------------|---------|-------------|
| **1** | 0 | Grosse caisse | **Bass** | Joue la note bass suivante de la séquence |
| **2** | 1 | Caisse claire | **Melody** | Joue la note melody suivante de la séquence |
| **3** | 2 | Tom/Crash | **Song** | Passe à la chanson suivante dans le set |
| **4** | 3 | Ride/Hi-hat | **Part** | Passe à la partie suivante de la chanson |

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
- **INPUTS** : Définit le mapping canal MIDI → nom symbolique
- **PART** : Chaque partie peut avoir plusieurs tracks
- **TRACK[input]** : Format `TRACKNAME[input]: notes`
- Les **`|`** sont optionnels (séparateurs visuels ignorés)
- **Valeurs MIDI directes** (0-127) : `0` = silence, `60` = DO central (C4)
- Les séquences bouclent automatiquement

**Tracks disponibles** :
- **CAT** → Canal MIDI sortie 3
- **MS20** → Canal MIDI sortie 4
- **HAPINESTRIANGLE** → Canal MIDI sortie 5
- **HAPINESSQUARE** → Canal MIDI sortie 6
- **SAMPLERVOICE** → Canal MIDI sortie 7
- **SAMPLERFX** → Canal MIDI sortie 8

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

1. **Test Bass (Canal 0)** : Frapper la grosse caisse
   - ✅ Doit jouer séquentiellement : `61, 68, 61, 68, 61, 68, 61, 73, ...`
   - ✅ Retour automatique au début de séquence

2. **Test Melody (Canal 1)** : Frapper la caisse claire  
   - ✅ Doit jouer séquentiellement : `73, 68, 58` (puis boucle)

3. **Test Song Change (Canal 3)** : Frapper tom/crash
   - ✅ Affichage : "Changement de chanson: IDO ENTROIDO - SONG2"
   - ✅ Reset des indices bass/melody

4. **Test Part Change (Canal 4)** : Frapper ride/hi-hat
   - ✅ Affichage : "Changement de partie: partie 2"  
   - ✅ Reset des indices bass/melody

### Debug et dépannage

#### Problèmes courants

| Problème | Diagnostic | Solution |
|----------|------------|----------|
| "VirMIDI 2-0 not found" | `make midi-ports` | `sudo modprobe snd-virmidi midi_devs=1` |
| "Cannot open ALSA sequencer" | Permissions | `sudo usermod -a -G audio $USER` puis logout/login |
| "cJSON missing" | Dépendance manquante | `sudo pacman -S cjson` |
| Pas de réaction aux notes MIDI | Bitwig mal configuré | Vérifier canaux HW Instrument (0-4) |
| Notes MIDI incorrectes | Fichier JSON | Vérifier format et valeurs numériques |

#### Commandes de diagnostic

```bash
# Voir l'état des modules audio
lsmod | grep snd

# Voir tous les ports MIDI avec détails
aconnect -l

# Voir les connexions actives uniquement
aconnect -o -l

# Surveiller un port spécifique
aseqdump -p 24:0

# Créer/supprimer une connexion manuelle
aconnect 14:0 24:0      # Créer
aconnect -d 14:0 24:0   # Supprimer

# Debug avec gdb (version debug)
make debug
gdb ./basic-frendo
(gdb) run
```

## 🔧 Développement et personnalisation

### Créer un nouveau set

1. **Créer un nouveau répertoire** :
```bash
mkdir sets/mon-nouveau-set
```

2. **Créer vos fichiers .frendo** :
```bash
nano sets/mon-nouveau-set/chanson1.frendo
```

Exemple de contenu :
```frendo
INPUTS
  kick = channel:0
  snare = channel:1

PART
  CAT[kick]: 36 38 40 42 | 48 50 52 55
  MS20[snare]: 60 62 64 67

PART
  CAT[kick]: 48 50 52 55
  HAPINESSQUARE[kick]: 72 74 76 79
```

3. **Lancer Basic Frendo** et sélectionner votre nouveau set dans le menu

## � Déploiement multi-distribution

### Installation locale dans le système

```bash
# Compilation optimisée pour la production
make clean && make

# Installation dans /usr/local/bin (recommandé)
sudo make install

# ou copie manuelle
sudo cp basic-frendo /usr/local/bin/
sudo chmod +x /usr/local/bin/basic-frendo
```

### Configuration permanente VirMIDI

```bash
# Rendre VirMIDI permanent au boot
echo "snd-virmidi midi_devs=1" | sudo tee -a /etc/modules-load.d/virmidi.conf

# Charger immédiatement
sudo modprobe snd-virmidi midi_devs=1

# Vérifier le chargement
lsmod | grep virmidi
```

### Utilisation

```bash
# Lancer Basic Frendo
./basic-frendo

# Le programme affiche un menu interactif pour sélectionner le set :
Available sets:
  1. ido-entroido-2025 (3 songs)
  2. tuning (1 song)

Select set [1-2]: 1

# Le set complet est chargé en mémoire
# Changement de chanson : canal MIDI 2, note 48
# Changement de partie : canal MIDI 2, note 49
```

### Workflow de développement Arch

```bash
# 1. Clone/édition du projet
git clone <repo> && cd basic-frendo

# 2. Installation des dépendances
sudo pacman -S base-devel alsa-lib alsa-utils

# 3. Configuration VirMIDI
sudo modprobe snd-virmidi midi_devs=1

# 4. Test et développement
make && ./basic-frendo

# 5. Installation système (quand satisfait)
sudo make install
```

## �📊 Performance et optimisation

### Latence mesurée

- **Version Node.js** : ~5-15ms (variable selon la charge système)
- **Version C** : <1ms (latence théorique ALSA ~0.1ms)

### Optimisations techniques

- **ALSA direct** : Bypass de toutes les couches d'abstraction
- **Polling à 1ms** : Équilibre performance/CPU  
- **Pas de malloc en runtime** : Mémoire statique uniquement
- **Compilation optimisée** : `-O2` par défaut, `-O3` possible
- **Linkage statique** : Déploiement sans dépendances

### Monitoring des performances

```bash
# CPU usage en temps réel
top -p $(pgrep basic-frendo)

# Analyse mémoire (version debug)
valgrind --leak-check=full ./basic-frendo sets/ido-entroido-2025.json

# Profiling avec perf (avancé)  
sudo pacman -S perf
perf record ./basic-frendo sets/ido-entroido-2025.json
perf report

# Vérifier les dépendances dynamiques
ldd ./basic-frendo
```



## 📄 Licence et contribution

**Projet personnel** - Usage libre pour performances live et développement éducatif.

Le code est volontairement **pédagogique et commenté** pour faciliter l'apprentissage du C et d'ALSA.

---