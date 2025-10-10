/*
 * midi2json.c - Convertisseur minimal MIDI (.mid) vers JSON Basic Frendo
 *
 * Dépendances :
 *   - libsmf (https://github.com/stump/libsmf)
 *   - cJSON (déjà utilisée dans le projet)
 *
 * Compilation :
 *   gcc midi2json.c -o midi2json -lsmf -lcjson
 *
 * Usage :
 *   ./midi2json fichier.mid output.json
 */

#include <stdio.h>
#include <stdlib.h>
#include <smf.h>
#include <cjson/cJSON.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <input.mid> <output.json>\n", argv[0]);
        return 1;
    }

    smf_t *smf = smf_load(argv[1]);
    if (!smf) {
        printf("Erreur : impossible de charger le fichier MIDI\n");
        return 1;
    }

    cJSON *bass = cJSON_CreateArray();
    cJSON *melody = cJSON_CreateArray();

    // Simple heuristique : track 0 = bass, track 1 = melody
    for (int t = 0; t < smf->number_of_tracks && t < 2; t++) {
        smf_track_t *track = smf->tracks[t];
        for (smf_event_t *ev = track->first_event; ev; ev = ev->next) {
            if (ev->midi_buffer_length >= 3 && (ev->midi_buffer[0] & 0xF0) == 0x90 && ev->midi_buffer[2] > 0) {
                int note = ev->midi_buffer[1];
                if (t == 0) cJSON_AddItemToArray(bass, cJSON_CreateNumber(note));
                else cJSON_AddItemToArray(melody, cJSON_CreateNumber(note));
            }
        }
    }

    cJSON *part = cJSON_CreateObject();
    cJSON_AddItemToObject(part, "bass", bass);
    cJSON_AddItemToObject(part, "melody", melody);

    cJSON *song = cJSON_CreateObject();
    cJSON_AddStringToObject(song, "name", "MIDI Import");
    cJSON *parts = cJSON_CreateArray();
    cJSON_AddItemToArray(parts, part);
    cJSON_AddItemToObject(song, "parts", parts);

    cJSON *root = cJSON_CreateObject();
    cJSON *songs = cJSON_CreateArray();
    cJSON_AddItemToArray(songs, song);
    cJSON_AddItemToObject(root, "songs", songs);

    FILE *f = fopen(argv[2], "w");
    if (!f) {
        printf("Erreur : impossible d'ouvrir le fichier de sortie\n");
        cJSON_Delete(root);
        smf_delete(smf);
        return 1;
    }
    char *json_str = cJSON_Print(root);
    fprintf(f, "%s\n", json_str);
    fclose(f);
    free(json_str);

    cJSON_Delete(root);
    smf_delete(smf);
    printf("✅ Conversion terminée : %s\n", argv[2]);
    return 0;
}
