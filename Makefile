# Makefile for Xbox Controller Driver
# Requires: libusb, CoreGraphics (macOS framework)

CC = gcc
OBJCC = clang
CFLAGS = -Wall -Wextra -O2 -I./include
OBJCFLAGS = -Wall -Wextra -O2 -I./include -fobjc-arc
LIBUSB_FLAGS = $(shell pkg-config --cflags --libs libusb-1.0)
FRAMEWORK_FLAGS = -framework CoreGraphics -framework ApplicationServices -framework CoreMIDI -framework CoreFoundation
COCOA_FLAGS = -framework Cocoa
LDFLAGS = -lm -lpthread

# Source files
SRC_LOG = src/log.c
SRC_CONFIG = src/config/config.c
SRC_EVENT = src/event/event.c
SRC_INPUT = src/input/input.c
SRC_INPUT_MIDI = src/input/input_midi.c
SRC_MIDI = src/midi/midi.c
SRC_INPUT_OSC = src/input/input_osc.c
SRC_OSC = src/osc/osc.c
SRC_USB = src/usb/usb.c
SRC_DRIVER = src/core/driver.c
SRC_MAIN_CLI = src/main.c
SRC_MAIN_GUI = src/main_gui.c
SRC_MENUBAR = src/gui/menubar.m
SRC_SETTINGS = src/gui/settings.m

# Object files
OBJ_LOG = build/log.o
OBJ_CONFIG = build/config.o
OBJ_EVENT = build/event.o
OBJ_INPUT = build/input.o
OBJ_INPUT_MIDI = build/input_midi.o
OBJ_MIDI = build/midi.o
OBJ_INPUT_OSC = build/input_osc.o
OBJ_OSC = build/osc.o
OBJ_USB = build/usb.o
OBJ_DRIVER = build/driver.o
OBJ_MAIN_CLI = build/main_cli.o
OBJ_MAIN_GUI = build/main_gui.o
OBJ_MENUBAR = build/menubar.o
OBJ_SETTINGS = build/settings.o

# All objects for CLI driver
OBJS_CLI = $(OBJ_LOG) $(OBJ_CONFIG) $(OBJ_EVENT) $(OBJ_INPUT) $(OBJ_INPUT_MIDI) $(OBJ_MIDI) $(OBJ_INPUT_OSC) $(OBJ_OSC) $(OBJ_USB) $(OBJ_DRIVER) $(OBJ_MAIN_CLI)

# All objects for GUI driver (default)
OBJS_GUI = $(OBJ_LOG) $(OBJ_CONFIG) $(OBJ_EVENT) $(OBJ_INPUT) $(OBJ_INPUT_MIDI) $(OBJ_MIDI) $(OBJ_INPUT_OSC) $(OBJ_OSC) $(OBJ_USB) $(OBJ_DRIVER) $(OBJ_MAIN_GUI) $(OBJ_MENUBAR) $(OBJ_SETTINGS)

# App bundle
APP_NAME = Xbox Controller Driver
APP_BUNDLE = $(APP_NAME).app
APP_ID = com.valerialaura.xbox-controller-driver

# Targets - GUI is the default
all: simulator simulator-cli

# Build directory
build:
	mkdir -p build

# Object file compilation
$(OBJ_LOG): $(SRC_LOG) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_CONFIG): $(SRC_CONFIG) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_EVENT): $(SRC_EVENT) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_INPUT): $(SRC_INPUT) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_INPUT_MIDI): $(SRC_INPUT_MIDI) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_MIDI): $(SRC_MIDI) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_INPUT_OSC): $(SRC_INPUT_OSC) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_OSC): $(SRC_OSC) | build
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_USB): $(SRC_USB) | build
	$(CC) $(CFLAGS) $(LIBUSB_FLAGS) -c $< -o $@

$(OBJ_DRIVER): $(SRC_DRIVER) | build
	$(CC) $(CFLAGS) $(LIBUSB_FLAGS) -c $< -o $@

$(OBJ_MAIN_CLI): $(SRC_MAIN_CLI) | build
	$(CC) $(CFLAGS) $(LIBUSB_FLAGS) -c $< -o $@

$(OBJ_MAIN_GUI): $(SRC_MAIN_GUI) | build
	$(CC) $(CFLAGS) $(LIBUSB_FLAGS) -c $< -o $@

$(OBJ_MENUBAR): $(SRC_MENUBAR) | build
	$(OBJCC) $(OBJCFLAGS) $(LIBUSB_FLAGS) $(COCOA_FLAGS) -c $< -o $@

$(OBJ_SETTINGS): $(SRC_SETTINGS) | build
	$(OBJCC) $(OBJCFLAGS) $(LIBUSB_FLAGS) $(COCOA_FLAGS) -c $< -o $@

# Main simulator with GUI (default)
simulator: $(OBJS_GUI)
	$(CC) $(CFLAGS) $(OBJS_GUI) $(LIBUSB_FLAGS) $(FRAMEWORK_FLAGS) $(COCOA_FLAGS) $(LDFLAGS) -o $@
	@echo ""
	@echo "Built simulator successfully!"
	@echo "   Run with: sudo ./simulator"
	@echo ""
	@echo "A menu bar icon will appear when running."
	@echo ""
	@echo "IMPORTANT: Grant Accessibility permissions:"
	@echo "   System Settings -> Privacy & Security -> Accessibility"
	@echo "   Add your terminal app to the allowed list"

# Double-clickable app bundle (menu-bar-only, no Dock icon, no terminal)
app: simulator
	@rm -rf "$(APP_BUNDLE)"
	@mkdir -p "$(APP_BUNDLE)/Contents/MacOS"
	@cp simulator "$(APP_BUNDLE)/Contents/MacOS/$(APP_NAME)"
	@printf '%s\n' \
	  '<?xml version="1.0" encoding="UTF-8"?>' \
	  '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' \
	  '<plist version="1.0">' \
	  '<dict>' \
	  '  <key>CFBundleExecutable</key><string>$(APP_NAME)</string>' \
	  '  <key>CFBundleIdentifier</key><string>$(APP_ID)</string>' \
	  '  <key>CFBundleName</key><string>$(APP_NAME)</string>' \
	  '  <key>CFBundlePackageType</key><string>APPL</string>' \
	  '  <key>CFBundleShortVersionString</key><string>2.1</string>' \
	  '  <key>CFBundleVersion</key><string>2.1</string>' \
	  '  <key>LSMinimumSystemVersion</key><string>12.0</string>' \
	  '  <key>LSUIElement</key><true/>' \
	  '  <key>NSHighResolutionCapable</key><true/>' \
	  '</dict>' \
	  '</plist>' > "$(APP_BUNDLE)/Contents/Info.plist"
	@codesign --force -s - "$(APP_BUNDLE)" 2>/dev/null || true
	@echo ""
	@echo "Built $(APP_BUNDLE)"
	@echo "   Double-click it in Finder (or: open \"$(APP_BUNDLE)\")"
	@echo "   It will ask for your password once to access the controller."

# CLI-only simulator (no menu bar)
simulator-cli: $(OBJS_CLI)
	$(CC) $(CFLAGS) $(OBJS_CLI) $(LIBUSB_FLAGS) $(FRAMEWORK_FLAGS) $(LDFLAGS) -o $@
	@echo ""
	@echo "Built simulator-cli successfully!"
	@echo "   Run with: sudo ./simulator-cli"
	@echo ""
	@echo "This is the CLI-only version (no menu bar)."

# Legacy targets (for compatibility) - all sources in legacy/
xbox_usb_test: legacy/phase2_usb_test.c
	$(CC) $(CFLAGS) $< $(LIBUSB_FLAGS) -o $@

xbox_gip_test: legacy/phase3_gip_test.c legacy/gip.h
	$(CC) $(CFLAGS) -I./legacy $< $(LIBUSB_FLAGS) -o $@

simulator-legacy: legacy/simulator.c legacy/gip.h legacy/keymapping.h
	$(CC) $(CFLAGS) -I./legacy $< $(LIBUSB_FLAGS) $(FRAMEWORK_FLAGS) -o $@ -lm
	@echo ""
	@echo "Built legacy simulator successfully!"
	@echo "   This is the original single-file version."

# Test targets
test: test-input test-config test-lut
	@echo ""
	@echo "All tests completed!"

test-input: tests/test_input.c include/types.h | build
	$(CC) $(CFLAGS) $< -o build/test_input -lm
	@echo ""
	@echo "Running input tests..."
	@./build/test_input

test-config: tests/test_config.c src/config/config.c include/types.h include/config.h src/log.c | build
	$(CC) $(CFLAGS) tests/test_config.c src/config/config.c src/log.c -o build/test_config -lm
	@echo ""
	@echo "Running config tests..."
	@./build/test_config

test-lut: tests/test_lut.c | build
	$(CC) $(CFLAGS) $< -o build/test_lut -lm
	@echo ""
	@echo "Running LUT tests..."
	@./build/test_lut

# Clean
clean:
	rm -rf build "$(APP_BUNDLE)"
	rm -f xbox_usb_test xbox_gip_test simulator simulator-cli simulator-legacy
	@echo "Cleaned up build artifacts"

# Install dependencies (homebrew)
deps:
	@echo "Installing dependencies..."
	brew install libusb pkg-config
	@echo "Dependencies installed"

# Install configuration
install-config:
	@mkdir -p ~/.config/xbox-controller
	@cp config/controller.json ~/.config/xbox-controller/config.json
	@echo "Configuration installed to ~/.config/xbox-controller/config.json"

# Help
help:
	@echo "Xbox Controller Driver - Build System"
	@echo "======================================"
	@echo ""
	@echo "Build Targets:"
	@echo "  make all            - Build all programs"
	@echo "  make simulator      - Build the simulator with menu bar GUI (default)"
	@echo "  make simulator-cli  - Build the CLI-only simulator (no menu bar)"
	@echo "  make simulator-legacy - Build the original single-file version"
	@echo ""
	@echo "Test Targets:"
	@echo "  make test           - Run all unit tests"
	@echo "  make test-input     - Run input processing tests"
	@echo "  make test-config    - Run configuration tests"
	@echo "  make test-lut       - Run lookup table tests"
	@echo ""
	@echo "Usage:"
	@echo "  sudo ./simulator          - Run with menu bar GUI"
	@echo "  sudo ./simulator-cli      - Run CLI-only version"
	@echo "  ./simulator --config path - Use custom config file"
	@echo "  ./simulator --help        - Show help"
	@echo ""
	@echo "Configuration:"
	@echo "  Edit config/controller.json to customize button bindings"
	@echo "  Or use --config flag to specify a custom config file"
	@echo "  Or place config at ~/.config/xbox-controller/config.json"
	@echo ""
	@echo "Other Targets:"
	@echo "  make clean          - Remove all built files"
	@echo "  make deps           - Install dependencies (libusb, pkg-config)"
	@echo "  make install-config - Install config to ~/.config/xbox-controller/"
	@echo ""
	@echo "Note: Requires accessibility permissions for keyboard/mouse input"

.PHONY: all app clean deps help test test-input test-config test-lut install-config
