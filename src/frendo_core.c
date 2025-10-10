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
 * Remet à zéro les indices de notes (bass et melody)
 * Appelé lors du changement de chanson ou de partie
 */
void reset_note_indices(frendo_state_t *state) {
    if (!state) return;
    
    state->bass_note_index = 0;
    state->melody_note_index = 0;
    
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
    const char *song_name = song_set->songs[state->song_index].name;
    print_state_change("chanson", song_name);
    
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
    char part_info[64];
    snprintf(part_info, sizeof(part_info), "partie %d", state->part_index + 1);
    print_state_change("partie", part_info);
    
    printf("[STATE] Song: %d/%d | Part: %d/%d\n", 
           state->song_index + 1, song_set->song_count,
           state->part_index + 1, current_song->part_count);
}

/**
 * Joue la note bass suivante de la séquence courante
 * Canal MIDI 0 - Déclenché par les notes sur canal 0
 */
void play_bass_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
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
    const note_sequence_t *bass_seq = &current_part->bass;
    
    // Vérifier qu'il y a des notes dans la séquence
    if (bass_seq->count == 0) {
        printf("[WARNING] No bass notes in current part\n");
        return;
    }
    
    // Obtenir la note courante
    uint8_t note = bass_seq->notes[state->bass_note_index];
    
    // Envoyer la note sur le canal 0
    send_midi_note(midi, 4, note); // Canal 5 (0-indexé)
    printf("[MIDI OUT] Channel: 4 | Type: bass | Note: %d\n", note);
    
    // Avancer dans la séquence
    state->bass_note_index++;
    
    // Revenir au début si on a atteint la fin
    if (state->bass_note_index >= bass_seq->count) {
        state->bass_note_index = 0;
        printf("[INFO] Bass sequence looped back to start\n");
    }
    
    printf("[STATE] Bass note index: %d/%d\n", 
           state->bass_note_index, bass_seq->count);
}

/**
 * Joue la note melody suivante de la séquence courante
 * Canal MIDI 1 - Déclenché par les notes sur canal 1
 */
void play_melody_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
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
    const note_sequence_t *melody_seq = &current_part->melody;
    
    // Vérifier qu'il y a des notes dans la séquence
    if (melody_seq->count == 0) {
        printf("[WARNING] No melody notes in current part\n");
        return;
    }
    
    // Obtenir la note courante
    uint8_t note = melody_seq->notes[state->melody_note_index];
    
    // Envoyer la note sur le canal 1
    send_midi_note(midi, 5, note); // Canal 6 (0-indexé)
    printf("[MIDI OUT] Channel: 5 | Type: melody | Note: %d\n", note);
    
    // Avancer dans la séquence
    state->melody_note_index++;
    
    // Revenir au début si on a atteint la fin
    if (state->melody_note_index >= melody_seq->count) {
        state->melody_note_index = 0;
        printf("[INFO] Melody sequence looped back to start\n");
    }
    
    printf("[STATE] Melody note index: %d/%d\n", 
           state->melody_note_index, melody_seq->count);
}