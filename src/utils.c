/* 
 * utils.c - Fonctions utilitaires pour Basic Frendo
 * 
 * Conversion de notes, affichage, gestion d'erreurs
 */

#include "basic_frendo.h"
#include <ctype.h>

/**
 * Convertit un code d'erreur en chaîne lisible
 */
const char* error_to_string(frendo_error_t error) {
    switch (error) {
        case FRENDO_OK:          return "Success";
        case FRENDO_ERROR_FILE:  return "File error";
        case FRENDO_ERROR_PARSE: return "Parse error";
        case FRENDO_ERROR_MIDI:  return "MIDI error";
        case FRENDO_ERROR_MEMORY: return "Memory allocation error";
        case FRENDO_ERROR_ALSA:  return "ALSA error";
        default:                 return "Unknown error";
    }
}

/**
 * Affiche la bannière de démarrage
 */
void print_banner(void) {
    printf("\n");
    printf("═══════════════════════════════════════\n");
    printf("  BASIC FRENDO (C version) - v1.0\n");
    printf("═══════════════════════════════════════\n");
    printf("  High-performance MIDI sequencer\n");
    printf("  Built for live drumming performances\n");
    printf("═══════════════════════════════════════\n\n");
}

/**
 * Affiche un changement d'état avec formatage spécial
 */
void print_state_change(void) {
    printf("═══════════════════════════════════════════\n");
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
            printf("  Part %d: BASS[%d] MS20[%d] SAMPLERVOICE[%d] SAMPLERFX[%d]\n",
                   j + 1,
                   part->BASS.sequence.count, part->MS20.sequence.count,
                   part->SAMPLERVOICE.sequence.count, part->SAMPLERFX.sequence.count);
        }
        printf("\n");
    }

    printf("─────────────────────────────────────────\n");
}