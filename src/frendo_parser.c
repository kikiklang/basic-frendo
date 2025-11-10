/*
 * frendo_parser.c - Parser custom haute performance pour format .frendo
 *
 * Parsing ligne par ligne sans AST intermédiaire
 * Écriture directe dans les structures de données
 */

#include "frendo_parser.h"
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

// Mapping des noms de tracks vers leurs indices
typedef enum {
    TRACK_CAT = 0,
    TRACK_MS20 = 1,
    TRACK_HAPINESTRIANGLE = 2,
    TRACK_HAPINESSQUARE = 3,
    TRACK_SAMPLERVOICE = 4,
    TRACK_SAMPLERFX = 5,
    TRACK_UNKNOWN = -1
} track_index_t;

// Mapping inputs nom -> channel
typedef struct {
    char name[32];
    uint8_t channel;
} input_mapping_t;

typedef struct {
    input_mapping_t inputs[16];
    int input_count;
} parser_context_t;

/**
 * Trim espaces et tabulations d'une chaîne
 */
static char* trim(char *str) {
    if (!str) return NULL;

    // Trim début
    while (isspace(*str)) str++;

    if (*str == 0) return str;

    // Trim fin
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';

    return str;
}

/**
 * Trouve le canal MIDI pour un nom d'input
 */
static int find_input_channel(parser_context_t *ctx, const char *input_name) {
    for (int i = 0; i < ctx->input_count; i++) {
        if (strcmp(ctx->inputs[i].name, input_name) == 0) {
            return ctx->inputs[i].channel;
        }
    }
    return -1;
}

/**
 * Parse le nom de track et retourne son index
 */
static track_index_t parse_track_name(const char *name) {
    if (strcmp(name, "CAT") == 0) return TRACK_CAT;
    if (strcmp(name, "MS20") == 0) return TRACK_MS20;
    if (strcmp(name, "HAPINESTRIANGLE") == 0) return TRACK_HAPINESTRIANGLE;
    if (strcmp(name, "HAPINESSQUARE") == 0) return TRACK_HAPINESSQUARE;
    if (strcmp(name, "SAMPLERVOICE") == 0) return TRACK_SAMPLERVOICE;
    if (strcmp(name, "SAMPLERFX") == 0) return TRACK_SAMPLERFX;
    return TRACK_UNKNOWN;
}

/**
 * Parse une ligne INPUTS : name = channel:N
 */
static frendo_error_t parse_input_line(const char *line, parser_context_t *ctx, int line_num) {
    char input_name[32];
    int channel;

    // Format: "name = channel:N"
    if (sscanf(line, "%31s = channel:%d", input_name, &channel) != 2) {
        printf("[ERROR] Invalid INPUT format at line %d: %s\n", line_num, line);
        printf("[HELP] Expected format: name = channel:N\n");
        return FRENDO_ERROR_JSON;
    }

    if (channel < 0 || channel > 15) {
        printf("[ERROR] Invalid MIDI channel %d at line %d (must be 0-15)\n", channel, line_num);
        return FRENDO_ERROR_JSON;
    }

    if (ctx->input_count >= 16) {
        printf("[ERROR] Too many inputs (max 16) at line %d\n", line_num);
        return FRENDO_ERROR_JSON;
    }

    strncpy(ctx->inputs[ctx->input_count].name, input_name, sizeof(ctx->inputs[0].name) - 1);
    ctx->inputs[ctx->input_count].channel = channel;
    ctx->input_count++;

    return FRENDO_OK;
}

/**
 * Fisher-Yates shuffle pour randomiser un tableau de notes
 */
static void shuffle_notes(uint8_t *notes, int count) {
    for (int i = count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        uint8_t temp = notes[i];
        notes[i] = notes[j];
        notes[j] = temp;
    }
}

/**
 * Parse une séquence de notes MIDI depuis une chaîne
 * Format: "48 50 52 | 55 60 62"
 * Supporte aussi les ranges: "[1..16]" génère 1 2 3 4 5 ... 16
 * Supporte aussi les ranges randomisés: "R[1..16]" génère 1..16 dans un ordre aléatoire
 * Les | sont ignorés
 */
static frendo_error_t parse_note_sequence(const char *str, note_sequence_t *seq, int line_num) {
    memset(seq, 0, sizeof(note_sequence_t));

    const char *p = str;
    int note_count = 0;

    while (*p && note_count < MAX_NOTES_PER_SEQ) {
        // Ignorer espaces et |
        while (*p && (isspace(*p) || *p == '|')) p++;

        if (!*p) break;

        // Détecter range randomisé R[start..end] ou range normal [start..end]
        if (*p == 'R' && *(p + 1) == '[') {
            // Range randomisé
            int start, end;
            const char *range_end = strchr(p, ']');

            if (!range_end) {
                printf("[ERROR] Missing closing ']' in random range at line %d\n", line_num);
                return FRENDO_ERROR_JSON;
            }

            if (sscanf(p, "R[%d..%d]", &start, &end) == 2) {
                // Valider les valeurs MIDI
                if (start < 0 || start > 127 || end < 0 || end > 127) {
                    printf("[ERROR] Invalid MIDI range R[%d..%d] at line %d (must be 0-127)\n",
                           start, end, line_num);
                    return FRENDO_ERROR_JSON;
                }

                // Générer la séquence (toujours ascendante pour le random)
                int range_start = (start <= end) ? start : end;
                int range_end_val = (start <= end) ? end : start;
                int temp_count = 0;

                for (int note = range_start; note <= range_end_val; note++) {
                    if (note_count + temp_count >= MAX_NOTES_PER_SEQ) {
                        printf("[ERROR] Too many notes (max %d) at line %d\n",
                               MAX_NOTES_PER_SEQ, line_num);
                        return FRENDO_ERROR_JSON;
                    }
                    seq->notes[note_count + temp_count] = (uint8_t)note;
                    temp_count++;
                }

                // Randomiser uniquement les notes générées
                shuffle_notes(&seq->notes[note_count], temp_count);
                note_count += temp_count;

                // Avancer le pointeur après le ']'
                p = range_end + 1;
            } else {
                printf("[ERROR] Invalid random range syntax at line %d (expected R[N..M])\n", line_num);
                return FRENDO_ERROR_JSON;
            }
        }
        // Détecter range normal [start..end]
        else if (*p == '[') {
            int start, end;
            const char *range_end = strchr(p, ']');

            if (!range_end) {
                printf("[ERROR] Missing closing ']' in range at line %d\n", line_num);
                return FRENDO_ERROR_JSON;
            }

            if (sscanf(p, "[%d..%d]", &start, &end) == 2) {
                // Valider les valeurs MIDI
                if (start < 0 || start > 127 || end < 0 || end > 127) {
                    printf("[ERROR] Invalid MIDI range [%d..%d] at line %d (must be 0-127)\n",
                           start, end, line_num);
                    return FRENDO_ERROR_JSON;
                }

                // Générer la séquence (ascendant ou descendant)
                int step = (start <= end) ? 1 : -1;
                for (int note = start;
                     (step > 0 && note <= end) || (step < 0 && note >= end);
                     note += step) {

                    if (note_count >= MAX_NOTES_PER_SEQ) {
                        printf("[ERROR] Too many notes (max %d) at line %d\n",
                               MAX_NOTES_PER_SEQ, line_num);
                        return FRENDO_ERROR_JSON;
                    }

                    seq->notes[note_count++] = (uint8_t)note;
                }

                // Avancer le pointeur après le ']'
                p = range_end + 1;
            } else {
                printf("[ERROR] Invalid range syntax at line %d (expected [N..M])\n", line_num);
                return FRENDO_ERROR_JSON;
            }
        }
        // Parser un nombre
        else if (isdigit(*p)) {
            int value = 0;
            while (isdigit(*p)) {
                value = value * 10 + (*p - '0');
                p++;
            }

            if (value > 127) {
                printf("[ERROR] Invalid MIDI note %d at line %d (must be 0-127)\n", value, line_num);
                return FRENDO_ERROR_JSON;
            }

            seq->notes[note_count++] = (uint8_t)value;
        } else {
            printf("[ERROR] Unexpected character '%c' in note sequence at line %d\n", *p, line_num);
            return FRENDO_ERROR_JSON;
        }
    }

    seq->count = note_count;
    return FRENDO_OK;
}

/**
 * Parse une ligne de track : TRACKNAME[input]: notes
 */
static frendo_error_t parse_track_line(const char *line, song_part_t *part, parser_context_t *ctx, int line_num) {
    char track_name[32];
    char input_name[32];
    const char *notes_start;

    // Trouver le [
    const char *bracket_open = strchr(line, '[');
    if (!bracket_open) {
        printf("[ERROR] Missing [input] in track declaration at line %d\n", line_num);
        printf("[HELP] Expected format: TRACKNAME[input]: notes\n");
        return FRENDO_ERROR_JSON;
    }

    // Extraire le nom de track
    size_t track_name_len = bracket_open - line;
    if (track_name_len >= sizeof(track_name)) {
        printf("[ERROR] Track name too long at line %d\n", line_num);
        return FRENDO_ERROR_JSON;
    }
    strncpy(track_name, line, track_name_len);
    track_name[track_name_len] = '\0';
    char *trimmed_track = trim(track_name);

    // Trouver le ]
    const char *bracket_close = strchr(bracket_open, ']');
    if (!bracket_close) {
        printf("[ERROR] Missing ] in track declaration at line %d\n", line_num);
        return FRENDO_ERROR_JSON;
    }

    // Extraire le nom d'input
    size_t input_name_len = bracket_close - bracket_open - 1;
    if (input_name_len >= sizeof(input_name)) {
        printf("[ERROR] Input name too long at line %d\n", line_num);
        return FRENDO_ERROR_JSON;
    }
    strncpy(input_name, bracket_open + 1, input_name_len);
    input_name[input_name_len] = '\0';
    char *trimmed_input = trim(input_name);

    // Trouver le :
    const char *colon = strchr(bracket_close, ':');
    if (!colon) {
        printf("[ERROR] Missing : after track declaration at line %d\n", line_num);
        return FRENDO_ERROR_JSON;
    }

    notes_start = colon + 1;

    // Identifier la track
    track_index_t track_idx = parse_track_name(trimmed_track);
    if (track_idx == TRACK_UNKNOWN) {
        printf("[ERROR] Unknown track name '%s' at line %d\n", trimmed_track, line_num);
        printf("[HELP] Valid tracks: CAT, MS20, HAPINESTRIANGLE, HAPINESSQUARE, SAMPLERVOICE, SAMPLERFX\n");
        return FRENDO_ERROR_JSON;
    }

    // Trouver le canal de l'input
    int channel = find_input_channel(ctx, trimmed_input);
    if (channel < 0) {
        printf("[ERROR] Unknown input '%s' at line %d\n", trimmed_input, line_num);
        printf("[HELP] Input must be declared in INPUTS section first\n");
        return FRENDO_ERROR_JSON;
    }

    // Parser la séquence de notes
    note_sequence_t *seq = NULL;
    switch (track_idx) {
        case TRACK_CAT: seq = &part->CAT.sequence; part->CAT.listen_channel = channel; break;
        case TRACK_MS20: seq = &part->MS20.sequence; part->MS20.listen_channel = channel; break;
        case TRACK_HAPINESTRIANGLE: seq = &part->HAPINESTRIANGLE.sequence; part->HAPINESTRIANGLE.listen_channel = channel; break;
        case TRACK_HAPINESSQUARE: seq = &part->HAPINESSQUARE.sequence; part->HAPINESSQUARE.listen_channel = channel; break;
        case TRACK_SAMPLERVOICE: seq = &part->SAMPLERVOICE.sequence; part->SAMPLERVOICE.listen_channel = channel; break;
        case TRACK_SAMPLERFX: seq = &part->SAMPLERFX.sequence; part->SAMPLERFX.listen_channel = channel; break;
        default: return FRENDO_ERROR_JSON;
    }

    return parse_note_sequence(notes_start, seq, line_num);
}

/**
 * Charge un fichier .frendo
 */
frendo_error_t load_frendo_file(const char *filename, song_t *song, const char *song_name) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("[ERROR] Cannot open file: %s\n", filename);
        return FRENDO_ERROR_FILE;
    }

    // Initialiser la chanson
    memset(song, 0, sizeof(song_t));
    strncpy(song->name, song_name, MAX_NAME_LENGTH - 1);
    song->name[MAX_NAME_LENGTH - 1] = '\0';

    parser_context_t ctx = {0};

    char line[4096];
    int line_num = 0;
    int in_inputs = 0;
    int in_part = 0;
    song_part_t *current_part = NULL;

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        char *trimmed = trim(line);

        // Ignorer lignes vides
        if (strlen(trimmed) == 0) continue;

        // Détecter section INPUTS
        if (strcmp(trimmed, "INPUTS") == 0) {
            in_inputs = 1;
            in_part = 0;
            continue;
        }

        // Détecter section PART
        if (strcmp(trimmed, "PART") == 0) {
            in_inputs = 0;
            in_part = 1;

            if (song->part_count >= MAX_PARTS_PER_SONG) {
                printf("[ERROR] Too many PART sections at line %d (max %d)\n", line_num, MAX_PARTS_PER_SONG);
                fclose(file);
                return FRENDO_ERROR_JSON;
            }

            current_part = &song->parts[song->part_count];
            memset(current_part, 0, sizeof(song_part_t));
            song->part_count++;
            continue;
        }

        // Parser contenu selon section
        if (in_inputs) {
            frendo_error_t result = parse_input_line(trimmed, &ctx, line_num);
            if (result != FRENDO_OK) {
                fclose(file);
                return result;
            }
        } else if (in_part && current_part) {
            frendo_error_t result = parse_track_line(trimmed, current_part, &ctx, line_num);
            if (result != FRENDO_OK) {
                fclose(file);
                return result;
            }
        }
    }

    fclose(file);
    return FRENDO_OK;
}

/**
 * Charge tous les fichiers .frendo d'un répertoire
 */
frendo_error_t load_frendo_set(const char *directory, song_set_t *song_set) {
    memset(song_set, 0, sizeof(song_set_t));

    DIR *dir = opendir(directory);
    if (!dir) {
        printf("[ERROR] Cannot open directory: %s\n", directory);
        return FRENDO_ERROR_FILE;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && song_set->song_count < MAX_SONGS) {
        // Vérifier extension .frendo
        size_t name_len = strlen(entry->d_name);
        if (name_len < 8 || strcmp(entry->d_name + name_len - 7, ".frendo") != 0) {
            continue;
        }

        // Construire le chemin complet
        char filepath[MAX_FILENAME_LENGTH];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);

        // Extraire le nom de la chanson (sans extension)
        char song_name[MAX_NAME_LENGTH];
        strncpy(song_name, entry->d_name, name_len - 7);
        song_name[name_len - 7] = '\0';

        // Charger le fichier
        frendo_error_t result = load_frendo_file(filepath, &song_set->songs[song_set->song_count], song_name);
        if (result != FRENDO_OK) {
            closedir(dir);
            return result;
        }

        song_set->song_count++;
    }

    closedir(dir);

    if (song_set->song_count == 0) {
        printf("[ERROR] No .frendo files found in %s\n", directory);
        return FRENDO_ERROR_FILE;
    }

    return FRENDO_OK;
}

/**
 * Liste les sets disponibles (sous-répertoires de sets/)
 */
int list_available_sets(const char *sets_dir, char set_names[][MAX_NAME_LENGTH], int max_sets) {
    DIR *dir = opendir(sets_dir);
    if (!dir) {
        printf("[ERROR] Cannot open sets directory: %s\n", sets_dir);
        return 0;
    }

    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < max_sets) {
        // Ignorer . et ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Vérifier que c'est un répertoire
        char fullpath[MAX_FILENAME_LENGTH];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", sets_dir, entry->d_name);

        struct stat statbuf;
        if (stat(fullpath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            strncpy(set_names[count], entry->d_name, MAX_NAME_LENGTH - 1);
            set_names[count][MAX_NAME_LENGTH - 1] = '\0';
            count++;
        }
    }

    closedir(dir);
    return count;
}

/**
 * Affiche les informations d'un set de chansons (pour debug)
 */
void print_song_set_info(const song_set_t *song_set) {
    if (!song_set) return;

    printf("\n═══════════════════════════════════════\n");
    printf("  SONG SET INFORMATION\n");
    printf("═══════════════════════════════════════\n");
    printf("Total songs: %d\n\n", song_set->song_count);

    for (int i = 0; i < song_set->song_count; i++) {
        const song_t *song = &song_set->songs[i];
        printf("Song %d: %s (%d parts)\n", i + 1, song->name, song->part_count);

        for (int j = 0; j < song->part_count; j++) {
            const song_part_t *part = &song->parts[j];
            printf("  Part %d: CAT[%d] MS20[%d] HAPINESTRIANGLE[%d] HAPINESSQUARE[%d] SAMPLERVOICE[%d] SAMPLERFX[%d]\n",
                   j + 1,
                   part->CAT.sequence.count, part->MS20.sequence.count,
                   part->HAPINESTRIANGLE.sequence.count, part->HAPINESSQUARE.sequence.count,
                   part->SAMPLERVOICE.sequence.count, part->SAMPLERFX.sequence.count);
        }
        printf("\n");
    }

    printf("─────────────────────────────────────────\n");
}
