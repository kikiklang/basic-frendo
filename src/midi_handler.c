/* 
 * midi_handler.c - Interface ALSA pour Basic Frendo
 * 
 * Gère l'ouverture, la configuration et la communication avec les ports MIDI
 * Utilise le séquenceur ALSA pour une latence minimale
 */

#include "basic_frendo.h"
#include <alloca.h>

/**
 * Initialise l'interface MIDI ALSA
 * Ouvre le séquenceur et crée un client ALSA
 */
frendo_error_t init_midi_interface(midi_interface_t *midi) {
    if (!midi) {
        return FRENDO_ERROR_MIDI;
    }
    
    printf("[INIT] Initializing ALSA MIDI interface...\n");
    
    // Initialiser la structure
    memset(midi, 0, sizeof(midi_interface_t));
    midi->input_port = -1;
    midi->output_port = -1;
    
    // Ouvrir le séquenceur ALSA
    int result = snd_seq_open(&midi->seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (result < 0) {
        printf("[ERROR] Cannot open ALSA sequencer: %s\n", snd_strerror(result));
        return FRENDO_ERROR_ALSA;
    }
    
    // Définir le nom du client
    snd_seq_set_client_name(midi->seq_handle, "Basic Frendo");
    
    // Obtenir l'ID du client
    midi->client_id = snd_seq_client_id(midi->seq_handle);
    printf("[INIT] ALSA client created with ID: %d\n", midi->client_id);
    
    // Créer un port d'entrée
    midi->input_port = snd_seq_create_simple_port(midi->seq_handle, "Input",
                                                  SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                  SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    
    if (midi->input_port < 0) {
        printf("[ERROR] Cannot create input port: %s\n", snd_strerror(midi->input_port));
        snd_seq_close(midi->seq_handle);
        return FRENDO_ERROR_ALSA;
    }
    
    // Créer un port de sortie
    midi->output_port = snd_seq_create_simple_port(midi->seq_handle, "Output",
                                                   SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                                                   SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
    
    if (midi->output_port < 0) {
        printf("[ERROR] Cannot create output port: %s\n", snd_strerror(midi->output_port));
        snd_seq_close(midi->seq_handle);
        return FRENDO_ERROR_ALSA;
    }
    
    printf("[INIT] MIDI ports created - Input: %d, Output: %d\n", 
           midi->input_port, midi->output_port);
    
    return FRENDO_OK;
}

/**
 * Trouve et connecte automatiquement aux ports VirMIDI
 */
frendo_error_t setup_midi_ports(midi_interface_t *midi, const char *port_name) {
    if (!midi || !port_name) {
        return FRENDO_ERROR_MIDI;
    }
    
    printf("[INIT] Looking for MIDI port: %s\n", port_name);
    
    // Variables pour parcourir les clients et ports
    snd_seq_client_info_t *client_info;
    snd_seq_port_info_t *port_info;
    
    snd_seq_client_info_alloca(&client_info);
    snd_seq_port_info_alloca(&port_info);
    
    // Parcourir tous les clients ALSA
    snd_seq_client_info_set_client(client_info, -1);
    while (snd_seq_query_next_client(midi->seq_handle, client_info) >= 0) {
        int client_id = snd_seq_client_info_get_client(client_info);
        
        // Parcourir tous les ports de ce client
        snd_seq_port_info_set_client(port_info, client_id);
        snd_seq_port_info_set_port(port_info, -1);
        
        while (snd_seq_query_next_port(midi->seq_handle, port_info) >= 0) {
            const char *current_port_name = snd_seq_port_info_get_name(port_info);
            
            // Vérifier si c'est le port qu'on cherche
            if (current_port_name && strstr(current_port_name, port_name)) {
                int port_id = snd_seq_port_info_get_port(port_info);
                unsigned int caps = snd_seq_port_info_get_capability(port_info);
                
                printf("[INFO] Found port '%s' at %d:%d\n", current_port_name, client_id, port_id);
                
                // Connecter l'entrée si le port peut écrire
                if (caps & SND_SEQ_PORT_CAP_READ) {
                    int result = snd_seq_connect_from(midi->seq_handle, midi->input_port, client_id, port_id);
                    if (result < 0) {
                        printf("[WARNING] Cannot connect input from %d:%d: %s\n", 
                               client_id, port_id, snd_strerror(result));
                    } else {
                        printf("[INIT] Connected input from %s (%d:%d)\n", 
                               current_port_name, client_id, port_id);
                    }
                }
                
                // Connecter la sortie si le port peut lire
                if (caps & SND_SEQ_PORT_CAP_WRITE) {
                    int result = snd_seq_connect_to(midi->seq_handle, midi->output_port, client_id, port_id);
                    if (result < 0) {
                        printf("[WARNING] Cannot connect output to %d:%d: %s\n", 
                               client_id, port_id, snd_strerror(result));
                    } else {
                        printf("[INIT] Connected output to %s (%d:%d)\n", 
                               current_port_name, client_id, port_id);
                    }
                }
                
                // Port trouvé et connecté
                return FRENDO_OK;
            }
        }
    }
    
    printf("[ERROR] MIDI port '%s' not found\n", port_name);
    printf("[INFO] Available ports:\n");
    
    // Lister les ports disponibles pour aider au debug
    snd_seq_client_info_set_client(client_info, -1);
    while (snd_seq_query_next_client(midi->seq_handle, client_info) >= 0) {
        int client_id = snd_seq_client_info_get_client(client_info);
        const char *client_name = snd_seq_client_info_get_name(client_info);
        
        snd_seq_port_info_set_client(port_info, client_id);
        snd_seq_port_info_set_port(port_info, -1);
        
        while (snd_seq_query_next_port(midi->seq_handle, port_info) >= 0) {
            int port_id = snd_seq_port_info_get_port(port_info);
            const char *port_name_debug = snd_seq_port_info_get_name(port_info);
            unsigned int caps = snd_seq_port_info_get_capability(port_info);
            
            printf("[INFO]   %d:%d - %s:%s (caps: 0x%x)\n", 
                   client_id, port_id, client_name, port_name_debug, caps);
        }
    }
    
    return FRENDO_ERROR_MIDI;
}

/**
 * Traite un message MIDI reçu et déclenche les actions appropriées
 */
void process_midi_message(const snd_seq_event_t *event, 
                         midi_interface_t *midi, 
                         song_set_t *song_set, 
                         frendo_state_t *state) {
    
    if (!event || !midi || !song_set || !state) {
        return;
    }
    
    // Ne traiter que les événements NOTE ON
    if (event->type != SND_SEQ_EVENT_NOTEON) {
        return;
    }
    
    uint8_t channel = event->data.note.channel;
    uint8_t note = event->data.note.note;
    uint8_t velocity = event->data.note.velocity;
    
    // Ignorer les NOTE ON avec vélocité 0 (équivalent à NOTE OFF)
    if (velocity == 0) {
        return;
    }
    
    printf("[MIDI IN]  Channel: %d | Note: %d | Velocity: %d\n", 
           channel, note, velocity);
    
    // Traiter selon le canal MIDI
    switch (channel) {
        case 0:
            // Channel 1 (index 0): Jouer la note bass suivante
            play_bass_note(midi, song_set, state);
            break;
        case 1:
            // Channel 2 (index 1): Jouer la note melody suivante
            play_melody_note(midi, song_set, state);
            break;
        case 2:
            // Channel 3 (index 2): Changer selon la note reçue
            if (note == 48) {
                update_song(state, song_set);
            } else if (note == 49) {
                update_part(state, song_set);
            }
            break;
        default:
            // Autres canaux ignorés
            printf("[INFO] Ignored MIDI on channel %d\n", channel);
            break;
    }
    printf("─────────────────────────────────────────\n");
}

/**
 * Envoie une note MIDI (NOTE ON suivi de NOTE OFF)
 */
void send_midi_note(midi_interface_t *midi, uint8_t channel, uint8_t note) {
    if (!midi || !midi->seq_handle) {
        return;
    }
    
    // Ignorer les notes à 0 (silence)
    if (note == 0) {
        printf("[MIDI OUT] Channel: %d | Silent note (skipped)\n", channel);
        return;
    }
    
    snd_seq_event_t ev;
    snd_seq_ev_clear(&ev);
    
    // Configurer l'événement NOTE ON
    snd_seq_ev_set_source(&ev, midi->output_port);
    snd_seq_ev_set_subs(&ev);
    snd_seq_ev_set_direct(&ev);
    
    ev.type = SND_SEQ_EVENT_NOTEON;
    ev.data.note.channel = channel;
    ev.data.note.note = note;
    ev.data.note.velocity = MIDI_VELOCITY;
    
    // Envoyer NOTE ON
    int result = snd_seq_event_output(midi->seq_handle, &ev);
    if (result < 0) {
        printf("[ERROR] Failed to send NOTE ON: %s\n", snd_strerror(result));
        return;
    }
    
    // Forcer l'envoi immédiat
    snd_seq_drain_output(midi->seq_handle);
    
    printf("[MIDI OUT] Channel: %d | Note: %d | Velocity: %d\n", 
           channel, note, MIDI_VELOCITY);
    
    // Attendre un court délai (10ms comme dans la version Node.js)
    usleep(10000); // 10ms = 10000 microsecondes
    
    // Configurer et envoyer NOTE OFF
    ev.type = SND_SEQ_EVENT_NOTEOFF;
    ev.data.note.velocity = 0;
    
    result = snd_seq_event_output(midi->seq_handle, &ev);
    if (result < 0) {
        printf("[ERROR] Failed to send NOTE OFF: %s\n", snd_strerror(result));
        return;
    }
    
    snd_seq_drain_output(midi->seq_handle);
}

/**
 * Nettoie les ressources MIDI
 */
void cleanup_midi_interface(midi_interface_t *midi) {
    if (!midi) {
        return;
    }
    
    printf("[CLEANUP] Closing MIDI interface...\n");
    
    if (midi->seq_handle) {
        // Fermer les ports
        if (midi->input_port >= 0) {
            snd_seq_delete_simple_port(midi->seq_handle, midi->input_port);
        }
        if (midi->output_port >= 0) {
            snd_seq_delete_simple_port(midi->seq_handle, midi->output_port);
        }
        
        // Fermer le séquenceur
        snd_seq_close(midi->seq_handle);
    }
    
    memset(midi, 0, sizeof(midi_interface_t));
    printf("[CLEANUP] MIDI interface closed\n");
}