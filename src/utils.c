/* 
 * utils.c - Fonctions utilitaires pour Basic Frendo
 * 
 * Conversion de notes, affichage, gestion d'erreurs
 */

#include "basic_frendo.h"
#include <ctype.h>

/**
 * Note: La conversion nom de note -> MIDI n'est plus nécessaire
 * Les fichiers JSON contiennent maintenant directement les valeurs MIDI (0-127)
 */

/**
 * Convertit un code d'erreur en chaîne lisible
 */
const char* error_to_string(frendo_error_t error) {
    switch (error) {
        case FRENDO_OK:          return "Success";
        case FRENDO_ERROR_FILE:  return "File error";
        case FRENDO_ERROR_JSON:  return "JSON parsing error";
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