/* 
 * json_parser.c - Parseur JSON pour les fichiers de sets Basic Frendo
 * 
 * Utilise la bibliothèque cJSON pour charger les configurations de chansons
 * Format attendu : JSON avec structure songs/parts/bass/melody
 */

#include "basic_frendo.h"
#include <cjson/cJSON.h>

/**
 * Convertit une valeur JSON en note MIDI (0-127)
 * Les fichiers JSON contiennent maintenant directement des valeurs MIDI
 */
static uint8_t json_to_midi_value(cJSON *json_item) {
    if (!json_item) {
        return 0; // Note silencieuse
    }
    
    // Si c'est un nombre, l'utiliser directement
    if (cJSON_IsNumber(json_item)) {
        int midi_value = cJSON_GetNumberValue(json_item);
        if (midi_value >= 0 && midi_value <= 127) {
            return (uint8_t)midi_value;
        } else {
            printf("[WARNING] Invalid MIDI value %d, using 0 (silence)\n", midi_value);
            return 0;
        }
    }
    
    // Si c'est une chaîne, essayer de la convertir en nombre
    if (cJSON_IsString(json_item)) {
        const char *str_value = cJSON_GetStringValue(json_item);
        if (str_value && strlen(str_value) > 0) {
            int midi_value = atoi(str_value);
            if (midi_value >= 0 && midi_value <= 127) {
                return (uint8_t)midi_value;
            }
        }
        printf("[WARNING] Cannot convert '%s' to MIDI value, using 0 (silence)\n", str_value);
        return 0;
    }
    
    printf("[WARNING] Invalid JSON type for MIDI note, using 0 (silence)\n");
    return 0;
}

/**
 * Parse un tableau JSON de notes en séquence MIDI
 */
static frendo_error_t parse_note_sequence(cJSON *json_array, note_sequence_t *sequence) {
    if (!json_array || !sequence) {
        return FRENDO_ERROR_JSON;
    }
    
    // Initialiser la séquence
    memset(sequence, 0, sizeof(note_sequence_t));
    
    if (!cJSON_IsArray(json_array)) {
        printf("[WARNING] Expected array for note sequence\n");
        return FRENDO_ERROR_JSON;
    }
    
    int array_size = cJSON_GetArraySize(json_array);
    if (array_size > MAX_NOTES_PER_SEQ) {
        printf("[WARNING] Note sequence too long (%d), truncating to %d\n", 
               array_size, MAX_NOTES_PER_SEQ);
        array_size = MAX_NOTES_PER_SEQ;
    }
    
    // Parser chaque note du tableau
    for (int i = 0; i < array_size; i++) {
        cJSON *note_item = cJSON_GetArrayItem(json_array, i);
        if (!note_item) continue;
        
        // Convertir l'élément JSON en valeur MIDI
        sequence->notes[i] = json_to_midi_value(note_item);
    }
    
    sequence->count = array_size;
    return FRENDO_OK;
}

/**
 * Parse une partie d'une chanson depuis JSON
 */
static frendo_error_t parse_song_part(cJSON *part_json, song_part_t *part) {
    if (!part_json || !part) {
        return FRENDO_ERROR_JSON;
    }
    
    // Initialiser la partie
    memset(part, 0, sizeof(song_part_t));
    
    // Parser la séquence de basse
    cJSON *bass_json = cJSON_GetObjectItem(part_json, "bass");
    if (bass_json) {
        frendo_error_t result = parse_note_sequence(bass_json, &part->bass);
        if (result != FRENDO_OK) {
            printf("[ERROR] Failed to parse bass sequence\n");
            return result;
        }
    } else {
        printf("[WARNING] No bass sequence found in part\n");
    }
    
    // Parser la séquence de mélodie
    cJSON *melody_json = cJSON_GetObjectItem(part_json, "melody");
    if (melody_json) {
        frendo_error_t result = parse_note_sequence(melody_json, &part->melody);
        if (result != FRENDO_OK) {
            printf("[ERROR] Failed to parse melody sequence\n");
            return result;
        }
    } else {
        printf("[WARNING] No melody sequence found in part\n");
    }
    
    return FRENDO_OK;
}

/**
 * Parse une chanson complète depuis JSON
 */
static frendo_error_t parse_song(cJSON *song_json, song_t *song) {
    if (!song_json || !song) {
        return FRENDO_ERROR_JSON;
    }
    
    // Initialiser la chanson
    memset(song, 0, sizeof(song_t));
    
    // Parser le nom de la chanson
    cJSON *name_json = cJSON_GetObjectItem(song_json, "name");
    if (name_json && cJSON_IsString(name_json)) {
        strncpy(song->name, cJSON_GetStringValue(name_json), MAX_NAME_LENGTH - 1);
        song->name[MAX_NAME_LENGTH - 1] = '\0';
    } else {
        strcpy(song->name, "Untitled Song");
    }
    
    // Parser les parties
    cJSON *parts_json = cJSON_GetObjectItem(song_json, "parts");
    if (!parts_json || !cJSON_IsArray(parts_json)) {
        printf("[ERROR] No parts array found in song '%s'\n", song->name);
        return FRENDO_ERROR_JSON;
    }
    
    int parts_count = cJSON_GetArraySize(parts_json);
    if (parts_count > MAX_PARTS_PER_SONG) {
        printf("[WARNING] Too many parts (%d) in song '%s', truncating to %d\n", 
               parts_count, song->name, MAX_PARTS_PER_SONG);
        parts_count = MAX_PARTS_PER_SONG;
    }
    
    // Parser chaque partie
    for (int i = 0; i < parts_count; i++) {
        cJSON *part_json = cJSON_GetArrayItem(parts_json, i);
        if (!part_json) continue;
        
        frendo_error_t result = parse_song_part(part_json, &song->parts[i]);
        if (result != FRENDO_OK) {
            printf("[ERROR] Failed to parse part %d in song '%s'\n", i + 1, song->name);
            return result;
        }
    }
    
    song->part_count = parts_count;
    printf("[INFO] Loaded song '%s' with %d parts\n", song->name, song->part_count);
    
    return FRENDO_OK;
}

/**
 * Charge un set de chansons depuis un fichier JSON
 */
frendo_error_t load_song_set(const char *filename, song_set_t *song_set) {
    if (!filename || !song_set) {
        return FRENDO_ERROR_FILE;
    }
    
    printf("[INIT] Loading song set: %s\n", filename);
    
    // Initialiser le set
    memset(song_set, 0, sizeof(song_set_t));
    
    // Lire le fichier JSON
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("[ERROR] Cannot open file: %s\n", filename);
        return FRENDO_ERROR_FILE;
    }
    
    // Obtenir la taille du fichier
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allouer le buffer et lire le contenu
    char *json_string = malloc(file_size + 1);
    if (!json_string) {
        fclose(file);
        return FRENDO_ERROR_MEMORY;
    }
    
    size_t read_size = fread(json_string, 1, file_size, file);
    json_string[read_size] = '\0';
    fclose(file);
    
    // Parser le JSON
    cJSON *json = cJSON_Parse(json_string);
    free(json_string);
    
    if (!json) {
        printf("[ERROR] Invalid JSON format in file: %s\n", filename);
        printf("[ERROR] JSON Error: %s\n", cJSON_GetErrorPtr());
        return FRENDO_ERROR_JSON;
    }
    
    // Parser le tableau de chansons
    cJSON *songs_json = cJSON_GetObjectItem(json, "songs");
    if (!songs_json || !cJSON_IsArray(songs_json)) {
        printf("[ERROR] No 'songs' array found in JSON\n");
        cJSON_Delete(json);
        return FRENDO_ERROR_JSON;
    }
    
    int songs_count = cJSON_GetArraySize(songs_json);
    if (songs_count > MAX_SONGS) {
        printf("[WARNING] Too many songs (%d), truncating to %d\n", 
               songs_count, MAX_SONGS);
        songs_count = MAX_SONGS;
    }
    
    // Parser chaque chanson
    for (int i = 0; i < songs_count; i++) {
        cJSON *song_json = cJSON_GetArrayItem(songs_json, i);
        if (!song_json) continue;
        
        frendo_error_t result = parse_song(song_json, &song_set->songs[i]);
        if (result != FRENDO_OK) {
            printf("[ERROR] Failed to parse song %d\n", i + 1);
            cJSON_Delete(json);
            return result;
        }
    }
    
    song_set->song_count = songs_count;
    
    // Nettoyer et terminer
    cJSON_Delete(json);
    
    printf("[INIT] Loaded %d songs successfully\n", song_set->song_count);
    return FRENDO_OK;
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
            printf("  Part %d: Bass[%d notes] Melody[%d notes]\n", 
                   j + 1, part->bass.count, part->melody.count);
        }
        printf("\n");
    }
    
    printf("─────────────────────────────────────────\n");
}