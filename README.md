# Basic Frendo C

Séquenceur MIDI temps réel haute performance pour performances live de batterie.

## Flux du signal

```
Batterie → Carte son → Bitwig (Replacer) → VirMIDI → Basic Frendo C → VirMIDI → Bitwig → Synthés
```

## Installation (Arch Linux)

```bash
# Dépendances
sudo pacman -S base-devel alsa-lib alsa-utils

# VirMIDI
sudo modprobe snd-virmidi midi_devs=1

# Compilation
make
```

## Utilisation

```bash
./basic-frendo
```

Sélectionner un set depuis le menu interactif.

## Canaux MIDI

### Entrée (triggers)
- **Canal 0** : kick
- **Canal 1** : snare
- **Canal 2** : contrôle (note 48 = song suivante, note 49 = part suivante)

### Sortie (tracks)
- **BASS** → Canal 3
- **MS20** → Canal 4
- **HAPINESTRIANGLE** → Canal 5
- **HAPINESSQUARE** → Canal 6
- **SAMPLERVOICE** → Canal 7
- **SAMPLERFX** → Canal 8
- **BLOOPER CC** → Canal 9

## Format .frendo

```frendo
INPUTS
  kick = channel:0
  snare = channel:1

PART
#   BASS[kick]  MS20[snare]
01  36          60
02  38          62
03  40          64
```

### Syntaxe table
- `#` : ligne header (colonnes)
- `01`, `02`... : numéros de row
- Valeur MIDI : `36`, `127`, etc.
- `-` : skip (pas de note)
- `^` : répète la valeur du dessus
- `[36..42]` : range séquentiel
- `R[60..72]` : range randomisé
- `^` après range/random : prochaine valeur (boucle si épuisé)

### Tracks Blooper
Format : `BLOOPER<ccname>[input]`

Exemples :
- `BLOOPERSTOP[kick]` : CC 4 (stop)
- `BLOOPERPLAY[kick]` : CC 2 (play)
- `BLOOPERMODA_VAL[kick]` : CC 17 (modulation A)

CC disponibles : stop, play, record, overdub, undo, redo, erase, hold, switchb, volume, layers, repeats, moda_val, stability, modb_val, ramp, moda_mode, loop_mode, modb_mode, save_mode, moda, modb, clock_ign, ramp_onof, note_div, expr

## Debug

```bash
# Ports MIDI
make midi-ports

# Vérifier VirMIDI
make check-virmidi

# Nettoyer
make clean
```
