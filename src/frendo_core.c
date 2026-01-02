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

    state->bass_note_index = 0;
    state->ms20_note_index = 0;
    state->hapinestriangle_note_index = 0;
    state->hapinessquare_note_index = 0;
    state->samplervoice_note_index = 0;
    state->samplerfx_note_index = 0;

    // Reset Blooper CC indices
    for (int i = 0; i < 10; i++) {
        state->blooper_cc_index[i] = 0;
    }

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
    printf("\n");
    print_state_change();
    printf("[SONG CHANGE] → %s (%d/%d)\n",
           song_set->songs[state->song_index].name,
           state->song_index + 1, song_set->song_count);
    printf("[PART]        → Part %d/%d\n",
           state->part_index + 1, song_set->songs[state->song_index].part_count);
    print_state_change();
    printf("\n");
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
    printf("\n");
    print_state_change();
    printf("[PART CHANGE] → %s - Part %d/%d\n",
           current_song->name,
           state->part_index + 1, current_song->part_count);
    print_state_change();
    printf("\n");
}

/**
 * Joue la note BASS suivante de la séquence courante
 * Canal MIDI out 3 - Déclenché par les notes sur canal in 0
 */
void play_BASS_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
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
    const note_sequence_t *bass_seq = &current_part->BASS.sequence;

    // Vérifier qu'il y a des notes dans la séquence
    if (bass_seq->count == 0) {
        return;
    }

    // Obtenir la note courante
    uint8_t note = bass_seq->notes[state->bass_note_index];

    // Avancer dans la séquence
    state->bass_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->bass_note_index >= bass_seq->count) {
        state->bass_note_index = 0;
    }

    // Envoyer la note sur le canal MIDI out 3
    send_midi_note(midi, 3, note, "BASS", state->bass_note_index, bass_seq->count);
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
    const note_sequence_t *ms20_seq = &current_part->MS20.sequence;

    // Vérifier qu'il y a des notes dans la séquence
    if (ms20_seq->count == 0) {
        return;
    }

    // Obtenir la note courante
    uint8_t note = ms20_seq->notes[state->ms20_note_index];

    // Avancer dans la séquence
    state->ms20_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->ms20_note_index >= ms20_seq->count) {
        state->ms20_note_index = 0;
    }

    // Envoyer la note sur le canal MIDI out 4
    send_midi_note(midi, 4, note, "MS20", state->ms20_note_index, ms20_seq->count);
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
    const note_sequence_t *hapinestriangle_seq = &current_part->HAPINESTRIANGLE.sequence;

    // Vérifier qu'il y a des notes dans la séquence
    if (hapinestriangle_seq->count == 0) {
        return;
    }

    // Obtenir la note courante
    uint8_t note = hapinestriangle_seq->notes[state->hapinestriangle_note_index];

    // Avancer dans la séquence
    state->hapinestriangle_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->hapinestriangle_note_index >= hapinestriangle_seq->count) {
        state->hapinestriangle_note_index = 0;
    }

    // Envoyer la note sur le canal MIDI out 5
    send_midi_note(midi, 5, note, "HAPINESTRIANGLE", state->hapinestriangle_note_index, hapinestriangle_seq->count);
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
    const note_sequence_t *hapinessquare_seq = &current_part->HAPINESSQUARE.sequence;

    // Vérifier qu'il y a des notes dans la séquence
    if (hapinessquare_seq->count == 0) {
        return;
    }

    // Obtenir la note courante
    uint8_t note = hapinessquare_seq->notes[state->hapinessquare_note_index];

    // Avancer dans la séquence
    state->hapinessquare_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->hapinessquare_note_index >= hapinessquare_seq->count) {
        state->hapinessquare_note_index = 0;
    }

    // Envoyer la note sur le canal MIDI out 6
    send_midi_note(midi, 6, note, "HAPINESSQUARE", state->hapinessquare_note_index, hapinessquare_seq->count);
}

/**
 * Joue la note SAMPLERVOICE suivante de la séquence courante
 * Canal MIDI out 7 - Déclenché par les notes sur canal in 1
 */
void play_SAMPLERVOICE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
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
    const note_sequence_t *samplervoice_seq = &current_part->SAMPLERVOICE.sequence;

    // Vérifier qu'il y a des notes dans la séquence
    if (samplervoice_seq->count == 0) {
        return;
    }

    // Obtenir la note courante
    uint8_t note = samplervoice_seq->notes[state->samplervoice_note_index];

    // Avancer dans la séquence
    state->samplervoice_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->samplervoice_note_index >= samplervoice_seq->count) {
        state->samplervoice_note_index = 0;
    }

    // Envoyer la note sur le canal MIDI out 7
    send_midi_note(midi, 7, note, "SAMPLERVOICE", state->samplervoice_note_index, samplervoice_seq->count);
}

/**
 * Joue la note SAMPLERFX suivante de la séquence courante
 * Canal MIDI out 8 - Déclenché par les notes sur canal in 0
 */
void play_SAMPLERFX_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
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
    const note_sequence_t *samplerfx_seq = &current_part->SAMPLERFX.sequence;

    // Vérifier qu'il y a des notes dans la séquence
    if (samplerfx_seq->count == 0) {
        return;
    }

    // Obtenir la note courante
    uint8_t note = samplerfx_seq->notes[state->samplerfx_note_index];

    // Avancer dans la séquence
    state->samplerfx_note_index++;

    // Revenir au début si on a atteint la fin
    if (state->samplerfx_note_index >= samplerfx_seq->count) {
        state->samplerfx_note_index = 0;
    }

    // Envoyer la note sur le canal MIDI out 8
    send_midi_note(midi, 8, note, "SAMPLERFX", state->samplerfx_note_index, samplerfx_seq->count);
}

/**
 * Convertit un nom de CC Blooper en numéro de CC
 * Retourne -1 si le nom n'est pas reconnu
 */
int blooper_cc_name_to_number(const char *cc_name) {
    if (!cc_name) return -1;

    // Map CC names to CC numbers
    if (strcmp(cc_name, "record") == 0) return BLOOPER_CC_RECORD;
    if (strcmp(cc_name, "play") == 0) return BLOOPER_CC_PLAY;
    if (strcmp(cc_name, "overdub") == 0) return BLOOPER_CC_OVERDUB;
    if (strcmp(cc_name, "stop") == 0) return BLOOPER_CC_STOP;
    if (strcmp(cc_name, "undo") == 0) return BLOOPER_CC_UNDO;
    if (strcmp(cc_name, "redo") == 0) return BLOOPER_CC_REDO;
    if (strcmp(cc_name, "erase") == 0) return BLOOPER_CC_ERASE;
    if (strcmp(cc_name, "hold") == 0) return BLOOPER_CC_HOLD;
    if (strcmp(cc_name, "switchb") == 0) return BLOOPER_CC_SWITCHB;
    if (strcmp(cc_name, "volume") == 0) return BLOOPER_CC_VOLUME;
    if (strcmp(cc_name, "layers") == 0) return BLOOPER_CC_LAYERS;
    if (strcmp(cc_name, "repeats") == 0) return BLOOPER_CC_REPEATS;
    if (strcmp(cc_name, "moda_val") == 0) return BLOOPER_CC_MODA_VAL;
    if (strcmp(cc_name, "stability") == 0) return BLOOPER_CC_STABILITY;
    if (strcmp(cc_name, "modb_val") == 0) return BLOOPER_CC_MODB_VAL;
    if (strcmp(cc_name, "ramp") == 0) return BLOOPER_CC_RAMP;
    if (strcmp(cc_name, "moda_mode") == 0) return BLOOPER_CC_MODA_MODE;
    if (strcmp(cc_name, "loop_mode") == 0) return BLOOPER_CC_LOOP_MODE;
    if (strcmp(cc_name, "modb_mode") == 0) return BLOOPER_CC_MODB_MODE;
    if (strcmp(cc_name, "save_mode") == 0) return BLOOPER_CC_SAVE_MODE;
    if (strcmp(cc_name, "moda") == 0) return BLOOPER_CC_MODA;
    if (strcmp(cc_name, "modb") == 0) return BLOOPER_CC_MODB;
    if (strcmp(cc_name, "clock_ign") == 0) return BLOOPER_CC_CLOCK_IGN;
    if (strcmp(cc_name, "ramp_onof") == 0) return BLOOPER_CC_RAMP_ONOF;
    if (strcmp(cc_name, "note_div") == 0) return BLOOPER_CC_NOTE_DIV;
    if (strcmp(cc_name, "expr") == 0) return BLOOPER_CC_EXPR;

    return -1; // Not found
}

/**
 * Joue le prochain CC Blooper pour toutes les tracks CC actives
 */
void play_BLOOPER_cc(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    if (!midi || !song_set || !state) {
        return;
    }

    // Vérifier indices valides
    if (state->song_index >= song_set->song_count) {
        return;
    }

    const song_t *current_song = &song_set->songs[state->song_index];
    if (state->part_index >= current_song->part_count) {
        return;
    }

    const song_part_t *current_part = &current_song->parts[state->part_index];

    // Parcourir toutes les tracks CC Blooper actives
    for (int i = 0; i < current_part->blooper_cc_count && i < 10; i++) {
        const cc_sequence_t *cc_seq = &current_part->BLOOPER_CC[i];

        // Vérifier qu'il y a des valeurs dans la séquence
        if (cc_seq->count == 0) {
            continue;
        }

        // Obtenir la valeur courante
        uint8_t value = cc_seq->values[state->blooper_cc_index[i]];

        // Avancer dans la séquence
        state->blooper_cc_index[i]++;

        // Revenir au début si on a atteint la fin
        if (state->blooper_cc_index[i] >= cc_seq->count) {
            state->blooper_cc_index[i] = 0;
        }

        // Envoyer le CC sur le canal MIDI out 9 (Blooper)
        char cc_name_buffer[64];
        snprintf(cc_name_buffer, sizeof(cc_name_buffer), "BLOOPER[CC#%d]", cc_seq->cc_number);
        send_midi_cc(midi, 9, cc_seq->cc_number, value, cc_name_buffer, state->blooper_cc_index[i], cc_seq->count);
    }
}