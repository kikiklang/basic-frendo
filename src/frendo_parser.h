/*
 * frendo_parser.h - Parser custom pour format .frendo
 *
 * Format minimal haute performance pour séquences MIDI live
 * Pas de dépendances externes, parsing direct en mémoire
 */

#ifndef FRENDO_PARSER_H
#define FRENDO_PARSER_H

#include "basic_frendo.h"

/**
 * Charge un fichier .frendo et remplit une chanson
 *
 * @param filename Chemin vers le fichier .frendo
 * @param song Pointeur vers la structure song à remplir
 * @param song_name Nom de la chanson (extrait du nom de fichier)
 * @return FRENDO_OK si succès, code d'erreur sinon
 */
frendo_error_t load_frendo_file(const char *filename, song_t *song, const char *song_name);

/**
 * Charge tous les fichiers .frendo d'un répertoire
 *
 * @param directory Chemin vers le répertoire du set
 * @param song_set Pointeur vers la structure song_set à remplir
 * @return FRENDO_OK si succès, code d'erreur sinon
 */
frendo_error_t load_frendo_set(const char *directory, song_set_t *song_set);

/**
 * Liste les sets disponibles dans le répertoire sets/
 *
 * @param sets_dir Chemin vers le répertoire racine des sets
 * @param set_names Tableau pour stocker les noms de sets trouvés
 * @param max_sets Taille maximale du tableau
 * @return Nombre de sets trouvés
 */
int list_available_sets(const char *sets_dir, char set_names[][MAX_NAME_LENGTH], int max_sets);

#endif // FRENDO_PARSER_H
