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
 * Fonction générique pour jouer la note suivante d'une track
 * Remplace les 6 fonctions play_*_note identiques
 */
void play_track_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state, track_id_t track_id) {
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

    // Sélectionner la track, l'index et le canal selon le track_id
    const note_sequence_t *seq;
    int *note_index;
    uint8_t out_channel;

    switch (track_id) {
        case TRACK_BASS:
            seq = &current_part->BASS.sequence;
            note_index = &state->bass_note_index;
            out_channel = MIDI_OUT_BASS;
            break;
        case TRACK_MS20:
            seq = &current_part->MS20.sequence;
            note_index = &state->ms20_note_index;
            out_channel = MIDI_OUT_MS20;
            break;
        case TRACK_SAMPLERVOICE:
            seq = &current_part->SAMPLERVOICE.sequence;
            note_index = &state->samplervoice_note_index;
            out_channel = MIDI_OUT_SAMPLERVOICE;
            break;
        case TRACK_SAMPLERFX:
            seq = &current_part->SAMPLERFX.sequence;
            note_index = &state->samplerfx_note_index;
            out_channel = MIDI_OUT_SAMPLERFX;
            break;
        default:
            return;
    }

    // Vérifier qu'il y a des notes dans la séquence
    if (seq->count == 0) {
        return;
    }

    // Obtenir la note courante
    uint8_t note = seq->notes[*note_index];

    // Avancer dans la séquence
    (*note_index)++;

    // Revenir au début si on a atteint la fin
    if (*note_index >= seq->count) {
        *note_index = 0;
    }

    // Envoyer la note
    send_midi_note(midi, out_channel, note);
}

/**
 * Joue la note BASS suivante de la séquence courante
 * Canal MIDI out 3 - Déclenché par les notes sur canal in 0
 */
void play_BASS_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    play_track_note(midi, song_set, state, TRACK_BASS);
}

/**
 * Joue la note MS20 suivante de la séquence courante
 * Canal MIDI out 4 - Déclenché par les notes sur canal in 1
 */
void play_MS20_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    play_track_note(midi, song_set, state, TRACK_MS20);
}

/**
 * Joue la note SAMPLERVOICE suivante de la séquence courante
 * Canal MIDI out 5 - Déclenché par les notes sur canal in 2
 */
void play_SAMPLERVOICE_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    play_track_note(midi, song_set, state, TRACK_SAMPLERVOICE);
}

/**
 * Joue la note SAMPLERFX suivante de la séquence courante
 * Canal MIDI out 6 - Déclenché par les notes sur canal in 3
 */
void play_SAMPLERFX_note(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    play_track_note(midi, song_set, state, TRACK_SAMPLERFX);
}

/**
 * Table de mapping unique pour les CC Blooper
 * Élimine la duplication entre name_to_number et number_to_name
 */
typedef struct {
    const char *lowercase_name;  // Nom en minuscule pour le parsing
    int cc_number;               // Numéro du CC MIDI
} blooper_cc_mapping_t;

static const blooper_cc_mapping_t BLOOPER_CC_MAP[] = {
    {"record", BLOOPER_CC_RECORD},
    {"play", BLOOPER_CC_PLAY},
    {"overdub", BLOOPER_CC_OVERDUB},
    {"stop", BLOOPER_CC_STOP},
    {"undo", BLOOPER_CC_UNDO},
    {"redo", BLOOPER_CC_REDO},
    {"erase", BLOOPER_CC_ERASE},
    {"hold", BLOOPER_CC_HOLD},
    {"switchb", BLOOPER_CC_SWITCHB},
    {"volume", BLOOPER_CC_VOLUME},
    {"layers", BLOOPER_CC_LAYERS},
    {"repeats", BLOOPER_CC_REPEATS},
    {"moda_val", BLOOPER_CC_MODA_VAL},
    {"stability", BLOOPER_CC_STABILITY},
    {"modb_val", BLOOPER_CC_MODB_VAL},
    {"ramp", BLOOPER_CC_RAMP},
    {"moda_mode", BLOOPER_CC_MODA_MODE},
    {"loop_mode", BLOOPER_CC_LOOP_MODE},
    {"modb_mode", BLOOPER_CC_MODB_MODE},
    {"save_mode", BLOOPER_CC_SAVE_MODE},
    {"moda", BLOOPER_CC_MODA},
    {"modb", BLOOPER_CC_MODB},
    {"clock_ign", BLOOPER_CC_CLOCK_IGN},
    {"ramp_onof", BLOOPER_CC_RAMP_ONOF},
    {"note_div", BLOOPER_CC_NOTE_DIV},
    {"expr", BLOOPER_CC_EXPR},
    {NULL, -1}  // Sentinel
};

/**
 * Convertit un nom de CC Blooper en numéro de CC
 * Retourne -1 si le nom n'est pas reconnu
 */
int blooper_cc_name_to_number(const char *cc_name) {
    if (!cc_name) return -1;

    for (int i = 0; BLOOPER_CC_MAP[i].lowercase_name != NULL; i++) {
        if (strcmp(cc_name, BLOOPER_CC_MAP[i].lowercase_name) == 0) {
            return BLOOPER_CC_MAP[i].cc_number;
        }
    }

    return -1; // Not found
}

/**
 * Convertit un numéro de CC Blooper en nom d'affichage (BLOOPER + nom en majuscules)
 * Retourne "UNKNOWN" si le numéro n'est pas reconnu
 */
const char* blooper_cc_number_to_name(int cc_number) {
    static char display_names[26][32];  // Cache statique pour éviter allocations répétées
    static int initialized = 0;

    // Initialiser le cache une seule fois
    if (!initialized) {
        for (int i = 0; BLOOPER_CC_MAP[i].lowercase_name != NULL; i++) {
            const char *name = BLOOPER_CC_MAP[i].lowercase_name;
            snprintf(display_names[i], sizeof(display_names[i]), "BLOOPER%s", name);

            // Convertir en majuscules
            for (char *p = display_names[i] + 7; *p; p++) {  // +7 pour sauter "BLOOPER"
                if (*p >= 'a' && *p <= 'z') {
                    *p = *p - 'a' + 'A';
                }
            }
        }
        initialized = 1;
    }

    // Chercher le CC number dans la table
    for (int i = 0; BLOOPER_CC_MAP[i].lowercase_name != NULL; i++) {
        if (BLOOPER_CC_MAP[i].cc_number == cc_number) {
            return display_names[i];
        }
    }

    return "UNKNOWN";
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
        send_midi_cc(midi, MIDI_OUT_BLOOPER, cc_seq->cc_number, value);
    }
}