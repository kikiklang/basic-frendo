# Workflow Bitwig → Basic Frendo

## 🎼 Composer dans Bitwig

### 1. Structure de projet Bitwig recommandée

```
Projet Bitwig "Basic Frendo Composer"
├── Track 1: Bass Sequence (notes graves)
├── Track 2: Melody Sequence (notes aigües)  
├── Track 3: Bass Sequence Part 2 (optionnel)
└── Track 4: Melody Sequence Part 2 (optionnel)
```

### 2. Composition des séquences

- **Piste Bass** : Notes de basse, accords, lignes de bass
- **Piste Melody** : Mélodie principale, lead, arpèges
- **Timing** : Pas d'importance, seul l'ordre des notes compte
- **Longueur** : Autant de notes que voulu (Basic Frendo bouclera automatiquement)

## 📤 Export depuis Bitwig

### Méthode 1 : Export MIDI individuel

```bash
# Dans Bitwig, pour chaque piste :
1. Solo la piste Bass
2. File → Export → Export Audio/MIDI → MIDI
3. Nom : "ma-chanson-bass.mid"
4. Répéter pour Melody : "ma-chanson-melody.mid"
```

### Méthode 2 : Export par glisser-déposer

```bash
# Sélectionner les clips MIDI dans Bitwig
# Les glisser vers le dossier midi-exports/
```

## 🔄 Conversion automatique

### Setup initial

```bash
# Installer la dépendance Python
pip install mido

# Créer le dossier d'export
mkdir midi-exports

# Placer les fichiers MIDI de Bitwig dans midi-exports/
```

### Conversion simple (recommandé)

```bash
# Méthode automatique
python3 quick-convert.py

# Résultat : sets/bitwig-export.json
# Lancement : ./basic-frendo sets/bitwig-export.json
```

### Conversion avancée

```bash
# Fichiers individuels
python3 bitwig-to-frendo.py bass.mid melody.mid --song "Ma Chanson" --output mon-set.json

# Dossier complet  
python3 bitwig-to-frendo.py --folder midi-exports/ --output complete-set.json --pretty

# Multiples chansons
python3 bitwig-to-frendo.py --folder midi-exports/ --output live-set.json
```

## 🎯 Exemple complet

### Dans Bitwig

1. **Créer un nouveau projet**
2. **Composer la séquence bass** sur Track 1 :
   ```
   Notes : C2, G2, C2, F2, C2, G2, A2, C3
   ```
3. **Composer la mélodie** sur Track 2 :
   ```
   Notes : C4, E4, G4, C5, G4, E4, C4, A4
   ```
4. **Exporter** :
   - Track 1 → `song1-bass.mid`
   - Track 2 → `song1-melody.mid`

### Conversion

```bash
# Placer les fichiers dans midi-exports/
mv song1-*.mid midi-exports/

# Convertir
python3 quick-convert.py

# Tester
./basic-frendo sets/bitwig-export.json
```

### Résultat JSON généré

```json
{
  "songs": [
    {
      "name": "Song1", 
      "parts": [
        {
          "bass": [36, 43, 36, 41, 36, 43, 45, 48],
          "melody": [60, 64, 67, 72, 67, 64, 60, 69]
        }
      ]
    }
  ]
}
```

## 🚀 Workflow de production

### 1. Composition continue
```bash
# Composer dans Bitwig
# Exporter régulièrement les pistes
# Conversion automatique avec quick-convert.py
```

### 2. Test en live  
```bash
# Test immédiat
./basic-frendo sets/bitwig-export.json

# Ajustement dans Bitwig si nécessaire
# Re-export → re-conversion → re-test
```

### 3. Gestion des versions
```bash
# Archiver les versions
cp sets/bitwig-export.json sets/live-$(date +%Y%m%d).json

# Git versioning
git add sets/*.json
git commit -m "Update live set from Bitwig"
```

## ⚡ Workflow ultra-rapide

```bash
# 1. Composer dans Bitwig
# 2. Export drag & drop vers midi-exports/
# 3. Terminal :
python3 quick-convert.py && ./basic-frendo sets/bitwig-export.json
```

**En 3 étapes, vos compositions Bitwig sont prêtes pour le live !** 🎵