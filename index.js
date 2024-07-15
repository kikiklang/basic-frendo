import jzz from 'jzz';
import inquirer from 'inquirer';
import fs from 'fs/promises';
import readline from 'readline/promises';
import path from 'path';

const MIDI_STATUS_NOTE_ON = 144;
const MIDI_STATUS_NOTE_OFF = 160;

let currentSong = {};
let current = {
    partIndex: 1,
    bassNoteIndex: 0,
    melodyNoteIndex: 0
};

async function selectSong() {
    const songsDir = path.join(process.cwd(), 'songs');
    const rl = readline.createInterface({
        input: process.stdin,
        output: process.stdout
    });

    try {
        const files = await fs.readdir(songsDir);

        const { song } = await inquirer.prompt([
            {
                type: 'list',
                name: 'song',
                message: 'Choisis la chanson Frendo:',
                choices: files
            }
        ]);

        await import(path.join(songsDir, song))
            .then((module) => {
                currentSong = module.default;
            });
    } catch (err) {
        console.error("Could not list the directory.", err);
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
    const inputPortName = midiAccess.info().inputs[2].name;
    const outputPortName = midiAccess.info().outputs[1].name;
    console.log(`Selected input port: ${inputPortName}`);
    console.log(`Selected output port: ${outputPortName}`);
    return { inputPortName, outputPortName };
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
        let key = msg["1"];
        
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
            current.partIndex = updatePart();
            sendMidiNotes("melody", 1, midiOut);
            break;
    }
}

async function sendMidiNotes(type, channel, midiOut) {
    const fullNote = currentSong[current.partIndex][type][current[`${type}NoteIndex`]];

    if (fullNote) {
        await midiOut.noteOn(channel, fullNote, 127);
        await midiOut.wait(5);
        await midiOut.noteOff(channel, fullNote, 0);
        console.log(`midi out | channel : ${channel + 1} | type: ${type} | note : ${fullNote}`);
        console.log('---------------------------------------------')
    } 

    current[`${type}NoteIndex`]++;

    if (current[`${type}NoteIndex`] >= currentSong[current.partIndex][type].length) {
        current[`${type}NoteIndex`] = 0;
    }
}

function updatePart() {
    current.partIndex++;

    if (!currentSong[current.partIndex]) {
        current.partIndex = 1;
    }
    
    current.bassNoteIndex = 0;
    current.melodyNoteIndex = 0;

    return current.partIndex;
}

async function selectSong() {
    const songsDir = path.join(process.cwd(), 'songs');
    const rl = readline.createInterface({
        input: process.stdin,
        output: process.stdout
    });

    try {
        const files = await fs.readdir(songsDir);

        const { song } = await inquirer.prompt([
            {
                type: 'list',
                name: 'song',
                message: 'Choisis la chanson Frendo:',
                choices: files
            }
        ]);

        await import(path.join(songsDir, song))
            .then((module) => {
                currentSong = module.default;
            });
    } catch (err) {
        console.error("Could not list the directory.", err);
        process.exit(1);
    }
}

selectSong()
    .then(() => init());
