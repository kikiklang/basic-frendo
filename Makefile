# Makefile for Basic Frendo C version
# Optimisé pour Arch Linux avec ALSA

# === CONFIGURATION ===
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g -D_DEFAULT_SOURCE
LIBS = -lasound -lpthread
TARGET = basic-frendo

# Répertoires
SRCDIR = src
OBJDIR = build
SETSDIR = sets

# Fichiers sources
SOURCES = main.c frendo_parser.c midi_handler.c frendo_core.c utils.c
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)
DEPENDS = $(OBJECTS:.o=.d)

# === RÈGLES PRINCIPALES ===

.PHONY: all clean install uninstall run debug test help deps

# Compilation par défaut
all: $(TARGET)

# Création du programme principal
$(TARGET): $(OBJECTS)
	@echo "🔗 Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)
	@echo "✅ $(TARGET) built successfully!"

# Compilation des objets avec dépendances automatiques
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	@echo "🔨 Compiling $<..."
	$(CC) $(CFLAGS) -I$(SRCDIR) -MMD -MP -c $< -o $@

# Inclure les dépendances si elles existent
-include $(DEPENDS)

# === RÈGLES UTILITAIRES ===

# Nettoyage des fichiers générés
clean:
	@echo "🧹 Cleaning build files..."
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
	@echo "✅ Clean completed!"

# Installation système (optionnel)
install: $(TARGET)
	@echo "📦 Installing $(TARGET) to /usr/local/bin..."
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "✅ $(TARGET) installed successfully!"

# Désinstallation
uninstall:
	@echo "🗑️ Removing $(TARGET) from /usr/local/bin..."
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "✅ $(TARGET) uninstalled!"

# Compilation et lancement immédiat
run: $(TARGET)
	@echo "🚀 Running $(TARGET)..."
	./$(TARGET)

# Compilation avec debug activé
debug: CFLAGS += -DDEBUG -g3 -O0
debug: clean $(TARGET)
	@echo "🐛 Debug version built!"



# Test de compilation sans erreurs
test-compile: clean
	@echo "🧪 Testing compilation..."
	$(CC) $(CFLAGS) -I$(SRCDIR) -c $(SRCDIR)/*.c
	@echo "✅ All files compile without errors!"
	rm -f *.o

# Vérification des dépendances système
deps:
	@echo "🔍 Checking system dependencies..."
	@echo "Checking for ALSA development files..."
	@pkg-config --exists alsa || (echo "❌ ALSA dev files missing. Install with: sudo pacman -S alsa-lib" && exit 1)
	@echo "✅ ALSA found: $$(pkg-config --modversion alsa)"
	@echo "Checking for VirMIDI module..."
	@lsmod | grep -q virmidi && echo "✅ VirMIDI module loaded" || echo "⚠️ VirMIDI not loaded. Load with: sudo modprobe snd-virmidi midi_devs=1"
	@echo "✅ All dependencies checked!"

# === RÈGLES DE TEST MIDI ===

# Test avec aseqdump (surveillance des ports MIDI)
test-midi: $(TARGET)
	@echo "🎵 Starting MIDI test..."
	@echo "1. Starting aseqdump to monitor MIDI output..."
	@echo "2. Launch $(TARGET) in another terminal"
	@echo "3. Press Ctrl+C to stop"
	aseqdump -p "Basic Frendo" || aseqdump

# Lister les ports MIDI disponibles
midi-ports:
	@echo "🎹 Available MIDI ports:"
	aconnect -l

# Vérifier le module VirMIDI
check-virmidi:
	@echo "🔌 Checking VirMIDI status..."
	@lsmod | grep virmidi && echo "✅ VirMIDI module is loaded" || echo "❌ VirMIDI module not loaded"
	@echo "To load VirMIDI: sudo modprobe snd-virmidi midi_devs=1"

# === RÈGLES D'AIDE ===

help:
	@echo "Basic Frendo - Build System"
	@echo "==========================="
	@echo ""
	@echo "Main targets:"
	@echo "  all          Build the program (default)"
	@echo "  clean        Remove build files"
	@echo "  run          Build and run the program"
	@echo "  debug        Build with debug symbols"
	@echo ""
	@echo "Installation:"
	@echo "  install      Install to /usr/local/bin"
	@echo "  uninstall    Remove from /usr/local/bin"
	@echo ""
	@echo "Testing:"
	@echo "  deps         Check system dependencies"
	@echo "  test-compile Test compilation without linking"
	@echo "  test-midi    Start MIDI monitoring"
	@echo "  midi-ports   List available MIDI ports"
	@echo "  check-virmidi Check VirMIDI module status"
	@echo ""
	@echo "Dependencies installation (Arch Linux):"
	@echo "  sudo pacman -S base-devel alsa-lib alsa-utils"
	@echo ""
	@echo "VirMIDI setup:"
	@echo "  sudo modprobe snd-virmidi midi_devs=1"

# === RÈGLES POUR LE DÉVELOPPEMENT ===

# Formatage automatique du code (si clang-format est disponible)
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		echo "🎨 Formatting C code..."; \
		clang-format -i $(SRCDIR)/*.c $(SRCDIR)/*.h; \
		echo "✅ Code formatted!"; \
	else \
		echo "⚠️ clang-format not found. Install with: sudo pacman -S clang"; \
	fi

# Analyse statique du code (si cppcheck est disponible)
analyze:
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "🔍 Running static analysis..."; \
		cppcheck --enable=all --std=c99 -I$(SRCDIR) $(SRCDIR)/*.c; \
		echo "✅ Analysis completed!"; \
	else \
		echo "⚠️ cppcheck not found. Install with: sudo pacman -S cppcheck"; \
	fi

# Compilation avec tous les avertissements activés
strict: CFLAGS += -Wpedantic -Wshadow -Wformat=2 -Wconversion
strict: clean $(TARGET)
	@echo "✅ Strict compilation completed!"

# === INFORMATIONS SUR L'ENVIRONNEMENT ===

info:
	@echo "Build Environment Information"
	@echo "============================="
	@echo "Compiler: $(CC) $$($(CC) --version | head -n1)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Libraries: $(LIBS)"
	@echo "Target: $(TARGET)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo ""
	@echo "System Info:"
	@echo "OS: $$(uname -s) $$(uname -r)"
	@echo "Architecture: $$(uname -m)"
	@echo "CPU Cores: $$(nproc)"

# Empêcher la suppression des fichiers intermédiaires
.PRECIOUS: $(OBJDIR)/%.o