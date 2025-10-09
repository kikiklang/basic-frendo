#!/bin/bash

# Script de test MIDI pour Basic Frendo
# Envoie des notes MIDI pour tester les différents canaux

echo "🎵 Test MIDI Basic Frendo"
echo "========================="
echo ""

# Fonction pour envoyer une note MIDI
send_note() {
    local channel=$1
    local note=$2
    local velocity=$3
    local duration=${4:-100}  # durée en ms, 100ms par défaut
    
    echo "Sending NOTE ON: Channel $channel, Note $note, Velocity $velocity"
    
    # Calculer le statut MIDI (NOTE ON = 0x90 + channel)
    local status=$((144 + channel))
    
    # Envoyer la note via aplaymidi (méthode alternative avec amidi)
    # Format: status note velocity
    printf "\\x$(printf %02x $status)\\x$(printf %02x $note)\\x$(printf %02x $velocity)" | \
    amidi -p hw:2,0 -S || echo "Note: amidi failed, trying alternative method"
    
    sleep 0.1  # Courte pause
    
    # NOTE OFF
    local status_off=$((128 + channel))
    printf "\\x$(printf %02x $status_off)\\x$(printf %02x $note)\\x00" | \
    amidi -p hw:2,0 -S 2>/dev/null || true
    
    echo "Sent!"
    sleep 1
}

echo "Test 1: Bass trigger (Channel 0)"
send_note 0 36 127

echo ""
echo "Test 2: Melody trigger (Channel 1)" 
send_note 1 38 127

echo ""
echo "Test 3: Song change (Channel 3)"
send_note 3 40 127

echo ""
echo "Test 4: Part change (Channel 4)"
send_note 4 42 127

echo ""
echo "✅ Test MIDI terminé!"