import jzz from 'jzz';
import inquirer from 'inquirer';
import fs from 'fs/promises';
import readline from 'readline/promises';
import path from 'path';

const MIDI_STATUS_NOTE_ON = 144;
const MIDI_STATUS_NOTE_OFF = 160;

let currentSongSet = {};
let current = {
    songIndex: 0,
    partIndex: 0,
    bassNoteIndex: 0,
    melodyNoteIndex: 0
};

async function selectSongFile() {
    const setsDir = path.join(process.cwd(), 'sets');
    const rl = readline.createInterface({
        input: process.stdin,
        output: process.stdout
    });

    try {
        const files = await fs.readdir(setsDir);

        const { songFile } = await inquirer.prompt([
            {
                type: 'list',
                name: 'songFile',
                message: 'Choisis le fichier de chansons Frendo:',
                choices: files
            }
        ]);

        const songFilePath = path.join(setsDir, songFile);
        console.log(`Selected song file path: ${songFilePath}`);

        await import(songFilePath)
            .then((module) => {
                currentSongSet = module.default;
            });

        current.songIndex = 0;
        current.partIndex = 0;
    } catch (err) {
        console.error("Could not list the directory or import the file.", err);
        process.exit(1);
    }
}

async function init() {
    try {
        const midiAccess = await jzz();
        const { inputPortName, outputPortName } = await selectPorts(midiAccess);
        const { midiIn, midiOut } = await initializeMidiPorts(midiAccess, inputPortName, outputPortName);
        console.log(`READY TO PLAY------------------------------`);
        connectMidiIn(midiIn, midiOut);
    } catch (error) {
        console.error("Error accessing MIDI:", error);
    }
}

async function selectPorts(midiAccess) {
    console.log(`SELECTING PORTS------------------------------`);

    const inputPort = midiAccess.info().inputs.find(input => input.name === 'VirMIDI 2-0');
    if (!inputPort) throw new Error(`Input port not found.`);

    const outputPort = midiAccess.info().outputs.find(output => output.name === 'VirMIDI 2-0');
    if (!outputPort) throw new Error(`Output port not found.`);

    console.log(`Selected input port: ${inputPort.name}`);
    console.log(`Selected output port: ${outputPort.name}`);
    return { inputPortName: inputPort.name, outputPortName: outputPort.name };
}

async function initializeMidiPorts(midiAccess, inputPortName, outputPortName) {
    const midiIn = await midiAccess.openMidiIn(inputPortName);
    const midiOut = await midiAccess.openMidiOut(outputPortName);
    return { midiIn, midiOut };
}

function connectMidiIn(midiIn, midiOut) {
    midiIn.connect(async (msg) => {
        let status = msg[0];
        let channel = status & 0x0F;
        let key = msg[1];
        
        if (status >= MIDI_STATUS_NOTE_ON && status < MIDI_STATUS_NOTE_OFF) {
            console.log(`midi in | channel ${channel} | key ${key}`);
            playMidiNotes(channel, midiOut);
        }
    });
}

function playMidiNotes(channel, midiOut) {
    switch (channel) {
        case 0:
            sendMidiNotes("bass", 0, midiOut);
            break;
        case 1:
            sendMidiNotes("melody", 1, midiOut);
            break;
        case 2:
            // current.partIndex = updatePart(); pour le trigger tap tap desormais dispo depuis l'arrivee du controller pourri
            break;
        case 3:
            current.songIndex = updateSong();
            break;
        case 4:
            current.partIndex = updatePart();
            break;
    }
}

async function sendMidiNotes(type, channel, midiOut) {
    const songKeys = Object.keys(currentSongSet);
    const currentSong = currentSongSet[songKeys[current.songIndex]];
    const partKey = (current.partIndex + 1).toString();
    const fullNote = currentSong[partKey][type][current[`${type}NoteIndex`]];

    if (fullNote) {
        await midiOut.noteOn(channel, fullNote, 127);
        await midiOut.wait(10);
        await midiOut.noteOff(channel, fullNote, 0);
        console.log(`midi out | channel : ${channel + 1} | type: ${type} | note : ${fullNote}`);
        console.log('---------------------------------------------')
    } 

    current[`${type}NoteIndex`]++;

    if (current[`${type}NoteIndex`] >= currentSong[partKey][type].length) {
        current[`${type}NoteIndex`] = 0;
    }
}

function updatePart() {
    const songKeys = Object.keys(currentSongSet);
    const currentSong = currentSongSet[songKeys[current.songIndex]];

    current.partIndex++;

    if (!currentSong[(current.partIndex + 1).toString()]) {
        current.partIndex = 0;
    }
    
    current.bassNoteIndex = 0;
    current.melodyNoteIndex = 0;

    console.log(`
╔═══════════════════════════════════════════╗
║ Changement de partie: partie ${(current.partIndex + 1).toString().padEnd(12)} ║
╚═══════════════════════════════════════════╝
`);

    return current.partIndex;
}

function updateSong() {
    const songKeys = Object.keys(currentSongSet);

    current.songIndex++;

    if (current.songIndex >= songKeys.length) {
        current.songIndex = 0;
    }

    current.partIndex = 0;
    current.bassNoteIndex = 0;
    current.melodyNoteIndex = 0;

    console.log(`
╔════════════════════════════════════════╗
║ Changement de chanson: ${songKeys[current.songIndex].padEnd(12)}    ║
╚════════════════════════════════════════╝
    `);

    return current.songIndex;
}

selectSongFile().then(() => init());