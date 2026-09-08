/*
 * tracker_display.c - Affichage tracker temps réel avec libfort
 */

#include "tracker_display.h"
#include "fort.h"
#include <stdio.h>
#include <string.h>

// Noms des notes MIDI
static const char* NOTE_NAMES[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

/**
 * Formate une note MIDI en nom de note + octave
 * Exemple: 60 -> "C4(60)", 255 -> "--"
 */
static void format_note(char *buf, uint8_t midi_note) {
    if (midi_note == 255) {
        strcpy(buf, "--");
    } else {
        int octave = (midi_note / 12) - 1;
        int note = midi_note % 12;
        snprintf(buf, 16, "%s%d(%d)", NOTE_NAMES[note], octave, midi_note);
    }
}

/**
 * Dessine une barre de progression ASCII (max 64 caractères, cycle si plus)
 * La largeur = nombre d'éléments (max 64)
 * Si total > 64, la barre cycle (wrap around)
 * Exemples:
 *   pos=3, total=8 -> "[###-----]"
 *   pos=65, total=128 -> "[#-------...] (cycle 2, position 1/64)"
 */
static void draw_progress_bar(char *buf, int pos, int total) {
    const int max_width = 64;
    int bar_width, display_pos;

    // Cas spécial: pas de séquence
    if (total == 0) {
        strcpy(buf, "[-]");
        return;
    }

    // Calculer largeur et position avec wrap
    bar_width = (total > max_width) ? max_width : total;
    display_pos = (bar_width == max_width) ? (pos % max_width) : pos;

    buf[0] = '[';
    for (int i = 0; i < bar_width; i++) {
        if (i < display_pos) {
            buf[i+1] = '#';  // Rempli
        } else {
            buf[i+1] = '-';  // Vide
        }
    }
    buf[bar_width+1] = ']';
    buf[bar_width+2] = '\0';
}

/**
 * Initialise l'affichage tracker
 */
void init_tracker_display(void) {
    // Clear screen et curseur home
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
}

/**
 * Met à jour l'affichage tracker avec libfort
 */
void update_tracker_display(song_set_t *song_set, frendo_state_t *state) {
    const song_t *song = &song_set->songs[state->song_index];
    const song_part_t *part = &song->parts[state->part_index];

    // Créer une nouvelle table
    ft_table_t *table = ft_create_table();

    // Configuration de la table
    ft_set_border_style(table, FT_DOUBLE2_STYLE);

    // Header avec info song/part
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_printf_ln(table, "BASIC FRENDO | Song: %s (%d/%d) | Part: %d/%d",
                 song->name, state->song_index+1, song_set->song_count,
                 state->part_index+1, song->part_count);

    // Ligne vide pour séparer
    ft_set_cell_prop(table, FT_CUR_ROW, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_write_ln(table, "TRACK", "IN->OUT", "POS", "NOTE", "PROGRESS");

    // Helper buffers
    char routing[16];
    char position[16];
    char note_str[16];
    char progress[67];  // [###...---] max 64 + '[' + ']' + '\0'

    // 4 tracks normales
    const track_t *tracks[] = {&part->BASS, &part->MS20, &part->SAMPLERVOICE, &part->SAMPLERFX};
    const char *track_names[] = {"BASS", "MS20", "SAMPLERVOICE", "SAMPLERFX"};
    const int track_indices[] = {state->bass_note_index, state->ms20_note_index,
                                  state->samplervoice_note_index, state->samplerfx_note_index};
    const int out_channels[] = {3, 4, 5, 6};

    for (int i = 0; i < 4; i++) {
        const track_t *track = tracks[i];
        snprintf(routing, sizeof(routing), "%d->%d", track->listen_channel, out_channels[i]);

        if (track->sequence.count == 0) {
            strcpy(position, "--/--");
            strcpy(note_str, "--");
            draw_progress_bar(progress, 0, 0);
        } else {
            snprintf(position, sizeof(position), "%02d/%d", track_indices[i], track->sequence.count);
            format_note(note_str, track->sequence.notes[track_indices[i]]);
            draw_progress_bar(progress, track_indices[i], track->sequence.count);
        }

        ft_write_ln(table, track_names[i], routing, position, note_str, progress);
    }

    // Blooper CC tracks actives
    for (int i = 0; i < part->blooper_cc_count; i++) {
        const cc_sequence_t *cc = &part->BLOOPER_CC[i];
        const char *cc_name = blooper_cc_number_to_name(cc->cc_number);

        snprintf(routing, sizeof(routing), "%d->%d", part->blooper_listen_channel, MIDI_OUT_BLOOPER);

        if (cc->count == 0) {
            strcpy(position, "--/--");
            strcpy(note_str, "--");
            draw_progress_bar(progress, 0, 0);
        } else {
            snprintf(position, sizeof(position), "%02d/%d", state->blooper_cc_index[i], cc->count);
            snprintf(note_str, sizeof(note_str), "%d", cc->values[state->blooper_cc_index[i]]);
            draw_progress_bar(progress, state->blooper_cc_index[i], cc->count);
        }

        ft_write_ln(table, cc_name, routing, position, note_str, progress);
    }

    // Afficher la table
    const char *table_str = ft_to_string(table);
    printf("\x1b[H");  // Cursor home
    printf("%s", table_str);
    fflush(stdout);

    // Libérer la table
    ft_destroy_table(table);
}

/**
 * Nettoie l'affichage tracker
 */
void cleanup_tracker_display(void) {
    // Restaurer écran normal
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
}
