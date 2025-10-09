# Guide de test Basic Frendo C

## 🧪 Tests fonctionnels

### Prérequis
1. **VirMIDI chargé** : `sudo modprobe snd-virmidi midi_devs=1`
2. **Basic Frendo compilé** : `make`
3. **Ports MIDI visibles** : `make midi-ports`

### Test 1 : Lancement du programme

```bash
# Terminal 1 : Lancer Basic Frendo
./basic-frendo sets/ido-entroido-2025.json

# Terminal 2 : Surveiller la sortie MIDI
aseqdump -p 24  # Port VirMIDI
```

**Résultat attendu** :
- ✅ Chargement de 4 chansons IDO ENTROIDO
- ✅ Connexion ALSA réussie (client ID affiché)
- ✅ "Ready to play! Waiting for MIDI input..."

### Test 2 : Envoi de notes MIDI manuelles

Utiliser un contrôleur MIDI ou Bitwig connecté au port VirMIDI.

**Canaux à tester** :

| Canal | Action | Résultat attendu |
|-------|---------|------------------|
| 0 | NOTE ON | Déclenche note BASS suivante |
| 1 | NOTE ON | Déclenche note MELODY suivante |
| 3 | NOTE ON | Change de chanson (SONG1→SONG2→...) |
| 4 | NOTE ON | Change de partie (Part1→Part2→...) |

### Test 3 : Séquençage des notes

1. **Canal 0** (Bass) : Notes attendues pour SONG1, Part1
   ```
   61, 68, 61, 68, 61, 68, 61, 73, ...
   ```

2. **Canal 1** (Melody) : Notes attendues pour SONG1, Part1
   ```
   73, 68, 58 (puis loop)
   ```

### Test 4 : Navigation entre chansons

1. Envoyer plusieurs notes sur **canal 3**
2. Observer les messages :
   ```
   ╔═══════════════════════════════════════════╗
   ║ Changement de chanson: IDO ENTROIDO - SONG2 ║
   ╚═══════════════════════════════════════════╝
   ```

### Test 5 : Navigation entre parties

1. Aller à SONG3 ou SONG4 (qui ont 2 parties)
2. Envoyer des notes sur **canal 4**
3. Observer :
   ```
   ╔═══════════════════════════════════════════╗
   ║ Changement de partie: partie 2              ║
   ╚═══════════════════════════════════════════╝
   ```

## 🐛 Dépannage

### Problème : "MIDI port 'VirMIDI 2-0' not found"
```bash
sudo modprobe snd-virmidi midi_devs=1
make check-virmidi
```

### Problème : "Cannot open ALSA sequencer"
```bash
sudo usermod -a -G audio $USER
# Redémarrer la session
```

### Problème : Pas de sortie MIDI visible
1. Vérifier les connexions :
   ```bash
   aconnect -l
   ```
2. S'assurer qu'aseqdump écoute le bon port :
   ```bash
   aseqdump -p 24  # Port VirMIDI, pas Basic Frendo
   ```

### Problème : Notes MIDI incorrectes
1. Vérifier le fichier JSON :
   ```bash
   cat sets/ido-entroido-2025.json | head -20
   ```
2. Les valeurs doivent être des nombres MIDI (0-127), pas des chaînes

## ✅ Validation complète

**Liste de contrôle** :

- [ ] Compilation sans erreurs
- [ ] Chargement du fichier JSON réussi
- [ ] Connexion ALSA réussie
- [ ] Notes BASS déclenchées sur canal 0
- [ ] Notes MELODY déclenchées sur canal 1
- [ ] Changement de chanson sur canal 3
- [ ] Changement de partie sur canal 4
- [ ] Sortie MIDI visible dans aseqdump
- [ ] Logs détaillés affichés
- [ ] Arrêt propre avec Ctrl+C

## 📊 Performances

Pour mesurer la latence (bonus) :
```bash
# Utiliser un oscilloscope logique ou un outil de mesure MIDI
# La latence cible est < 1ms entre réception et émission
```

## 🔧 Debug avancé

### Logs détaillés
Modifier `CFLAGS` dans le Makefile : `-DDEBUG -g3`

### Analyse mémoire
```bash
make debug
valgrind --leak-check=full ./basic-frendo sets/ido-entroido-2025.json
```

### Profile performances
```bash
sudo apt install perf
perf record ./basic-frendo sets/ido-entroido-2025.json
perf report
```