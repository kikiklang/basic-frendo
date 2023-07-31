const jzz = require("jzz");
const { spaceHarrier } = require('./space-harrier.js')

const MIDI_STATUS_NOTE_ON = 144;
const MIDI_STATUS_NOTE_OFF = 160;

class MidiController {
    constructor() {
        this.current = {
            partIndex: 1,
            bassNoteIndex: 0,
            melodyNoteIndex: 0,
            kickNoteIndex: 0,
            snareNoteIndex: 0,
            percNoteIndex: 0,
        };
    }

    /**
     * Initialise le contrôleur MIDI.
     */
    async init() {
        try {
            this.midiAccess = await jzz();
            this.getAvailableMidiPorts();
            await this.selectPorts();
            await this.initializeMidiPorts();
            console.log(`READY TO PLAY------------------------------`);
            this.connectMidiIn();
        } catch (error) {
            console.error("Error accessing MIDI:", error);
        }
    }

    /**
     * Affiche les ports MIDI disponibles.
     */
    getAvailableMidiPorts() {
        console.log(`AVAILABLE MIDI PORTS------------------------------`);
        this.midiAccess.info().inputs.forEach((port, index) => console.log(`Input Port ${index}: ${port.name}`));
        this.midiAccess.info().outputs.forEach((port, index) => console.log(`Output Port ${index}: ${port.name}`));
    }

    /**
     * Sélectionne les ports MIDI pour les entrées et les sorties.
     */
    async selectPorts() {
        console.log(`SELECTING PORTS------------------------------`);
        this.inputPortName = this.midiAccess.info().inputs[2].name;
        this.outputPortName = this.midiAccess.info().outputs[1].name;
        console.log(`Selected input port: ${this.inputPortName}`);
        console.log(`Selected output port: ${this.outputPortName}`);
    }

    /**
     * Initialise les ports MIDI pour les entrées et les sorties.
     */
    async initializeMidiPorts() {
        this.midiIn = await this.midiAccess.openMidiIn(this.inputPortName);
        this.midiOut = await this.midiAccess.openMidiOut(this.outputPortName);
    }

    /**
     * Connecte le port MIDI d'entrée pour recevoir les messages.
     */
    connectMidiIn() {
        this.midiIn.connect(async (msg) => {
            let status = msg[0];
            let channel = status & 0x0F;
            let key = msg["1"];
            let velocity = msg["2"];
            
            if (status >= MIDI_STATUS_NOTE_ON && status < MIDI_STATUS_NOTE_OFF) {
                console.log(`midi in | channel ${channel} | key ${key} | vel ${velocity}`);
                await this.playMidiNotes(channel);
            }
        });
    }

    /**
     * Joue les notes MIDI en fonction du canal.
     * @param {number} channel - Le canal MIDI.
     */
    async playMidiNotes(channel) {
        switch (channel) {
            case 0:
                await this.sendMidiNotes("kick", 2);
                await this.sendMidiNotes("snare", 3);
                await this.sendMidiNotes("bass", channel);
                console.log(`-------------------------------------------------------`);
                break;
            case 1:
                await this.sendMidiNotes("melody", channel);
                console.log(`-------------------------------------------------------`);
                break;
            case 2:
                await this.sendMidiNotes("perc", 4);
                console.log(`-------------------------------------------------------`);
                break;
            case 3:
                this.current.partIndex = this.updatePart();
                await this.sendMidiNotes("melody", 1);
                break;
            case 4:
                this.resetCurrent();
                break;
        }
    }

    /**
     * Envoie des notes MIDI à un canal spécifié.
     * @param {string} type - Le type de note à envoyer ("kick", "snare", "bass", "melody", "perc").
     * @param {number} channel - Le canal MIDI.
     */
    async sendMidiNotes(type, channel) {
        const fullNote = spaceHarrier[this.current.partIndex][type][this.current[`${type}NoteIndex`]];
        if (fullNote) {
            await this.midiOut.noteOn(channel, fullNote, 127);
            await this.midiOut.wait(50);
            await this.midiOut.noteOff(channel, fullNote, 0);
            console.log(`midi out | channel : ${channel + 1} | note : ${fullNote}`);
        } else {
            console.log(`No ${type} to play for this part`);
        }
        this.current[`${type}NoteIndex`]++;
        if (this.current[`${type}NoteIndex`] >= spaceHarrier[this.current.partIndex][type].length) {
            this.current[`${type}NoteIndex`] = 0;
        }
    }

    /**
     * Réinitialise les indices courants des notes.
     */
    resetCurrent() {
        this.current = {
            partIndex: 1,
            bassNoteIndex: 0,
            melodyNoteIndex: 0,
            kickNoteIndex: 0,
            snareNoteIndex: 0,
            percNoteIndex: 0,
        };
    }

    /**
     * Met à jour l'indice de la partie courante et réinitialise les indices des notes.
     * @returns {number} - L'indice de la partie courante.
     */
    updatePart() {
        this.current.partIndex++;
        if (!spaceHarrier[this.current.partIndex]) {
            this.current.partIndex = 1;
        }
        this.current.bassNoteIndex = 0;
        this.current.melodyNoteIndex = 0;
        this.current.snareNoteIndex = 0;
        this.current.kickNoteIndex = 0;
        this.current.percNoteIndex = 0;
        return this.current.partIndex;
    }
}

const midiController = new MidiController();
midiController.init();
