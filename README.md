# BASIC FRENDO

## Process de gestion du midi

Le concept de base a été developpé dans le projet bitwig `basic Frendo.bwproject`.

1. Quatre pièzos sont branchées et relient les éléments de la batterie à la carte son.
2. La carte son amplifie le signal audio avant de l'envoyer à Bitwig.
3. Dans Bitwig, nous retrouvons les quatre pistes (projet basic frendo.bwproject). Chaque piste reçoit un module "replacer" qui déclenche un signal vers un autre module (HW instrument )chaque fois qu'un seuil de gain est dépassé.
4. le module HW instrument se charge de créer un signal midi de sortie sur un canal defini par l'utilisateur (de 1 a 16)
5. le signal midi de sortie de bitwig est capté sur un port virtuel que l'on crée dans linux
6. Le programme jzz-midi recoit le signal transmis par linux et emet un nouveau signal midi sur un canal defini avec une note defini

## lancement

1. ouvrir le projet jzz-midi dans vs-code
2. ouvrir un premier terminal et lancer la commande pour verifier que des ports midi virtuels ne sont pas deja crées

```
aseqdump -l
```

3. si oui, créer 2 paires de ports midi virtuels

```
sudo modprobe snd-virmidi
```

4. le resultat est le suivant :

```
 20:0    Virtual Raw MIDI 1-0             VirMIDI 1-0
 21:0    Virtual Raw MIDI 1-1             VirMIDI 1-1
 22:0    Virtual Raw MIDI 1-2             VirMIDI 1-2
 23:0    Virtual Raw MIDI 1-3             VirMIDI 1-3
```

5. dans un nouveau terminal, se mettre en surveillance des ports virtuels et surveiller le flux midi qui arrive de bitwig :

```
aseqdump -p 21:0 // valeur peut varier
```

6. dans un nouveau terminal, se mettre en surveillance des ports virtuels et surveiller le flux midi qui part vers bitwig :

```
aseqdump -p 20:0 // valeur peut varier
```

7. ouvrir bitwig et ouvrir la console bitwig pour verifier que le script controller basic frendo est bien chargé

```
Ctrl+shift+j
```

8. verifier les entrées de la carte son, regler les volumes

9. lancer le programme jzz-midi avec node ou bun

```
bun --watch index.js
nodemon index.js
```

## Diverses commandes

| Action                                        | Commande                    |
| --------------------------------------------- | --------------------------- |
| Ouvrir la console Bitwig                      | `Ctrl+shift+j`              |
| Voir tous les ports MIDI disponibles          | `aseqdump -l`               |
| Surveiller le flux sur un port donné          | `aseqdump -p 24:0`          |
| Créer des paires de ports MIDI virtuels       | `sudo modprobe snd-virmidi` |
| Voir les connexions MIDI actuellement actives | `aconnect -o -l`            |
| Créer une liaison entre deux ports donnés     | `aconnect 14:0 24:0`        |
| Supprimer une liaison entre deux ports donnés | `aconnect -d 14:0 24:0`     |