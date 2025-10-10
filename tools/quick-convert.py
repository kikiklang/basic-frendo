#!/usr/bin/env python3
"""
quick-convert.py - Convertisseur rapide pour workflow Bitwig

Utilise une structure de dossier prédéfinie :
midi-exports/
├── song1-bass.mid
├── song1-melody.mid  
├── song2-bass.mid
├── song2-melody.mid
└── ...

Usage: python3 quick-convert.py
"""

import json
from pathlib import Path
import mido
import re

def extract_song_name(filename):
    """Extrait le nom de chanson à partir du nom de fichier"""
    # Enlever l'extension et les suffixes bass/melody
    name = Path(filename).stem
    name = re.sub(r'[-_](bass|melody|BASS|MELODY)$', '', name)
    return name.replace('-', ' ').replace('_', ' ').title()

def midi_to_notes(midi_path):
    """Convertit un fichier MIDI en liste de notes"""
    if not Path(midi_path).exists():
        return []
    
    notes = []
    try:
        mid = mido.MidiFile(midi_path)
        for track in mid.tracks:
            for msg in track:
                if msg.type == 'note_on' and msg.velocity > 0:
                    notes.append(msg.note)
    except:
        return []
    
    return notes

def auto_convert():
    """Conversion automatique du dossier midi-exports/"""
    
    exports_dir = Path("midi-exports")
    if not exports_dir.exists():
        print("❌ Créez le dossier 'midi-exports/' et placez vos fichiers MIDI dedans")
        print("📁 Structure attendue:")
        print("   midi-exports/")
        print("   ├── chanson1-bass.mid")
        print("   ├── chanson1-melody.mid")
        print("   ├── chanson2-bass.mid") 
        print("   └── chanson2-melody.mid")
        return
    
    # Grouper les fichiers par chanson
    songs_data = {}
    
    for midi_file in exports_dir.glob("*.mid"):
        filename = midi_file.name
        song_name = extract_song_name(filename)
        
        if song_name not in songs_data:
            songs_data[song_name] = {"bass": [], "melody": []}
        
        if "bass" in filename.lower():
            songs_data[song_name]["bass"] = midi_to_notes(midi_file)
            print(f"✅ {song_name} - Bass: {len(songs_data[song_name]['bass'])} notes")
        elif "melody" in filename.lower():
            songs_data[song_name]["melody"] = midi_to_notes(midi_file)
            print(f"✅ {song_name} - Melody: {len(songs_data[song_name]['melody'])} notes")
    
    # Créer la structure JSON
    songs = []
    for name, data in songs_data.items():
        song = {
            "name": name,
            "parts": [data]
        }
        songs.append(song)
    
    # Sauvegarder
    output_file = "sets/bitwig-export.json"
    Path("sets").mkdir(exist_ok=True)
    
    frendo_set = {"songs": songs}
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(frendo_set, f, indent=2, ensure_ascii=False)
    
    print(f"\n🎉 Conversion terminée!")
    print(f"📁 {len(songs)} chanson(s) → {output_file}")
    print(f"🚀 Lancez: ./basic-frendo {output_file}")

if __name__ == "__main__":
    print("🎵 Quick Bitwig → Basic Frendo Converter")
    print("=" * 45)
    auto_convert()