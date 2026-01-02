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
    TRACK_BASS = 0,
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

// ============================================================
// Structures pour le format TABLE
// ============================================================

// Types de cellule dans une table
typedef enum {
    CELL_EMPTY,           // Cellule vide
    CELL_VALUE,           // Valeur directe (ex: 34, 127)
    CELL_SKIP,            // Symbole - (skip = 255)
    CELL_CARET,           // Symbole ^ (repeat/continue)
    CELL_RANGE,           // Range [start..end]
    CELL_RANDOM           // Random R[start..end]
} cell_type_t;

// Cellule individuelle
typedef struct {
    cell_type_t type;
    union {
        uint8_t value;        // Pour CELL_VALUE et CELL_SKIP
        struct {
            int start;
            int end;
        } range;              // Pour CELL_RANGE et CELL_RANDOM
    } data;
} table_cell_t;

// Colonne complète (une track)
typedef struct {
    char track_name[32];      // "BASS", "BLOOPERSTOP", etc.
    char input_name[32];      // "kick", "snare", etc.
    bool is_blooper;          // true si BLOOPER*
    int cc_number;            // Si blooper, le CC number
    track_index_t track_idx;  // Index de la track

    table_cell_t cells[100];  // Cellules (rows 00-99)
    int cell_count;           // Nombre de rows
} table_column_t;

// Table complète (toutes les colonnes d'un PART)
typedef struct {
    table_column_t columns[20];  // Max 20 colonnes
    int column_count;
} table_data_t;

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
    if (strcmp(name, "BASS") == 0) return TRACK_BASS;
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

// ============================================================
// Fonctions pour le parsing TABLE
// ============================================================

/**
 * Parse une cellule individuelle du format TABLE
 *
 * @param p Pointeur sur le pointeur de la chaîne (modifié pour avancer)
 * @param cell Cellule à remplir
 * @param line_num Numéro de ligne (pour erreurs)
 * @return FRENDO_OK ou code d'erreur
 */
static frendo_error_t parse_table_cell(const char **p, table_cell_t *cell, int line_num) {
    // Initialiser cellule
    memset(cell, 0, sizeof(table_cell_t));

    // Skip espaces initiaux
    while (**p && isspace(**p)) (*p)++;

    if (!**p) {
        cell->type = CELL_EMPTY;
        return FRENDO_OK;
    }

    // 1. Symbole ^ (caret)
    if (**p == '^') {
        cell->type = CELL_CARET;
        (*p)++;
        return FRENDO_OK;
    }

    // 2. Symbole - (skip)
    if (**p == '-') {
        cell->type = CELL_SKIP;
        cell->data.value = 255;
        (*p)++;
        return FRENDO_OK;
    }

    // 3. Random R[start..end]
    if (**p == 'R' && *(*p + 1) == '[') {
        const char *bracket_close = strchr(*p, ']');
        if (!bracket_close) {
            printf("[ERROR] Missing ']' in random range at line %d\n", line_num);
            return FRENDO_ERROR_JSON;
        }

        int start, end;
        if (sscanf(*p, "R[%d..%d]", &start, &end) != 2) {
            printf("[ERROR] Invalid random range syntax at line %d\n", line_num);
            return FRENDO_ERROR_JSON;
        }

        if (start < 0 || start > 127 || end < 0 || end > 127) {
            printf("[ERROR] Invalid MIDI range R[%d..%d] at line %d (must be 0-127)\n",
                   start, end, line_num);
            return FRENDO_ERROR_JSON;
        }

        cell->type = CELL_RANDOM;
        cell->data.range.start = start;
        cell->data.range.end = end;

        *p = bracket_close + 1;
        return FRENDO_OK;
    }

    // 4. Range [start..end]
    if (**p == '[') {
        const char *bracket_close = strchr(*p, ']');
        if (!bracket_close) {
            printf("[ERROR] Missing ']' in range at line %d\n", line_num);
            return FRENDO_ERROR_JSON;
        }

        int start, end;
        if (sscanf(*p, "[%d..%d]", &start, &end) != 2) {
            printf("[ERROR] Invalid range syntax at line %d\n", line_num);
            return FRENDO_ERROR_JSON;
        }

        if (start < 0 || start > 127 || end < 0 || end > 127) {
            printf("[ERROR] Invalid MIDI range [%d..%d] at line %d (must be 0-127)\n",
                   start, end, line_num);
            return FRENDO_ERROR_JSON;
        }

        cell->type = CELL_RANGE;
        cell->data.range.start = start;
        cell->data.range.end = end;

        *p = bracket_close + 1;
        return FRENDO_OK;
    }

    // 5. Valeur numérique directe
    if (isdigit(**p)) {
        int value = 0;
        while (isdigit(**p)) {
            value = value * 10 + (**p - '0');
            (*p)++;
        }

        if (value > 127) {
            printf("[ERROR] Invalid MIDI value %d at line %d (must be 0-127)\n",
                   value, line_num);
            return FRENDO_ERROR_JSON;
        }

        cell->type = CELL_VALUE;
        cell->data.value = (uint8_t)value;
        return FRENDO_OK;
    }

    // Cellule invalide ou vide
    cell->type = CELL_EMPTY;
    return FRENDO_OK;
}

/**
 * Stocke une séquence expandée dans song_part_t
 *
 * @param col La colonne contenant les métadonnées de la track
 * @param values Les valeurs expandées
 * @param count Nombre de valeurs
 * @param part Le part à remplir
 * @param ctx Le contexte du parser
 * @param line_num Numéro de ligne (pour erreurs)
 * @return FRENDO_OK ou code d'erreur
 */
static frendo_error_t store_column_sequence(table_column_t *col, uint8_t *values, int count,
                                            song_part_t *part, parser_context_t *ctx, int line_num) {
    if (col->is_blooper) {
        // Stocker dans Blooper CC
        if (part->blooper_cc_count >= 10) {
            printf("[ERROR] Too many Blooper CC tracks (max 10) at line %d\n", line_num);
            return FRENDO_ERROR_JSON;
        }

        cc_sequence_t *cc_seq = &part->BLOOPER_CC[part->blooper_cc_count];
        cc_seq->cc_number = (uint8_t)col->cc_number;
        memcpy(cc_seq->values, values, count);
        cc_seq->count = count;

        // Set listen channel
        int channel = find_input_channel(ctx, col->input_name);
        part->blooper_listen_channel = channel;
        part->blooper_cc_count++;

    } else {
        // Stocker dans track normale
        note_sequence_t *seq = NULL;
        int channel = find_input_channel(ctx, col->input_name);

        switch (col->track_idx) {
            case TRACK_BASS:
                seq = &part->BASS.sequence;
                part->BASS.listen_channel = channel;
                break;
            case TRACK_MS20:
                seq = &part->MS20.sequence;
                part->MS20.listen_channel = channel;
                break;
            case TRACK_HAPINESTRIANGLE:
                seq = &part->HAPINESTRIANGLE.sequence;
                part->HAPINESTRIANGLE.listen_channel = channel;
                break;
            case TRACK_HAPINESSQUARE:
                seq = &part->HAPINESSQUARE.sequence;
                part->HAPINESSQUARE.listen_channel = channel;
                break;
            case TRACK_SAMPLERVOICE:
                seq = &part->SAMPLERVOICE.sequence;
                part->SAMPLERVOICE.listen_channel = channel;
                break;
            case TRACK_SAMPLERFX:
                seq = &part->SAMPLERFX.sequence;
                part->SAMPLERFX.listen_channel = channel;
                break;
            default:
                printf("[ERROR] Unknown track index at line %d\n", line_num);
                return FRENDO_ERROR_JSON;
        }

        memcpy(seq->notes, values, count);
        seq->count = count;
    }

    return FRENDO_OK;
}

/**
 * Résout le symbole ^ (caret) en trouvant la cellule source
 *
 * @param col La colonne
 * @param current_row La row actuelle (avec ^)
 * @param expanded Buffer de séquence expandée
 * @param expanded_count Compteur de la séquence
 * @param range_buffer Buffer pour le range/random actif
 * @param range_buffer_count Taille du buffer
 * @param range_idx Index dans le buffer (modifié)
 * @param source_cell Pointeur vers la cellule source (output)
 * @return FRENDO_OK ou code d'erreur
 */
static frendo_error_t resolve_caret(table_column_t *col, int current_row,
                                    uint8_t *expanded, int *expanded_count,
                                    uint8_t *range_buffer, int *range_buffer_count, int *range_idx,
                                    table_cell_t **source_cell) {
    // Remonter pour trouver la cellule source (non-^)
    int source_row = current_row - 1;
    while (source_row >= 0 && col->cells[source_row].type == CELL_CARET) {
        source_row--;
    }

    if (source_row < 0) {
        printf("[ERROR] Cannot resolve ^ at row %d: no source value above\n", current_row);
        return FRENDO_ERROR_JSON;
    }

    *source_cell = &col->cells[source_row];

    switch ((*source_cell)->type) {
        case CELL_VALUE:
            // ^ sur valeur directe = répéter la valeur
            if (*expanded_count >= MAX_NOTES_PER_SEQ) {
                printf("[ERROR] Too many values in TABLE column '%s'\n", col->track_name);
                return FRENDO_ERROR_JSON;
            }
            expanded[(*expanded_count)++] = (*source_cell)->data.value;
            break;

        case CELL_SKIP:
            // ^ sur skip = répéter skip
            if (*expanded_count >= MAX_NOTES_PER_SEQ) {
                printf("[ERROR] Too many values in TABLE column '%s'\n", col->track_name);
                return FRENDO_ERROR_JSON;
            }
            expanded[(*expanded_count)++] = 255;
            break;

        case CELL_RANGE:
        case CELL_RANDOM:
            // ^ sur range/random = prendre la prochaine valeur du buffer
            if (*range_buffer_count == 0) {
                printf("[ERROR] Range/Random not initialized for ^ at row %d\n", current_row);
                return FRENDO_ERROR_JSON;
            }

            // Si range_idx >= range_buffer_count, boucler
            if (*range_idx >= *range_buffer_count) {
                *range_idx = 0;  // Recommencer au début (comportement cyclique)
            }

            if (*expanded_count >= MAX_NOTES_PER_SEQ) {
                printf("[ERROR] Too many values in TABLE column '%s'\n", col->track_name);
                return FRENDO_ERROR_JSON;
            }
            expanded[(*expanded_count)++] = range_buffer[*range_idx];
            (*range_idx)++;
            break;

        default:
            printf("[ERROR] Cannot resolve ^ at row %d: invalid source type\n", current_row);
            return FRENDO_ERROR_JSON;
    }

    return FRENDO_OK;
}

/**
 * Expande une table complète en séquences et stocke dans song_part_t
 *
 * @param table La table à expander
 * @param part Le part à remplir
 * @param ctx Le contexte du parser
 * @param line_num Numéro de ligne (pour erreurs)
 * @return FRENDO_OK ou code d'erreur
 */
static frendo_error_t expand_table_to_sequences(table_data_t *table, song_part_t *part,
                                                parser_context_t *ctx, int line_num) {
    // Pour chaque colonne, générer la séquence finale
    for (int col_idx = 0; col_idx < table->column_count; col_idx++) {
        table_column_t *col = &table->columns[col_idx];

        // Buffer pour la séquence expandée
        uint8_t expanded[MAX_NOTES_PER_SEQ];
        int expanded_count = 0;

        // Buffer pour range/random actif
        uint8_t range_buffer[128];
        int range_buffer_count = 0;
        int range_idx = 0;

        // Parcourir chaque cellule et résoudre
        for (int row = 0; row < col->cell_count; row++) {
            table_cell_t *cell = &col->cells[row];

            switch (cell->type) {
                case CELL_VALUE:
                    // Valeur directe
                    if (expanded_count >= MAX_NOTES_PER_SEQ) {
                        printf("[ERROR] Too many values in TABLE column '%s'\n", col->track_name);
                        return FRENDO_ERROR_JSON;
                    }
                    expanded[expanded_count++] = cell->data.value;
                    break;

                case CELL_SKIP:
                    // Skip (255)
                    if (expanded_count >= MAX_NOTES_PER_SEQ) {
                        printf("[ERROR] Too many values in TABLE column '%s'\n", col->track_name);
                        return FRENDO_ERROR_JSON;
                    }
                    expanded[expanded_count++] = 255;
                    break;

                case CELL_RANGE:
                    // Expander le range et prendre la première valeur
                    range_buffer_count = 0;
                    range_idx = 0;

                    {
                        int start = cell->data.range.start;
                        int end = cell->data.range.end;
                        int step = (start <= end) ? 1 : -1;

                        for (int note = start;
                             (step > 0 && note <= end) || (step < 0 && note >= end);
                             note += step) {
                            if (range_buffer_count >= 128) break;
                            range_buffer[range_buffer_count++] = (uint8_t)note;
                        }
                    }

                    // Prendre la première valeur
                    if (expanded_count >= MAX_NOTES_PER_SEQ) {
                        printf("[ERROR] Too many values in TABLE column '%s'\n", col->track_name);
                        return FRENDO_ERROR_JSON;
                    }
                    if (range_buffer_count > 0) {
                        expanded[expanded_count++] = range_buffer[range_idx++];
                    }
                    break;

                case CELL_RANDOM:
                    // Expander le random et prendre la première valeur
                    range_buffer_count = 0;
                    range_idx = 0;

                    {
                        int start = cell->data.range.start;
                        int end = cell->data.range.end;
                        int range_start = (start <= end) ? start : end;
                        int range_end = (start <= end) ? end : start;

                        // Générer la séquence
                        for (int note = range_start; note <= range_end; note++) {
                            if (range_buffer_count >= 128) break;
                            range_buffer[range_buffer_count++] = (uint8_t)note;
                        }

                        // Randomiser
                        shuffle_notes(range_buffer, range_buffer_count);
                    }

                    // Prendre la première valeur
                    if (expanded_count >= MAX_NOTES_PER_SEQ) {
                        printf("[ERROR] Too many values in TABLE column '%s'\n", col->track_name);
                        return FRENDO_ERROR_JSON;
                    }
                    if (range_buffer_count > 0) {
                        expanded[expanded_count++] = range_buffer[range_idx++];
                    }
                    break;

                case CELL_CARET:
                    // Résoudre le ^
                    {
                        table_cell_t *source_cell = NULL;
                        frendo_error_t err = resolve_caret(
                            col, row, expanded, &expanded_count,
                            range_buffer, &range_buffer_count, &range_idx,
                            &source_cell
                        );
                        if (err != FRENDO_OK) return err;
                    }
                    break;

                case CELL_EMPTY:
                default:
                    // Ignorer
                    break;
            }
        }

        // Stocker la séquence dans song_part_t
        if (expanded_count > 0) {
            frendo_error_t err = store_column_sequence(
                col, expanded, expanded_count, part, ctx, line_num
            );
            if (err != FRENDO_OK) return err;
        }
    }

    return FRENDO_OK;
}

/**
 * Parse la ligne de header d'une TABLE
 * Format: #   BASS[kick]  MS20[snare]  BLOOPERSTOP[kick]
 *
 * @param line La ligne à parser
 * @param table La table à remplir
 * @param ctx Le contexte du parser (pour valider inputs)
 * @param line_num Numéro de ligne (pour erreurs)
 * @return FRENDO_OK ou code d'erreur
 */
static frendo_error_t parse_table_header_line(const char *line, table_data_t *table,
                                               parser_context_t *ctx, int line_num) {
    const char *p = line;

    // Skip espaces
    while (isspace(*p)) p++;

    // Vérifier que la ligne commence par '#'
    if (*p != '#') {
        printf("[ERROR] TABLE header must start with '#' at line %d\n", line_num);
        return FRENDO_ERROR_JSON;
    }
    p++;  // Skip '#'

    table->column_count = 0;

    // Parser chaque colonne
    while (*p && table->column_count < 20) {
        // Skip espaces et séparateurs
        while (*p && (isspace(*p) || *p == '|')) p++;
        if (!*p) break;

        // Chercher '[' pour détecter "TRACKNAME[input]"
        const char *bracket_open = strchr(p, '[');
        if (!bracket_open || (bracket_open - p) > 50) {
            // Pas de bracket dans les 50 prochains chars = probablement pas une colonne
            // Avancer jusqu'au prochain espace
            while (*p && !isspace(*p)) p++;
            continue;
        }

        table_column_t *col = &table->columns[table->column_count];
        memset(col, 0, sizeof(table_column_t));

        // Extraire nom de track
        size_t track_len = bracket_open - p;
        if (track_len >= sizeof(col->track_name)) {
            printf("[ERROR] Track name too long in TABLE header at line %d\n", line_num);
            return FRENDO_ERROR_JSON;
        }
        strncpy(col->track_name, p, track_len);
        col->track_name[track_len] = '\0';

        // Trim le nom de track
        char *trimmed_track = trim(col->track_name);
        if (trimmed_track != col->track_name) {
            memmove(col->track_name, trimmed_track, strlen(trimmed_track) + 1);
        }

        // Extraire input name
        const char *bracket_close = strchr(bracket_open, ']');
        if (!bracket_close) {
            printf("[ERROR] Missing ']' in TABLE header at line %d\n", line_num);
            return FRENDO_ERROR_JSON;
        }

        size_t input_len = bracket_close - bracket_open - 1;
        if (input_len >= sizeof(col->input_name)) {
            printf("[ERROR] Input name too long in TABLE header at line %d\n", line_num);
            return FRENDO_ERROR_JSON;
        }
        strncpy(col->input_name, bracket_open + 1, input_len);
        col->input_name[input_len] = '\0';

        // Trim le nom d'input
        char *trimmed_input = trim(col->input_name);
        if (trimmed_input != col->input_name) {
            memmove(col->input_name, trimmed_input, strlen(trimmed_input) + 1);
        }

        // Déterminer si c'est BLOOPER ou track normale
        if (strncmp(col->track_name, "BLOOPER", 7) == 0) {
            col->is_blooper = true;

            // Extraire CC name et convertir en numéro
            const char *cc_name_raw = col->track_name + 7;
            char cc_name_lower[64];
            int i = 0;
            while (cc_name_raw[i] && i < 63) {
                cc_name_lower[i] = tolower(cc_name_raw[i]);
                i++;
            }
            cc_name_lower[i] = '\0';

            col->cc_number = blooper_cc_name_to_number(cc_name_lower);
            if (col->cc_number < 0) {
                printf("[ERROR] Unknown Blooper CC '%s' in TABLE header at line %d\n",
                       cc_name_lower, line_num);
                return FRENDO_ERROR_JSON;
            }
        } else {
            col->is_blooper = false;
            col->track_idx = parse_track_name(col->track_name);
            if (col->track_idx == TRACK_UNKNOWN) {
                printf("[ERROR] Unknown track '%s' in TABLE header at line %d\n",
                       col->track_name, line_num);
                return FRENDO_ERROR_JSON;
            }
        }

        // Vérifier que l'input existe
        int channel = find_input_channel(ctx, col->input_name);
        if (channel < 0) {
            printf("[ERROR] Unknown input '%s' in TABLE header at line %d\n",
                   col->input_name, line_num);
            printf("[HELP] Make sure '%s' is declared in INPUTS section\n",
                   col->input_name);
            return FRENDO_ERROR_JSON;
        }

        table->column_count++;

        // Avancer au prochain champ
        p = bracket_close + 1;
    }

    if (table->column_count == 0) {
        printf("[ERROR] No columns found in TABLE header at line %d\n", line_num);
        return FRENDO_ERROR_JSON;
    }

    return FRENDO_OK;
}

/**
 * Parse une ligne de données TABLE
 * Format: 00  34  R[60..72]  127  -
 *
 * @param line La ligne à parser
 * @param table La table à remplir
 * @param line_num Numéro de ligne (pour erreurs)
 * @return FRENDO_OK ou code d'erreur
 */
static frendo_error_t parse_table_data_line(const char *line, table_data_t *table, int line_num) {
    const char *p = line;

    // Skip espaces
    while (isspace(*p)) p++;

    // Extraire le numéro de ligne (ex: "00", "01", etc.)
    int row_num = -1;
    if (isdigit(*p)) {
        row_num = 0;
        while (isdigit(*p)) {
            row_num = row_num * 10 + (*p - '0');
            p++;
        }
    }

    if (row_num < 0 || row_num >= 100) {
        printf("[ERROR] Invalid row number in TABLE at line %d\n", line_num);
        return FRENDO_ERROR_JSON;
    }

    // Parser chaque colonne
    for (int col_idx = 0; col_idx < table->column_count; col_idx++) {
        table_column_t *col = &table->columns[col_idx];

        // Skip espaces et séparateurs
        while (*p && (isspace(*p) || *p == '|')) p++;

        if (!*p) {
            // Fin de ligne = cellule vide pour les colonnes restantes
            col->cells[row_num].type = CELL_EMPTY;
            continue;
        }

        table_cell_t *cell = &col->cells[row_num];

        // Parser la cellule
        frendo_error_t err = parse_table_cell(&p, cell, line_num);
        if (err != FRENDO_OK) return err;
    }

    // Mettre à jour cell_count si nécessaire
    for (int col_idx = 0; col_idx < table->column_count; col_idx++) {
        if (row_num + 1 > table->columns[col_idx].cell_count) {
            table->columns[col_idx].cell_count = row_num + 1;
        }
    }

    return FRENDO_OK;
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
    int in_table = 0;
    bool table_header_parsed = false;
    song_part_t *current_part = NULL;
    table_data_t current_table = {0};

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

        // Détecter section PART (détection automatique du format)
        if (strcmp(trimmed, "PART") == 0) {
            // Si on était en mode TABLE, finaliser la table précédente
            if (in_table && table_header_parsed && current_part) {
                frendo_error_t result = expand_table_to_sequences(&current_table, current_part, &ctx, line_num);
                if (result != FRENDO_OK) {
                    fclose(file);
                    return result;
                }
            }

            in_inputs = 0;
            in_part = 1;
            in_table = 0;  // Sera activé au premier header

            if (song->part_count >= MAX_PARTS_PER_SONG) {
                printf("[ERROR] Too many PART sections at line %d (max %d)\n", line_num, MAX_PARTS_PER_SONG);
                fclose(file);
                return FRENDO_ERROR_JSON;
            }

            current_part = &song->parts[song->part_count];
            memset(current_part, 0, sizeof(song_part_t));
            song->part_count++;

            // Initialiser table
            memset(&current_table, 0, sizeof(table_data_t));
            table_header_parsed = false;

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
            // Mode PART - format table uniquement

            // Si on voit un '#' avec '[', c'est un header de table
            if (trimmed[0] == '#' && strchr(trimmed, '[') != NULL) {
                // Activer mode table et parser le header
                in_table = 1;

                if (table_header_parsed) {
                    printf("[ERROR] Multiple headers in TABLE at line %d\n", line_num);
                    fclose(file);
                    return FRENDO_ERROR_JSON;
                }

                frendo_error_t result = parse_table_header_line(trimmed, &current_table, &ctx, line_num);
                if (result != FRENDO_OK) {
                    fclose(file);
                    return result;
                }

                table_header_parsed = true;
            }
            // Si on voit un chiffre, c'est une ligne de données
            else if (isdigit(trimmed[0])) {
                // Ligne de données
                if (!table_header_parsed) {
                    printf("[ERROR] TABLE data before header at line %d\n", line_num);
                    fclose(file);
                    return FRENDO_ERROR_JSON;
                }

                frendo_error_t result = parse_table_data_line(trimmed, &current_table, line_num);
                if (result != FRENDO_OK) {
                    fclose(file);
                    return result;
                }
            }
            // Sinon, ligne vide ou invalide - ignorer
        }
    }

    // Finaliser la table si on est encore en mode TABLE
    if (in_table && table_header_parsed && current_part) {
        frendo_error_t result = expand_table_to_sequences(&current_table, current_part, &ctx, line_num);
        if (result != FRENDO_OK) {
            fclose(file);
            return result;
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
            printf("  Part %d: BASS[%d] MS20[%d] HAPINESTRIANGLE[%d] HAPINESSQUARE[%d] SAMPLERVOICE[%d] SAMPLERFX[%d]\n",
                   j + 1,
                   part->BASS.sequence.count, part->MS20.sequence.count,
                   part->HAPINESTRIANGLE.sequence.count, part->HAPINESSQUARE.sequence.count,
                   part->SAMPLERVOICE.sequence.count, part->SAMPLERFX.sequence.count);
        }
        printf("\n");
    }

    printf("─────────────────────────────────────────\n");
}
