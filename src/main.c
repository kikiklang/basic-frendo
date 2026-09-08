#define _POSIX_C_SOURCE 200809L

/*
 * main.c - Programme principal de Basic Frendo C
 * 
 * Version C optimisée du séquenceur MIDI Basic Frendo
 * Conçue pour des performances live avec latence minimale
 * 
 * Usage: ./basic-frendo
 */

#include "basic_frendo.h"
#include "tracker_display.h"
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

// Variables globales pour la gestion propre de l'arrêt
static midi_interface_t g_midi;
static volatile sig_atomic_t g_running = 1;

/**
 * Gestionnaire de signal pour arrêt propre (Ctrl+C)
 * Ne fait que positionner le flag — pas d'I/O, pas d'exit()
 */
static void signal_handler(int sig) {
    (void)sig;
    g_running = 0;
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

    // Initialiser l'affichage tracker
    init_tracker_display();

    // Afficher l'état initial
    update_tracker_display(song_set, state);

    while (g_running != 0) {
        // Attendre des événements MIDI avec timeout de 100ms
        if (poll(pfds, npfds, 100) > 0) {
            
            // Traiter tous les événements disponibles
            snd_seq_event_t *ev;
            while (g_running != 0 && snd_seq_event_input(midi->seq_handle, &ev) >= 0) {
                process_midi_message(ev, midi, song_set, state);
                update_tracker_display(song_set, state);
                snd_seq_free_event(ev);
            }
        }
        
        // Petite pause pour éviter une utilisation CPU excessive
        usleep(1000); // 1ms
    }

    // Nettoyer l'affichage tracker
    cleanup_tracker_display();

    free(pfds);
    printf("\n[INFO] MIDI loop terminated\n");
}

/**
 * Fonction principale
 */
int main(void) {
    frendo_error_t result;
    song_set_t song_set;
    frendo_state_t state = {0}; // Initialiser tout à zéro

    // Initialiser le générateur de nombres aléatoires
    srand(time(NULL));

    // Afficher la bannière
    print_banner();

    // Installer le gestionnaire de signal (sans SA_RESTART pour que poll() soit interrompu)
    struct sigaction sa = { .sa_handler = signal_handler, .sa_flags = 0 };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

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
    
    printf("[INIT] Initial state:\n");
    printf("       Song: '%s' (1/%d)\n",
           song_set.songs[0].name, song_set.song_count);
    printf("       Part: 1/%d\n", song_set.songs[0].part_count);
    printf("       BASS[%d] MS20[%d] SAMPLERVOICE[%d] SAMPLERFX[%d]\n",
           song_set.songs[0].parts[0].BASS.sequence.count,
           song_set.songs[0].parts[0].MS20.sequence.count,
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