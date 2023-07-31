const fs = require('fs');
const midiFile = require('midi-file');

// Lire les données du fichier
const input = fs.readFileSync('space-harrier.mid');

// Analyser les données
const parsed = midiFile.parseMidi(input);

const result = parsed.tracks[0]
    .filter(obj => 'type' in obj && obj.type === 'noteOn')
    .map(obj => obj.noteNumber);

fs.writeFile('bass-for-space-harrier.js', JSON.stringify(result), (err) => {
    if (err) {
        console.error('Une erreur est survenue lors de l\'écriture du fichier:', err);
    } else {
        console.log('Fichier écrit avec succès!');
    }
});
    