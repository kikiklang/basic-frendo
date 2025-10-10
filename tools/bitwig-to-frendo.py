#!/usr/bin/env python3
"""
bitwig-to-frendo.py - Convertisseur MIDI Bitwig vers Basic Frendo JSON

Usage:
    python3 bitwig-to-frendo.py bass.mid melody.mid --song "Ma Chanson" --output mon-set.json
    python3 bitwig-to-frendo.py --folder midi-exports/ --output complete-set.json
"""

import argparse
import json
import sys
from pathlib import Path

# Installer avec: pip install mido
try:
    import mido
except ImportError:
    print("❌ Module 'mido' requis. Installez avec: pip install mido")
    sys.exit(1)

def midi_to_notes(midi_file):
    """Extrait les notes MIDI d'un fichier et retourne une liste de valeurs MIDI"""
    notes = []
    
    try:
        mid = mido.MidiFile(midi_file)
        
        for track in mid.tracks:
            for msg in track:
                # Ne garder que les messages NOTE ON avec vélocité > 0
                if msg.type == 'note_on' and msg.velocity > 0:
                    notes.append(msg.note)
                    
    except Exception as e:
        print(f"❌ Erreur lors de la lecture de {midi_file}: {e}")
        return []
    
    return notes

def convert_midi_files(bass_file=None, melody_file=None, song_name="Untitled"):
    """Convertit des fichiers MIDI en structure Basic Frendo"""
    
    part = {}
    
    # Traiter le fichier bass
    if bass_file and Path(bass_file).exists():
        bass_notes = midi_to_notes(bass_file)
        part["bass"] = bass_notes
        print(f"✅ Bass: {len(bass_notes)} notes extraites de {bass_file}")
    else:
        part["bass"] = []
        if bass_file:
            print(f"⚠️ Fichier bass non trouvé: {bass_file}")
    
    # Traiter le fichier melody  
    if melody_file and Path(melody_file).exists():
        melody_notes = midi_to_notes(melody_file)
        part["melody"] = melody_notes
        print(f"✅ Melody: {len(melody_notes)} notes extraites de {melody_file}")
    else:
        part["melody"] = []
        if melody_file:
            print(f"⚠️ Fichier melody non trouvé: {melody_file}")
    
    # Créer la structure de chanson
    song = {
        "name": song_name,
        "parts": [part]
    }
    
    return song

def convert_folder(folder_path, pattern="*.mid"):
    """Convertit tous les fichiers MIDI d'un dossier"""
    folder = Path(folder_path)
    if not folder.exists():
        print(f"❌ Dossier non trouvé: {folder_path}")
        return []
    
    midi_files = list(folder.glob(pattern))
    if not midi_files:
        print(f"❌ Aucun fichier MIDI trouvé dans {folder_path}")
        return []
    
    songs = []
    
    # Grouper les fichiers par paires (bass/melody) ou traiter individuellement
    for midi_file in midi_files:
        song_name = midi_file.stem
        
        # Détecter si c'est un fichier bass ou melody
        if "bass" in song_name.lower():
            # Chercher le fichier melody correspondant
            melody_name = song_name.replace("bass", "melody").replace("BASS", "MELODY")
            melody_file = folder / f"{melody_name}.mid"
            
            clean_name = song_name.replace("_bass", "").replace("-bass", "").replace("bass", "")
            song = convert_midi_files(str(midi_file), str(melody_file) if melody_file.exists() else None, clean_name)
            songs.append(song)
            
        elif "melody" in song_name.lower():
            # Skip, sera traité avec le bass correspondant
            continue
            
        else:
            # Fichier standalone
            song = convert_midi_files(str(midi_file), None, song_name)
            songs.append(song)
    
    return songs

def main():
    parser = argparse.ArgumentParser(description="Convertit des fichiers MIDI Bitwig en JSON Basic Frendo")
    
    # Mode fichiers individuels
    parser.add_argument("bass", nargs="?", help="Fichier MIDI pour la séquence bass")
    parser.add_argument("melody", nargs="?", help="Fichier MIDI pour la séquence melody") 
    parser.add_argument("--song", "-s", default="Untitled", help="Nom de la chanson")
    
    # Mode dossier
    parser.add_argument("--folder", "-f", help="Convertir tous les MIDI d'un dossier")
    
    # Sortie
    parser.add_argument("--output", "-o", default="frendo-set.json", help="Fichier JSON de sortie")
    parser.add_argument("--pretty", "-p", action="store_true", help="JSON formaté (plus lisible)")
    
    args = parser.parse_args()
    
    print("🎵 Bitwig to Basic Frendo Converter")
    print("=" * 40)
    
    songs = []
    
    if args.folder:
        # Mode dossier
        songs = convert_folder(args.folder)
    elif args.bass or args.melody:
        # Mode fichiers individuels
        song = convert_midi_files(args.bass, args.melody, args.song)
        songs = [song]
    else:
        print("❌ Spécifiez des fichiers MIDI ou utilisez --folder")
        parser.print_help()
        return
    
    if not songs:
        print("❌ Aucune chanson générée")
        return
    
    # Créer la structure JSON finale
    frendo_set = {"songs": songs}
    
    # Sauvegarder
    with open(args.output, 'w', encoding='utf-8') as f:
        if args.pretty:
            json.dump(frendo_set, f, indent=2, ensure_ascii=False)
        else:
            json.dump(frendo_set, f, ensure_ascii=False)
    
    print(f"\n✅ Conversion terminée!")
    print(f"📁 Fichier généré: {args.output}")
    print(f"🎵 {len(songs)} chanson(s) convertie(s)")
    
    # Statistiques
    total_parts = sum(len(song['parts']) for song in songs)
    print(f"📊 {total_parts} partie(s) au total")
    
    print(f"\n🚀 Utilisez avec: ./basic-frendo {args.output}")

if __name__ == "__main__":
    main()