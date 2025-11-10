/* 
 * main.c - Programme principal de Basic Frendo C
 * 
 * Version C optimisée du séquenceur MIDI Basic Frendo
 * Conçue pour des performances live avec latence minimale
 * 
 * Usage: ./basic-frendo [fichier_set.json]
 */

#include "basic_frendo.h"
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

// Variables globales pour la gestion propre de l'arrêt
static midi_interface_t g_midi;
static bool g_running = true;

/**
 * Gestionnaire de signal pour arrêt propre (Ctrl+C)
 */
void signal_handler(int signal) {
    printf("\n[SIGNAL] Received signal %d, shutting down...\n", signal);
    g_running = false;
    
    // Forcer l'arrêt immédiat si nécessaire
    cleanup_midi_interface(&g_midi);
    exit(0);
}



/**
 * Boucle principale d'écoute MIDI
 */
void midi_loop(midi_interface_t *midi, song_set_t *song_set, frendo_state_t *state) {
    printf("[INIT] Entering MIDI event loop...\n");
    printf("[INFO] Press Ctrl+C to stop\n\n");
    
    // Configuration du polling ALSA
    int npfds = snd_seq_poll_descriptors_count(midi->seq_handle, POLLIN);
    struct pollfd *pfds = malloc(npfds * sizeof(struct pollfd));
    
    if (!pfds) {
        printf("[ERROR] Cannot allocate poll descriptors\n");
        return;
    }
    
    snd_seq_poll_descriptors(midi->seq_handle, pfds, npfds, POLLIN);
    
    printf("Ready to play! Waiting for MIDI input...\n");
    printf("─────────────────────────────────────────\n");
    
    while (g_running) {
        // Attendre des événements MIDI avec timeout de 100ms
        if (poll(pfds, npfds, 100) > 0) {
            
            // Traiter tous les événements disponibles
            snd_seq_event_t *ev;
            while (snd_seq_event_input(midi->seq_handle, &ev) >= 0) {
                
                // Traiter l'événement MIDI
                process_midi_message(ev, midi, song_set, state);
                
                // Libérer l'événement
                snd_seq_free_event(ev);
            }
        }
        
        // Petite pause pour éviter une utilisation CPU excessive
        usleep(1000); // 1ms
    }
    
    free(pfds);
    printf("\n[INFO] MIDI loop terminated\n");
}

/**
 * Fonction principale
 */
int main(int argc, char *argv[]) {
    frendo_error_t result;
    song_set_t song_set;
    frendo_state_t state = {0}; // Initialiser tout à zéro

    // Initialiser le générateur de nombres aléatoires
    srand(time(NULL));

    // Afficher la bannière
    print_banner();

    // Installer le gestionnaire de signal pour arrêt propre
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // === 1. SÉLECTION DU SET ===

    printf("\n[INIT] Scanning sets directory...\n");

    char set_names[MAX_SONGS][MAX_NAME_LENGTH];
    int set_count = list_available_sets("sets", set_names, MAX_SONGS);

    if (set_count == 0) {
        printf("[ERROR] No sets found in 'sets/' directory\n");
        printf("[HELP] Create a set directory in 'sets/' with .frendo files\n");
        return EXIT_FAILURE;
    }

    printf("\nAvailable sets:\n");
    for (int i = 0; i < set_count; i++) {
        printf("  %d. %s\n", i + 1, set_names[i]);
    }

    int selection = 0;
    printf("\nSelect set [1-%d]: ", set_count);
    if (scanf("%d", &selection) != 1 || selection < 1 || selection > set_count) {
        printf("[ERROR] Invalid selection\n");
        return EXIT_FAILURE;
    }

    // Construire le chemin du set sélectionné
    char set_path[MAX_FILENAME_LENGTH];
    snprintf(set_path, sizeof(set_path), "sets/%s", set_names[selection - 1]);

    printf("\n[INIT] Loading set: %s\n", set_names[selection - 1]);

    // === 2. CHARGEMENT DU SET ===

    result = load_frendo_set(set_path, &song_set);
    if (result != FRENDO_OK) {
        printf("[ERROR] Failed to load set: %s\n", error_to_string(result));
        return EXIT_FAILURE;
    }

    if (song_set.song_count == 0) {
        printf("[ERROR] No songs found in the set\n");
        return EXIT_FAILURE;
    }

    printf("[INFO] Set loaded: %d songs\n", song_set.song_count);
    for (int i = 0; i < song_set.song_count; i++) {
        printf("       %d. %s\n", i + 1, song_set.songs[i].name);
    }
    printf("\n");

    // === 3. INITIALISATION MIDI ===
    
    printf("[INIT] Initializing MIDI interface...\n");
    
    result = init_midi_interface(&g_midi);
    if (result != FRENDO_OK) {
        printf("[ERROR] Failed to initialize MIDI: %s\n", error_to_string(result));
        return EXIT_FAILURE;
    }
    
    // Configuration des ports MIDI (VirMIDI 1-0)
    result = setup_midi_ports(&g_midi, "VirMIDI 1-0");
    if (result != FRENDO_OK) {
        printf("[ERROR] Failed to setup MIDI ports: %s\n", error_to_string(result));
        printf("[HELP] Make sure VirMIDI is loaded: sudo modprobe snd-virmidi midi_devs=1\n");
        cleanup_midi_interface(&g_midi);
        return EXIT_FAILURE;
    }
    
    // === 4. ÉTAT INITIAL ===
    
    // Initialiser l'état du système
    state.song_index = 0;
    state.part_index = 0;
    state.cat_note_index = 0;
    state.ms20_note_index = 0;
    state.hapinestriangle_note_index = 0;
    state.hapinessquare_note_index = 0;
    state.samplervoice_note_index = 0;
    state.samplerfx_note_index = 0;

    printf("[INIT] Initial state:\n");
    printf("       Song: '%s' (1/%d)\n",
           song_set.songs[0].name, song_set.song_count);
    printf("       Part: 1/%d\n", song_set.songs[0].part_count);
    printf("       CAT[%d] MS20[%d] HAPINESTRIANGLE[%d] HAPINESSQUARE[%d] SAMPLERVOICE[%d] SAMPLERFX[%d]\n",
           song_set.songs[0].parts[0].CAT.sequence.count,
           song_set.songs[0].parts[0].MS20.sequence.count,
           song_set.songs[0].parts[0].HAPINESTRIANGLE.sequence.count,
           song_set.songs[0].parts[0].HAPINESSQUARE.sequence.count,
           song_set.songs[0].parts[0].SAMPLERVOICE.sequence.count,
           song_set.songs[0].parts[0].SAMPLERFX.sequence.count);
    
    // === 5. BOUCLE PRINCIPALE ===
    
    printf("\n[READY] Basic Frendo is ready to play!\n");
    printf("─────────────────────────────────────────\n");

    
    // Entrer dans la boucle d'écoute MIDI
    midi_loop(&g_midi, &song_set, &state);
    
    // === 6. NETTOYAGE ===
    
    printf("[CLEANUP] Shutting down Basic Frendo...\n");
    cleanup_midi_interface(&g_midi);
    
    printf("[INFO] Basic Frendo terminated cleanly\n");
    printf("Thank you for using Basic Frendo! 🎵\n\n");
    
    return EXIT_SUCCESS;
}