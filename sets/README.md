# sets/

Fichiers de configuration JSON pour les sets musicaux.

## Format

```json
{
  "songs": [
    {
      "name": "Nom de la chanson",
      "parts": [
        {
          "bass": [36, 38, 36, 42],
          "melody": [60, 62, 64, 67]  
        }
      ]
    }
  ]
}
```

## Organisation recommandée

- Un fichier par set de performance
- Nommage explicite : `live-2025-10-15.json`
- Versioning avec git pour l'historique