const jzz = require("jzz");
const { spaceHarrier } = require('./space-harrier.js')

let current = { 
    partIndex: 1, 
    bassNoteIndex: 0, 
    melodyNoteIndex: 0, 
    kickNoteIndex: 0, 
    snareNoteIndex: 0,
    percNoteIndex: 0, 
};


function getMidiIO(midiAccess, type) {
    console.log(`${type.toUpperCase()} MIDI DISPONIBLES------------------------------`);
    midiAccess.info()[type].forEach((port, index) => console.log(`Port ${index}: ${port.name}`));
}
/**
 * Envoie une note MIDI à un canal spécifié.
 *
 * @param {number} part - L'indice de la partie de la composition actuellement jouée.
 * @param {string} type - Le type de note à jouer ("bass", "melody", "chords", "snare", etc.)
 * @param {Object} midiOut - L'objet de sortie MIDI vers lequel la note doit être envoyée.
 * @param {number} channel - Le canal MIDI sur lequel la note doit être envoyée.
 */
async function sendMidiNote(part, type, midiOut, channel) {
    const fullNote = spaceHarrier[part][type][current[`${type}NoteIndex`]];

    if (fullNote) {
        await midiOut.noteOn(channel, fullNote, 127);
        await midiOut.wait(50);
        await midiOut.noteOff(channel, fullNote, 0);
        console.log(`midi out | channel : ${channel + 1} | note : ${fullNote}`);
    } else {
        console.log(`Pas de ${type} à jouer pour cette partie`);
    }

    current[`${type}NoteIndex`]++;
    if (current[`${type}NoteIndex`] >= spaceHarrier[part][type].length) {
        current[`${type}NoteIndex`] = 0;
    }
}

async function handleMidiMessages() {
    try {
        const midiAccess = await jzz();
        getMidiIO(midiAccess, "inputs");
        getMidiIO(midiAccess, "outputs");

        console.log(`SÉLECTION DE PORT------------------------------`);
        const inputPortName = midiAccess.info().inputs[2].name;
        const outputPortName = midiAccess.info().outputs[1].name;

        console.log(`Port d'entrée sélectionné : ${inputPortName}`);
        console.log(`Port de sortie sélectionné : ${outputPortName}`);

        const midiIn = await midiAccess.openMidiIn(inputPortName);
        const midiOut = await midiAccess.openMidiOut(outputPortName);

        console.log(`PRET  A JOUER------------------------------`);

        midiIn.connect(async (msg) => {
            let status = msg[0];
            let channel = status & 0x0F;
            let key = msg["1"];
            let velocity = msg["2"];
            
            
            if (status >= 144 && status < 160) { // status between 144 and 159 indicates noteOn message on some channel
                console.log(`midi in | channel ${channel} | key ${key} | vel ${velocity}`);
                switch (channel) {
                    case 0:
                        await sendMidiNote(current.partIndex, "kick", midiOut, 2);
                        await sendMidiNote(current.partIndex, "snare", midiOut, 3);
                        await sendMidiNote(current.partIndex, "bass", midiOut, channel);
                        console.log(`-------------------------------------------------------`);
                        break;
                    case 1:
                        await sendMidiNote(current.partIndex, "melody", midiOut, channel);
                        console.log(`-------------------------------------------------------`);
                        break;
                    case 2:
                        await sendMidiNote(current.partIndex, "perc", midiOut, 4);
                        console.log(`-------------------------------------------------------`);
                        break;
                    case 3:
                        current.partIndex = updatePart();
                        await sendMidiNote(current.partIndex, "melody", midiOut, 1);
                        break;
                    case 4:
                        resetCurrent();
                        break;
                }
            }
        });
        
    } catch (error) {
        console.error("Erreur lors de l'accès MIDI :", error);
    }
}

function resetCurrent() {
    current.partIndex = 1;
    current.bassNoteIndex = 0;
    current.melodyNoteIndex = 0;
    current.snareNoteIndex = 0;
    current.kickNoteIndex = 0;
    current.percNoteIndex = 0;
}

function updatePart() {
    current.partIndex++;
    if (!spaceHarrier[current.partIndex]) {
        current.partIndex = 1;
    }
    current.bassNoteIndex = 0;
    current.melodyNoteIndex = 0;
    current.snareNoteIndex = 0;
    current.kickNoteIndex = 0;
    current.percNoteIndex = 0;
    return current.partIndex;
}

handleMidiMessages();
