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
    char set_filename[MAX_FILENAME_LENGTH] = {0};
    
    // Afficher la bannière
    print_banner();
    
    // Installer le gestionnaire de signal pour arrêt propre
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // === 1. VÉRIFICATION DES ARGUMENTS ===
    
    if (argc != 2) {
        printf("[ERROR] Usage: %s <song-set-file.json>\n", argv[0]);
        printf("[INFO] Example: %s sets/ido-entroido-2025.json\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    strncpy(set_filename, argv[1], sizeof(set_filename) - 1);
    set_filename[sizeof(set_filename) - 1] = '\0';
    printf("[INIT] Using song set file: %s\n", set_filename);
    
    // Utiliser le chemin tel quel (pas de préfixe automatique)
    char full_path[MAX_FILENAME_LENGTH];
    strncpy(full_path, set_filename, sizeof(full_path) - 1);
    full_path[sizeof(full_path) - 1] = '\0';
    
    // === 2. CHARGEMENT DU FICHIER DE SET ===
    
    result = load_song_set(full_path, &song_set);
    if (result != FRENDO_OK) {
        printf("[ERROR] Failed to load song set: %s\n", error_to_string(result));
        printf("[HELP] Make sure the file exists and has valid JSON format\n");
        return EXIT_FAILURE;
    }
    
    // Afficher les informations du set chargé
    print_song_set_info(&song_set);
    
    if (song_set.song_count == 0) {
        printf("[ERROR] No songs found in the set file\n");
        return EXIT_FAILURE;
    }
    
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
    state.bass_note_index = 0;
    state.melody_note_index = 0;
    
    printf("[INIT] Initial state:\n");
    printf("       Song: '%s' (1/%d)\n", 
           song_set.songs[0].name, song_set.song_count);
    printf("       Part: 1/%d\n", song_set.songs[0].part_count);
    printf("       Bass notes: %d, Melody notes: %d\n",
           song_set.songs[0].parts[0].bass.count,
           song_set.songs[0].parts[0].melody.count);
    
    // === 5. BOUCLE PRINCIPALE ===
    
    printf("\n[READY] Basic Frendo is ready to play!\n");
    printf("─────────────────────────────────────────\n");
    printf("MIDI Channels (dans Bitwig):\n");
    printf("  • Channel 1: Trigger next BASS note\n");
    printf("  • Channel 2: Trigger next MELODY note\n");
    printf("  • Channel 3: Switch to next SONG\n");
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