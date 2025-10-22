/* 
 * frendo_core.c - Logique métier centrale de Basic Frendo
 * 
 * Implémente la logique de séquençage MIDI :
 * - Gestion des séquences bass/melody
 * - Navigation entre chansons et parties
 * - État du système
 */

#include "basic_frendo.h"

/**
 * Remet à zéro les indices de notes (tous les synthés)
 * Appelé lors du changement de chanson ou de partie
 */
void reset_note_indices(frendo_state_t *state) {
    if (!state) return;

    state->cat_note_index = 0;
    state->ms20_note_index = 0;
    state->hapinestriangle_note_index = 0;
    state->hapinessquare_note_index = 0;

    printf("[STATE] Note indices reset to 0\n");
}

/**
 * Passe à la chanson suivante dans le set
 * Remet à zéro la partie et les indices de notes
 */
void update_song(frendo_state_t *state, const song_set_t *song_set) {
    if (!state || !song_set || song_set->song_count == 0) {
        return;
    }
    
    // Incrémenter l'index de chanson
    state->song_index++;
    
    // Revenir au début si on dépasse le nombre de chansons
    if (state->song_index >= song_set->song_count) {
        state->song_index = 0;
    }
    
    // Remettre à zéro la partie et les indices
    state->part_index = 0;
    reset_note_indices(state);
    
    // Afficher le changement
    // plus besoin de song_name
    print_state_change();
    
    printf("[STATE] Song: %d/%d | Part: %d/%d\n", 
           state->song_index + 1, song_set->song_count,
           state->part_index + 1, song_set->songs[state->song_index].part_count);
}

/**
 * Passe à la partie suivante de la chanson courante
 * Remet à zéro les indices de notes
 */
void update_part(frendo_state_t *state, const song_set_t *song_set) {
    if (!state || !song_set || song_set->song_count == 0) {
        return;
    }
    
    const song_t *current_song = &song_set->songs[state->song_index];
    
    // Incrémenter l'index de partie
    state->part_index++;
    
    // Revenir au début si on dépasse le nombre de parties
    if (state->part_index >= current_song->part_count) {
        state->part_index = 0;
    }
    
    // Remettre à zéro les indices de notes
    reset_note_indices(state);
    
    // Afficher le changement
    // plus besoin de part_info
    print_state_change();
    
    printf("[STATE] Song: %d/%d | Part: %d/%d\n", 
           state->song_index + 1, song_set->song_count,
           state->part_index + 1, current_song->part_count);
}

/**
 * Joue la note CAT suivante de la séquence courante
 * Canal MIDI out 3 - Déclenché par les notes sur canal in 0
 */
void play_CAT_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    if (!midi || !song_set || !state) {
        return;
    }

    // Vérifier la validité des indices
    if (state->song_index >= song_set->song_count) {
        printf("[ERROR] Invalid song index: %d (max: %d)\n",
               state->song_index, song_set->song_count - 1);
        return;
    }

    const song_t *current_song = &song_set->songs[state->song_index];

    if (state->part_index >= current_song->part_count) {
        printf("[ERROR] Invalid part index: %d (max: %d)\n",
               state->part_index, current_song->part_count - 1);
        return;
    }

    const song_part_t *current_part = &current_song->parts[state->part_index];
    const note_sequence_t *cat_seq = &current_part->CAT;

    // Vérifier qu'il y a des notes dans la séquence
    if (cat_seq->count == 0) {
        printf("[WARNING] No CAT notes in current part\n");
        return;
    }

    // Obtenir la note courante
    uint8_t note = cat_seq->notes[state->cat_note_index];

    // Envoyer la note sur le canal MIDI out 3
    send_midi_note(midi, 3, note);

    // Avancer dans la séquence
    state->cat_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->cat_note_index >= cat_seq->count) {
        state->cat_note_index = 0;
        printf("[INFO] CAT sequence looped back to start\n");
    }

    printf("[STATE] CAT note index: %d/%d\n",
           state->cat_note_index, cat_seq->count);
}

/**
 * Joue la note MS20 suivante de la séquence courante
 * Canal MIDI out 4 - Déclenché par les notes sur canal in 1
 */
void play_MS20_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    if (!midi || !song_set || !state) {
        return;
    }

    // Vérifier la validité des indices
    if (state->song_index >= song_set->song_count) {
        printf("[ERROR] Invalid song index: %d (max: %d)\n",
               state->song_index, song_set->song_count - 1);
        return;
    }

    const song_t *current_song = &song_set->songs[state->song_index];

    if (state->part_index >= current_song->part_count) {
        printf("[ERROR] Invalid part index: %d (max: %d)\n",
               state->part_index, current_song->part_count - 1);
        return;
    }

    const song_part_t *current_part = &current_song->parts[state->part_index];
    const note_sequence_t *ms20_seq = &current_part->MS20;

    // Vérifier qu'il y a des notes dans la séquence
    if (ms20_seq->count == 0) {
        printf("[WARNING] No MS20 notes in current part\n");
        return;
    }

    // Obtenir la note courante
    uint8_t note = ms20_seq->notes[state->ms20_note_index];

    // Envoyer la note sur le canal MIDI out 4
    send_midi_note(midi, 4, note);

    // Avancer dans la séquence
    state->ms20_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->ms20_note_index >= ms20_seq->count) {
        state->ms20_note_index = 0;
        printf("[INFO] MS20 sequence looped back to start\n");
    }

    printf("[STATE] MS20 note index: %d/%d\n",
           state->ms20_note_index, ms20_seq->count);
}

/**
 * Joue la note HAPINESTRIANGLE suivante de la séquence courante
 * Canal MIDI out 5 - Déclenché par les notes sur canal in 1
 */
void play_HAPINESTRIANGLE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    if (!midi || !song_set || !state) {
        return;
    }

    // Vérifier la validité des indices
    if (state->song_index >= song_set->song_count) {
        printf("[ERROR] Invalid song index: %d (max: %d)\n",
               state->song_index, song_set->song_count - 1);
        return;
    }

    const song_t *current_song = &song_set->songs[state->song_index];

    if (state->part_index >= current_song->part_count) {
        printf("[ERROR] Invalid part index: %d (max: %d)\n",
               state->part_index, current_song->part_count - 1);
        return;
    }

    const song_part_t *current_part = &current_song->parts[state->part_index];
    const note_sequence_t *hapinestriangle_seq = &current_part->HAPINESTRIANGLE;

    // Vérifier qu'il y a des notes dans la séquence
    if (hapinestriangle_seq->count == 0) {
        printf("[WARNING] No HAPINESTRIANGLE notes in current part\n");
        return;
    }

    // Obtenir la note courante
    uint8_t note = hapinestriangle_seq->notes[state->hapinestriangle_note_index];

    // Envoyer la note sur le canal MIDI out 5
    send_midi_note(midi, 5, note);

    // Avancer dans la séquence
    state->hapinestriangle_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->hapinestriangle_note_index >= hapinestriangle_seq->count) {
        state->hapinestriangle_note_index = 0;
        printf("[INFO] HAPINESTRIANGLE sequence looped back to start\n");
    }

    printf("[STATE] HAPINESTRIANGLE note index: %d/%d\n",
           state->hapinestriangle_note_index, hapinestriangle_seq->count);
}

/**
 * Joue la note HAPINESSQUARE suivante de la séquence courante
 * Canal MIDI out 6 - Déclenché par les notes sur canal in 0
 */
void play_HAPINESSQUARE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    if (!midi || !song_set || !state) {
        return;
    }

    // Vérifier la validité des indices
    if (state->song_index >= song_set->song_count) {
        printf("[ERROR] Invalid song index: %d (max: %d)\n",
               state->song_index, song_set->song_count - 1);
        return;
    }

    const song_t *current_song = &song_set->songs[state->song_index];

    if (state->part_index >= current_song->part_count) {
        printf("[ERROR] Invalid part index: %d (max: %d)\n",
               state->part_index, current_song->part_count - 1);
        return;
    }

    const song_part_t *current_part = &current_song->parts[state->part_index];
    const note_sequence_t *hapinessquare_seq = &current_part->HAPINESSQUARE;

    // Vérifier qu'il y a des notes dans la séquence
    if (hapinessquare_seq->count == 0) {
        printf("[WARNING] No HAPINESSQUARE notes in current part\n");
        return;
    }

    // Obtenir la note courante
    uint8_t note = hapinessquare_seq->notes[state->hapinessquare_note_index];

    // Envoyer la note sur le canal MIDI out 6
    send_midi_note(midi, 6, note);

    // Avancer dans la séquence
    state->hapinessquare_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->hapinessquare_note_index >= hapinessquare_seq->count) {
        state->hapinessquare_note_index = 0;
        printf("[INFO] HAPINESSQUARE sequence looped back to start\n");
    }

    printf("[STATE] HAPINESSQUARE note index: %d/%d\n",
           state->hapinessquare_note_index, hapinessquare_seq->count);
}