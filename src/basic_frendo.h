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
#define MIDI_STATUS_NOTE_ON  0x90
#define MIDI_STATUS_NOTE_OFF 0x80
#define MIDI_VELOCITY        127

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

// Structure pour une track avec routing MIDI
typedef struct {
    note_sequence_t sequence;    // Séquence de notes
    uint8_t listen_channel;      // Canal MIDI d'entrée qui déclenche cette track
} track_t;

// Structure pour une partie d'une chanson
typedef struct {
    track_t CAT;              // Séquence CAT (canal MIDI out 3)
    track_t MS20;             // Séquence MS20 (canal MIDI out 4)
    track_t HAPINESTRIANGLE;  // Séquence HAPINESTRIANGLE (canal MIDI out 5)
    track_t HAPINESSQUARE;    // Séquence HAPINESSQUARE (canal MIDI out 6)
    track_t SAMPLERVOICE;     // Séquence SAMPLERVOICE (canal MIDI out 7)
    track_t SAMPLERFX;        // Séquence SAMPLERFX (canal MIDI out 8)
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
    int cat_note_index;              // Position dans la séquence CAT
    int ms20_note_index;             // Position dans la séquence MS20
    int hapinestriangle_note_index;  // Position dans la séquence HAPINESTRIANGLE
    int hapinessquare_note_index;    // Position dans la séquence HAPINESSQUARE
    int samplervoice_note_index;     // Position dans la séquence SAMPLERVOICE
    int samplerfx_note_index;        // Position dans la séquence SAMPLERFX
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
void cleanup_midi_interface(midi_interface_t *midi);

// frendo_core.c
void reset_note_indices(frendo_state_t *state);
void update_song(frendo_state_t *state, const song_set_t *song_set);
void update_part(frendo_state_t *state, const song_set_t *song_set);
void play_CAT_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_MS20_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_HAPINESTRIANGLE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_HAPINESSQUARE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_SAMPLERVOICE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);
void play_SAMPLERFX_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state);

// utils.c
const char* error_to_string(frendo_error_t error);
void print_banner(void);
void print_state_change(void);

#endif // BASIC_FRENDO_H