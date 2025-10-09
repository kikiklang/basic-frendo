# Basic Frendo C - Guide d'installation Arch Linux

## Dépendances système

### Installation des bibliothèques nécessaires

```bash
# Mise à jour du système
sudo pacman -Syu

# Outils de développement (si pas déjà installés via base-devel)
sudo pacman -S base-devel

# Bibliothèques ALSA et JSON
sudo pacman -S alsa-lib cjson

# Utilitaires MIDI (pour les tests)
sudo pacman -S alsa-utils

# (Optionnel) Outils de debug et profiling
sudo pacman -S gdb valgrind perf
```

### Configuration du module VirMIDI

```bash
# Charger le module VirMIDI
sudo modprobe snd-virmidi midi_devs=1

# Vérifier que le module est chargé
lsmod | grep virmidi

# Rendre le chargement permanent
echo "snd-virmidi midi_devs=1" | sudo tee -a /etc/modules-load.d/virmidi.conf
```

## Compilation et installation

### 1. Récupération du code source

```bash
# Clone du repository (si applicable)
git clone <repo-url> basic-frendo
cd basic-frendo

# Ou extraction d'une archive
tar -xzf basic-frendo.tar.gz
cd basic-frendo
```

### 2. Vérification des dépendances

```bash
make deps
```

### 3. Compilation

```bash
# Compilation optimisée
make

# Test local
./basic-frendo sets/ido-entroido-2025.json
```

### 4. Installation système

```bash
# Installation dans /usr/local/bin
sudo make install

# Vérification de l'installation
which basic-frendo
basic-frendo --help 2>/dev/null || echo "Installation réussie"
```

## Configuration Arch spécifique

### Permissions audio

```bash
# Ajouter l'utilisateur au groupe audio (si nécessaire)
sudo usermod -a -G audio $USER

# Redémarrer la session ou :
newgrp audio
```

### Optimisations temps réel (optionnel)

Pour des performances encore meilleures en live :

```bash
# Installer le kernel temps réel (optionnel)
sudo pacman -S linux-rt linux-rt-headers

# Configurer les limites temps réel
echo "@audio - rtprio 95" | sudo tee -a /etc/security/limits.conf
echo "@audio - memlock unlimited" | sudo tee -a /etc/security/limits.conf

# Redémarrer pour appliquer
```

### Services et démarrage automatique

```bash
# Créer un service systemd pour VirMIDI (optionnel)
sudo tee /etc/systemd/system/virmidi.service << EOF
[Unit]
Description=VirMIDI Virtual MIDI Driver
After=sound.target

[Service]
Type=oneshot
ExecStart=/usr/bin/modprobe snd-virmidi midi_devs=1
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF

# Activer le service
sudo systemctl enable virmidi.service
sudo systemctl start virmidi.service
```

## Test et validation

### Test rapide

```bash
# Vérifier VirMIDI
make check-virmidi

# Lister les ports MIDI
make midi-ports

# Test de compilation
make test-compile

# Lancement avec monitoring
basic-frendo sets/ido-entroido-2025.json &
aseqdump -p 24:0
```

### Intégration Bitwig

1. **Ouvrir Bitwig Studio**
2. **Charger le projet** `basic frendo.bwproject`
3. **Configurer les canaux MIDI** dans les modules HW Instrument :
   - Canal 0 : Bass trigger
   - Canal 1 : Melody trigger  
   - Canal 3 : Song change
   - Canal 4 : Part change
4. **Ajuster les seuils** des modules Replacer
5. **Tester** en frappant la batterie

## Dépannage Arch spécifique

### Erreurs de compilation

```bash
# Si cjson non trouvé
sudo pacman -S cjson

# Si ALSA non trouvé  
sudo pacman -S alsa-lib

# Si erreurs de linkage
sudo pacman -S base-devel
```

### Problèmes MIDI

```bash
# VirMIDI non disponible
sudo modprobe snd-virmidi midi_devs=1

# Permissions insuffisantes
sudo usermod -a -G audio $USER
# Puis redémarrer la session

# Port MIDI introuvable
aconnect -l  # Lister tous les ports
```

### Performance

```bash
# Vérifier la charge CPU
htop -p $(pgrep basic-frendo)

# Optimiser la priorité (si kernel RT installé)
sudo chrt -f 80 basic-frendo sets/ido-entroido-2025.json

# Mesurer la latence audio
jack_delay  # Si JACK est utilisé avec Bitwig
```

## Utilisation quotidienne

### Workflow type

```bash
# 1. Boot du système (VirMIDI auto-chargé si service activé)

# 2. Lancement Basic Frendo
basic-frendo ~/Music/sets/ido-entroido-2025.json

# 3. Lancement Bitwig + projet basic frendo

# 4. Jam session ! 🎵
```

### Gestion des sets

```bash
# Organisation recommandée
mkdir -p ~/Music/basic-frendo-sets/
cp sets/*.json ~/Music/basic-frendo-sets/

# Lancement avec set spécifique
basic-frendo ~/Music/basic-frendo-sets/mon-nouveau-set.json
```

### Backup et versioning

```bash
# Sauvegarde des sets
tar -czf basic-frendo-sets-backup.tar.gz ~/Music/basic-frendo-sets/

# Versioning avec git
cd ~/Music/basic-frendo-sets/
git init
git add *.json
git commit -m "Mes sets Basic Frendo"
```

## Mise à jour

```bash
# Récupération des mises à jour
cd basic-frendo/
git pull  # Si repository git

# Recompilation et réinstallation
make clean
make
sudo make install

# Vérification
basic-frendo --version 2>/dev/null || echo "Mise à jour réussie"
```

---

**Configuration Arch Linux optimisée pour Basic Frendo C ! 🎵**