/* 
 * basic_frendo.h - Header principal pour Basic Frendo
 * Version C pour performances MIDI optimisées
 */

#ifndef BASIC_FRENDO_H
#define BASIC_FRENDO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

// Constantes MIDI
#define MIDI_STATUS_NOTE_ON     0x90
#define MIDI_STATUS_NOTE_OFF    0x80
#define MIDI_STATUS_CONTROL_CHANGE 0xB0
#define MIDI_VELOCITY           127

// Blooper CC mapping (Chase Bliss Blooper MIDI CC numbers)
typedef enum {
    BLOOPER_CC_RECORD = 1,
    BLOOPER_CC_PLAY = 2,
    BLOOPER_CC_OVERDUB = 3,
    BLOOPER_CC_STOP = 4,
    BLOOPER_CC_UNDO = 5,
    BLOOPER_CC_REDO = 6,
    BLOOPER_CC_ERASE = 7,
    BLOOPER_CC_HOLD = 8,
    BLOOPER_CC_SWITCHB = 11,
    BLOOPER_CC_VOLUME = 14,
    BLOOPER_CC_LAYERS = 15,
    BLOOPER_CC_REPEATS = 16,
    BLOOPER_CC_MODA_VAL = 17,
    BLOOPER_CC_STABILITY = 18,
    BLOOPER_CC_MODB_VAL = 19,
    BLOOPER_CC_RAMP = 20,
    BLOOPER_CC_MODA_MODE = 21,
    BLOOPER_CC_LOOP_MODE = 22,
    BLOOPER_CC_MODB_MODE = 23,
    BLOOPER_CC_SAVE_MODE = 24,
    BLOOPER_CC_MODA = 30,
    BLOOPER_CC_MODB = 31,
    BLOOPER_CC_CLOCK_IGN = 51,
    BLOOPER_CC_RAMP_ONOF = 52,
    BLOOPER_CC_NOTE_DIV = 54,
    BLOOPER_CC_EXPR = 100
} blooper_cc_t;

// Limites du système
#define MAX_SONGS           50
#define MAX_PARTS_PER_SONG  50
#define MAX_NOTES_PER_SEQ   100
#define MAX_NAME_LENGTH     256
#define MAX_FILENAME_LENGTH 512

// Codes d'erreur
typedef enum {
    FRENDO_OK = 0,
    FRENDO_ERROR_FILE,
    FRENDO_ERROR_JSON,
    FRENDO_ERROR_MIDI,
    FRENDO_ERROR_MEMORY,
    FRENDO_ERROR_ALSA
} frendo_error_t;

// Structure pour une séquence de notes MIDI
typedef struct {
    uint8_t notes[MAX_NOTES_PER_SEQ];  // Valeurs MIDI (0-127)
    int count;                         // Nombre de notes dans la séquence
} note_sequence_t;

// Structure pour une séquence de valeurs CC (Control Change)
typedef struct {
    uint8_t values[MAX_NOTES_PER_SEQ];  // Valeurs CC (0-127), 255 = skip
    int count;                          // Nombre de valeurs dans la séquence
    uint8_t cc_number;                  // Numéro du CC (ex: 15 pour LAYERS)
} cc_sequence_t;

// Structure pour une track avec routing MIDI
typedef struct {
    note_sequence_t sequence;    // Séquence de notes
    uint8_t listen_channel;      // Canal MIDI d'entrée qui déclenche cette track
} track_t;

// Structure pour une partie d'une chanson
typedef struct {
    track_t BASS;              // Séquence BASS (canal MIDI out 3)
    track_t MS20;             // Séquence MS20 (canal MIDI out 4)
    track_t HAPINESTRIANGLE;  // Séquence HAPINESTRIANGLE (canal MIDI out 5)
    track_t HAPINESSQUARE;    // Séquence HAPINESSQUARE (canal MIDI out 6)
    track_t SAMPLERVOICE;     // Séquence SAMPLERVOICE (canal MIDI out 7)
    track_t SAMPLERFX;        // Séquence SAMPLERFX (canal MIDI out 8)

    // Blooper CC sequences (up to 10 different CC tracks per part)
    cc_sequence_t BLOOPER_CC[10];  // Array of CC sequences
    int blooper_cc_count;          // Number of active Blooper CC tracks
    uint8_t blooper_listen_channel; // Channel that triggers Blooper CC
} song_part_t;

// Structure pour une chanson complète
typedef struct {
    char name[MAX_NAME_LENGTH];        // Nom de la chanson
    song_part_t parts[MAX_PARTS_PER_SONG]; // Parties de la chanson
    int part_count;                    // Nombre de parties
} song_t;

// Structure pour un set complet de chansons
typedef struct {
    song_t songs[MAX_SONGS];           // Tableau de chansons
    int song_count;                    // Nombre de chansons
} song_set_t;

// État global du système
typedef struct {
    int song_index;                  // Index de la chanson courante
    int part_index;                  // Index de la partie courante
    int bass_note_index;              // Position dans la séquence BASS
    int ms20_note_index;             // Position dans la séquence MS20
    int hapinestriangle_note_index;  // Position dans la séquence HAPINESTRIANGLE
    int hapinessquare_note_index;    // Position dans la séquence HAPINESSQUARE
    int samplervoice_note_index;     // Position dans la séquence SAMPLERVOICE
    int samplerfx_note_index;        // Position dans la séquence SAMPLERFX
    int blooper_cc_index[10];        // Position dans les séquences CC Blooper (max 10)
} frendo_state_t;

// Structure pour l'interface ALSA
typedef struct {
    snd_seq_t *seq_handle;     // Handle du séquenceur ALSA
    int input_port;            // Port d'entrée MIDI
    int output_port;           // Port de sortie MIDI
    int client_id;             // ID client ALSA
} midi_interface_t;

// Fonctions principales (définies dans leurs fichiers respectifs)

// frendo_parser.c
frendo_error_t load_frendo_set(const char *directory, song_set_t *song_set);
int list_available_sets(const char *sets_dir, char set_names[][MAX_NAME_LENGTH], int max_sets);
void print_song_set_info(const song_set_t *song_set);

// midi_handler.c
frendo_error_t init_midi_interface(midi_interface_t *midi);
frendo_error_t setup_midi_ports(midi_interface_t *midi, const char *port_name);
void process_midi_message(const snd_seq_event_t *event,
                         midi_interface_t *midi,
                         song_set_t *song_set,
                         frendo_state_t *state);
void send_midi_note(midi_interface_t *midi, uint8_t channel, uint8_t note, const char *track_name, int note_index, int total_notes);
void send_midi_cc(midi_interface_t *midi, uint8_t channel, uint8_t cc_number, uint8_t value, const char *cc_name, int value_index, int total_values);
void cleanup_midi_interface(midi_interface_t *midi);

// frendo_core.c
void reset_note_indices(frendo_state_t *state);
void update_song(frendo_state_t *state, const song_set_t *song_set);
void update_part(frendo_state_t *state, const song_set_t *song_set);
void play_BASS_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_MS20_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_HAPINESTRIANGLE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_HAPINESSQUARE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_SAMPLERVOICE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_SAMPLERFX_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_BLOOPER_cc(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);

// Helper functions for Blooper CC
int blooper_cc_name_to_number(const char *cc_name);

// utils.c
const char* error_to_string(frendo_error_t error);
void print_banner(void);
void print_state_change(void);

#endif // BASIC_FRENDO_H