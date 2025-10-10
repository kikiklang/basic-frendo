# CONTRIBUTING.md

Guide de contribution pour Basic Frendo C.

## 🏗️ Structure du code

### Organisation des modules

- **Nouveau module** : Un fichier `.c` + déclarations dans `.h`
- **Fonctions publiques** : Déclarées dans `basic_frendo.h`
- **Fonctions privées** : `static` dans le fichier source
- **Nommage** : `snake_case` pour fonctions, `PascalCase` pour types

### Style de code

```c
// ✅ Bon style
static frendo_error_t parse_note_sequence(cJSON *json_array, note_sequence_t *sequence) {
    if (!json_array || !sequence) {
        return FRENDO_ERROR_JSON;
    }
    
    // Implementation...
    return FRENDO_OK;
}

// ❌ Éviter
static int parseNoteSeq(void* json,void* seq){
if(!json)return -1;
//...
}
```

### Gestion d'erreurs

- Toujours vérifier les paramètres d'entrée
- Utiliser les codes d'erreur `frendo_error_t`
- Messages d'erreur explicites avec `printf`
- Cleanup des ressources en cas d'erreur

## 🧪 Tests

### Compilation

```bash
make clean && make        # Build complet
make test-compile        # Test compilation uniquement  
make analyze            # Analyse statique (cppcheck)
```

### Tests fonctionnels

```bash
make                                    # Build
./basic-frendo sets/ido-entroido-2025.json  # Test basic
# Ctrl+C pour arrêter
```

### Workflow Bitwig

```bash
make bitwig-setup       # Setup initial
make bitwig-convert     # Test conversion
make bitwig-test        # Test complet
```

## 📝 Documentation

### Code

- **Commentaires de fonction** : Objectif, paramètres, retour
- **Sections importantes** : `// === SECTION ===`
- **TODOs** : `// TODO: Description`
- **Warnings** : `// WARNING: Condition critique`

### Fichiers

- **README.md** par dossier important
- **Guides d'usage** dans `docs/`
- **Exemples concrets** dans la documentation

## 🚀 Workflow de développement

### Nouvelles fonctionnalités

1. **Planifier** : Définir l'objectif et l'architecture
2. **Implémenter** : Code + tests
3. **Documenter** : Mise à jour des guides
4. **Tester** : Compilation + tests fonctionnels
5. **Commiter** : Messages explicites

### Corrections de bugs

1. **Reproduire** : Créer un cas de test
2. **Identifier** : Debug avec gdb si nécessaire
3. **Corriger** : Fix minimal et ciblé
4. **Vérifier** : Re-test du cas original
5. **Documenter** : Mise à jour si nécessaire

### Messages de commit

```bash
# ✅ Bons messages
feat: add hot-reload for JSON files
fix: resolve ALSA connection timeout
docs: update Bitwig workflow guide
refactor: simplify MIDI message parsing

# ❌ À éviter  
update stuff
fix bug
changes
```

## 🎯 Priorités du projet

### Performance
- Latence MIDI < 1ms
- Pas d'allocation en runtime
- Polling optimisé

### Fiabilité
- Gestion d'erreurs robuste
- Recovery automatique
- Pas de crash en live

### Simplicité
- Code lisible et maintenant
- Documentation claire
- Workflow intuitif

### Extensibilité
- Architecture modulaire
- API claire entre modules
- Configuration flexible