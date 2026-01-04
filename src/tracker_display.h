/*
 * tracker_display.h - Affichage tracker temps réel
 *
 * Affichage visuel de type tracker pour voir l'état des séquences
 */

#ifndef TRACKER_DISPLAY_H
#define TRACKER_DISPLAY_H

#include "basic_frendo.h"

/**
 * Initialise l'affichage tracker (clear screen)
 */
void init_tracker_display(void);

/**
 * Met à jour l'affichage tracker avec l'état actuel
 *
 * @param song_set Le set de songs
 * @param state L'état actuel du séquenceur
 */
void update_tracker_display(song_set_t *song_set, frendo_state_t *state);

/**
 * Nettoie l'affichage tracker (restaure l'écran)
 */
void cleanup_tracker_display(void);

#endif // TRACKER_DISPLAY_H
