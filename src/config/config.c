/*******************************************************************************
 * config.c - Runtime configuration implementation
 ******************************************************************************/

#include "../../include/config.h"
#include "../../include/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

/*******************************************************************************
 * Key Name to Keycode Mapping
 ******************************************************************************/
typedef struct {
    const char *name;
    uint16_t keycode;
} KeyMapping;

static const KeyMapping key_mappings[] = {
    // Letters
    {"a", 0x00}, {"b", 0x0B}, {"c", 0x08}, {"d", 0x02}, {"e", 0x0E},
    {"f", 0x03}, {"g", 0x05}, {"h", 0x04}, {"i", 0x22}, {"j", 0x26},
    {"k", 0x28}, {"l", 0x25}, {"m", 0x2E}, {"n", 0x2D}, {"o", 0x1F},
    {"p", 0x23}, {"q", 0x0C}, {"r", 0x0F}, {"s", 0x01}, {"t", 0x11},
    {"u", 0x20}, {"v", 0x09}, {"w", 0x0D}, {"x", 0x07}, {"y", 0x10},
    {"z", 0x06},

    // Numbers
    {"1", 0x12}, {"2", 0x13}, {"3", 0x14}, {"4", 0x15}, {"5", 0x17},
    {"6", 0x16}, {"7", 0x1A}, {"8", 0x1C}, {"9", 0x19}, {"0", 0x1D},

    // Special keys
    {"space", 0x31}, {"return", 0x24}, {"enter", 0x24}, {"tab", 0x30},
    {"escape", 0x35}, {"esc", 0x35}, {"delete", 0x33}, {"backspace", 0x33},
    {"forward_delete", 0x75},

    // Modifiers
    {"left_shift", 0x38}, {"right_shift", 0x3C}, {"shift", 0x38},
    {"left_control", 0x3B}, {"right_control", 0x3E}, {"control", 0x3B}, {"ctrl", 0x3B},
    {"left_option", 0x3A}, {"right_option", 0x3D}, {"option", 0x3A}, {"alt", 0x3A},
    {"left_command", 0x37}, {"right_command", 0x36}, {"command", 0x37}, {"cmd", 0x37},

    // Arrow keys
    {"up", 0x7E}, {"down", 0x7D}, {"left", 0x7B}, {"right", 0x7C},
    {"up_arrow", 0x7E}, {"down_arrow", 0x7D}, {"left_arrow", 0x7B}, {"right_arrow", 0x7C},

    // Function keys
    {"f1", 0x7A}, {"f2", 0x78}, {"f3", 0x63}, {"f4", 0x76},
    {"f5", 0x60}, {"f6", 0x61}, {"f7", 0x62}, {"f8", 0x64},
    {"f9", 0x65}, {"f10", 0x6D}, {"f11", 0x67}, {"f12", 0x6F},

    // Mouse buttons (special handling)
    {"mouse_left", 0xFFFE}, {"mouse_right", 0xFFFD}, {"mouse_middle", 0xFFFC},

    {NULL, 0}
};

/*******************************************************************************
 * OSC Default Addresses
 ******************************************************************************/
void config_build_osc_addresses(OscMapping *osc, const char *prefix) {
    snprintf(osc->addr_left_stick, OSC_ADDR_MAX, "%s/stick/left", prefix);
    snprintf(osc->addr_right_stick, OSC_ADDR_MAX, "%s/stick/right", prefix);
    snprintf(osc->addr_left_trigger, OSC_ADDR_MAX, "%s/trigger/left", prefix);
    snprintf(osc->addr_right_trigger, OSC_ADDR_MAX, "%s/trigger/right", prefix);
    snprintf(osc->addr_a, OSC_ADDR_MAX, "%s/button/a", prefix);
    snprintf(osc->addr_b, OSC_ADDR_MAX, "%s/button/b", prefix);
    snprintf(osc->addr_x, OSC_ADDR_MAX, "%s/button/x", prefix);
    snprintf(osc->addr_y, OSC_ADDR_MAX, "%s/button/y", prefix);
    snprintf(osc->addr_lb, OSC_ADDR_MAX, "%s/button/lb", prefix);
    snprintf(osc->addr_rb, OSC_ADDR_MAX, "%s/button/rb", prefix);
    snprintf(osc->addr_ls, OSC_ADDR_MAX, "%s/button/ls", prefix);
    snprintf(osc->addr_rs, OSC_ADDR_MAX, "%s/button/rs", prefix);
    snprintf(osc->addr_view, OSC_ADDR_MAX, "%s/button/view", prefix);
    snprintf(osc->addr_menu, OSC_ADDR_MAX, "%s/button/menu", prefix);
    snprintf(osc->addr_dpad_up, OSC_ADDR_MAX, "%s/dpad/up", prefix);
    snprintf(osc->addr_dpad_down, OSC_ADDR_MAX, "%s/dpad/down", prefix);
    snprintf(osc->addr_dpad_left, OSC_ADDR_MAX, "%s/dpad/left", prefix);
    snprintf(osc->addr_dpad_right, OSC_ADDR_MAX, "%s/dpad/right", prefix);
}

/*******************************************************************************
 * Default Configuration
 ******************************************************************************/
void config_get_defaults(ControllerMapping *mapping) {
    memset(mapping, 0, sizeof(ControllerMapping));

    // Output mode
    mapping->output_mode = OUTPUT_MODE_KEYBOARD;

    // MIDI mappings (used when output_mode is "midi")
    strncpy(mapping->midi.device_name, "Xbox Controller", MIDI_DEVICE_NAME_MAX - 1);
    mapping->midi.channel = 0;          // channel 1
    mapping->midi.velocity = 127;
    mapping->midi.note_a = 36;
    mapping->midi.note_b = 37;
    mapping->midi.note_x = 38;
    mapping->midi.note_y = 39;
    mapping->midi.note_lb = 40;
    mapping->midi.note_rb = 41;
    mapping->midi.note_view = 42;
    mapping->midi.note_menu = 43;
    mapping->midi.note_ls = 44;
    mapping->midi.note_rs = 45;
    mapping->midi.note_dpad_up = 46;
    mapping->midi.note_dpad_down = 47;
    mapping->midi.note_dpad_left = 48;
    mapping->midi.note_dpad_right = 49;
    mapping->midi.cc_left_x = 20;
    mapping->midi.cc_left_y = 21;
    mapping->midi.cc_right_x = 22;
    mapping->midi.cc_right_y = 23;
    mapping->midi.cc_left_trigger = 24;
    mapping->midi.cc_right_trigger = 25;
    mapping->midi.invert_y = false;

    // OSC settings (used when output_mode is "osc" or "midi+osc")
    strncpy(mapping->osc.host, "127.0.0.1", OSC_HOST_MAX - 1);
    mapping->osc.port = 9000;
    config_build_osc_addresses(&mapping->osc, "/xbox");

    // Button mappings
    mapping->buttons.key_a = 0x31;  // Space
    mapping->buttons.key_b = 0x08;  // C
    mapping->buttons.key_x = 0x0F;  // R
    mapping->buttons.key_y = 0x03;  // F
    mapping->buttons.key_lb = 0x0C; // Q
    mapping->buttons.key_rb = 0x0E; // E
    mapping->buttons.key_ls = 0x38; // Left Shift
    mapping->buttons.key_rs = 0x3B; // Left Control
    mapping->buttons.key_view = 0x30;   // Tab
    mapping->buttons.key_menu = 0x35;   // Escape
    mapping->buttons.key_dpad_up = 0x7E;    // Up Arrow
    mapping->buttons.key_dpad_down = 0x7D;  // Down Arrow
    mapping->buttons.key_dpad_left = 0x7B;  // Left Arrow
    mapping->buttons.key_dpad_right = 0x7C; // Right Arrow

    // Stick settings
    mapping->sticks.left_stick_mode = STICK_MODE_WASD;
    mapping->sticks.left_up = 0x0D;     // W
    mapping->sticks.left_down = 0x01;   // S
    mapping->sticks.left_left = 0x00;   // A
    mapping->sticks.left_right = 0x02;  // D

    mapping->sticks.right_stick_mode = STICK_MODE_MOUSE;
    mapping->sticks.right_up = 0x22;    // I
    mapping->sticks.right_down = 0x28;  // K
    mapping->sticks.right_left = 0x26;  // J
    mapping->sticks.right_right = 0x25; // L

    mapping->sticks.mouse_sensitivity = DEFAULT_SENSITIVITY;
    mapping->sticks.mouse_curve = DEFAULT_CURVE;
    mapping->sticks.mouse_smoothing = DEFAULT_SMOOTHING;
    mapping->sticks.deadzone = DEFAULT_DEADZONE;

    // Trigger settings
    mapping->triggers.left_trigger_mode = TRIGGER_MODE_MOUSE;
    mapping->triggers.right_trigger_mode = TRIGGER_MODE_MOUSE;
    mapping->triggers.left_trigger_key = 0x06;  // Z
    mapping->triggers.right_trigger_key = 0x07; // X
    mapping->triggers.threshold = DEFAULT_THRESHOLD;

    // Feature settings
    mapping->features.rumble.enabled = true;
    mapping->features.rumble.button_feedback = true;
    mapping->features.rumble.intensity = 100;
    mapping->features.rumble.duration_ms = RUMBLE_DURATION_MS;

    mapping->features.turbo.enabled = true;
    mapping->features.turbo.rate = DEFAULT_TURBO_RATE;

    mapping->features.analog_keyboard.enabled = false;

    // General settings
    mapping->console_output_enabled = true;
    mapping->streaming_mode = false;
    mapping->log_level = LOG_LEVEL_INFO;
}

/*******************************************************************************
 * Key Parsing
 ******************************************************************************/
uint16_t config_parse_key(const char *key_name) {
    if (key_name == NULL) return 0xFFFF;

    // Convert to lowercase for comparison
    char lower[64];
    size_t len = strlen(key_name);
    if (len >= sizeof(lower)) len = sizeof(lower) - 1;

    for (size_t i = 0; i < len; i++) {
        char c = key_name[i];
        if (c >= 'A' && c <= 'Z') {
            lower[i] = c + ('a' - 'A');
        } else {
            lower[i] = c;
        }
    }
    lower[len] = '\0';

    // Search in mappings
    for (const KeyMapping *km = key_mappings; km->name != NULL; km++) {
        if (strcmp(lower, km->name) == 0) {
            return km->keycode;
        }
    }

    // Try parsing as hex (0x##)
    if (len > 2 && lower[0] == '0' && lower[1] == 'x') {
        return (uint16_t)strtol(lower, NULL, 16);
    }

    return 0xFFFF;
}

const char* config_key_name(uint16_t keycode) {
    for (const KeyMapping *km = key_mappings; km->name != NULL; km++) {
        if (km->keycode == keycode) {
            return km->name;
        }
    }
    return "unknown";
}

/*******************************************************************************
 * Simple JSON Parser (minimal, no external dependencies)
 ******************************************************************************/
static char* read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    size_t read_size = fread(buffer, 1, size, f);
    buffer[read_size] = '\0';
    fclose(f);

    return buffer;
}

static const char* skip_whitespace(const char *p) {
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    return p;
}

static const char* parse_string(const char *p, char *out, size_t out_size) {
    p = skip_whitespace(p);
    if (*p != '"') return NULL;
    p++;

    size_t i = 0;
    while (*p && *p != '"' && i < out_size - 1) {
        if (*p == '\\' && *(p+1)) {
            p++;
        }
        out[i++] = *p++;
    }
    out[i] = '\0';

    if (*p == '"') p++;
    return p;
}

static const char* parse_number(const char *p, double *out) {
    p = skip_whitespace(p);
    char *end;
    *out = strtod(p, &end);
    return end;
}

static const char* parse_bool(const char *p, bool *out) {
    p = skip_whitespace(p);
    if (strncmp(p, "true", 4) == 0) {
        *out = true;
        return p + 4;
    } else if (strncmp(p, "false", 5) == 0) {
        *out = false;
        return p + 5;
    }
    return NULL;
}

static const char* find_key(const char *json, const char *key) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);

    // Skip matches that are values rather than keys (not followed by ':'),
    // e.g. the string "midi" in "output_mode": "midi"
    const char *p = json;
    while ((p = strstr(p, search)) != NULL) {
        const char *after = skip_whitespace(p + strlen(search));
        if (*after == ':') {
            return skip_whitespace(after + 1);
        }
        p += strlen(search);
    }
    return NULL;
}

static void parse_midi_num(const char *section, const char *key, uint8_t *out) {
    const char *p = find_key(section, key);
    if (p) {
        double num_val;
        parse_number(p, &num_val);
        if (num_val >= 0 && num_val <= 127) *out = (uint8_t)num_val;
    }
}

static void parse_osc_addr(const char *section, const char *key, char *out) {
    const char *p = find_key(section, key);
    if (p) {
        char val[OSC_ADDR_MAX] = {0};
        parse_string(p, val, sizeof(val));
        if (val[0] == '/') {
            strncpy(out, val, OSC_ADDR_MAX - 1);
            out[OSC_ADDR_MAX - 1] = '\0';
        }
    }
}

/*******************************************************************************
 * Configuration Loading
 ******************************************************************************/
int config_load(const char *path, ControllerMapping *mapping) {
    // Start with defaults
    config_get_defaults(mapping);

    char *json = read_file(path);
    if (!json) {
        LOG_DEBUG("Could not open config file: %s", path);
        return -1;
    }

    LOG_INFO("Loading configuration from: %s", path);

    const char *p;
    char str_val[64];
    double num_val;
    bool bool_val;

    // Parse buttons
    const char *buttons = find_key(json, "buttons");
    if (buttons) {
        if ((p = find_key(buttons, "a"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_a = key;
        }
        if ((p = find_key(buttons, "b"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_b = key;
        }
        if ((p = find_key(buttons, "x"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_x = key;
        }
        if ((p = find_key(buttons, "y"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_y = key;
        }
        if ((p = find_key(buttons, "lb"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_lb = key;
        }
        if ((p = find_key(buttons, "rb"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_rb = key;
        }
        if ((p = find_key(buttons, "ls"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_ls = key;
        }
        if ((p = find_key(buttons, "rs"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_rs = key;
        }
        if ((p = find_key(buttons, "view"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_view = key;
        }
        if ((p = find_key(buttons, "menu"))) {
            parse_string(p, str_val, sizeof(str_val));
            uint16_t key = config_parse_key(str_val);
            if (key != 0xFFFF) mapping->buttons.key_menu = key;
        }
    }

    // Parse left_stick
    const char *left_stick = find_key(json, "left_stick");
    if (left_stick) {
        if ((p = find_key(left_stick, "mode"))) {
            parse_string(p, str_val, sizeof(str_val));
            if (strcmp(str_val, "wasd") == 0) mapping->sticks.left_stick_mode = STICK_MODE_WASD;
            else if (strcmp(str_val, "arrows") == 0) mapping->sticks.left_stick_mode = STICK_MODE_ARROWS;
            else if (strcmp(str_val, "mouse") == 0) mapping->sticks.left_stick_mode = STICK_MODE_MOUSE;
            else if (strcmp(str_val, "disabled") == 0) mapping->sticks.left_stick_mode = STICK_MODE_DISABLED;
        }
        if ((p = find_key(left_stick, "deadzone"))) {
            parse_number(p, &num_val);
            mapping->sticks.deadzone = (int16_t)num_val;
        }
    }

    // Parse right_stick
    const char *right_stick = find_key(json, "right_stick");
    if (right_stick) {
        if ((p = find_key(right_stick, "mode"))) {
            parse_string(p, str_val, sizeof(str_val));
            if (strcmp(str_val, "wasd") == 0) mapping->sticks.right_stick_mode = STICK_MODE_WASD;
            else if (strcmp(str_val, "arrows") == 0) mapping->sticks.right_stick_mode = STICK_MODE_ARROWS;
            else if (strcmp(str_val, "mouse") == 0) mapping->sticks.right_stick_mode = STICK_MODE_MOUSE;
            else if (strcmp(str_val, "disabled") == 0) mapping->sticks.right_stick_mode = STICK_MODE_DISABLED;
        }
        if ((p = find_key(right_stick, "sensitivity"))) {
            parse_number(p, &num_val);
            mapping->sticks.mouse_sensitivity = (float)num_val;
        }
        if ((p = find_key(right_stick, "curve"))) {
            parse_number(p, &num_val);
            mapping->sticks.mouse_curve = (float)num_val;
        }
        if ((p = find_key(right_stick, "smoothing"))) {
            parse_number(p, &num_val);
            mapping->sticks.mouse_smoothing = (float)num_val;
        }
    }

    // Parse triggers
    const char *triggers = find_key(json, "triggers");
    if (triggers) {
        if ((p = find_key(triggers, "left"))) {
            parse_string(p, str_val, sizeof(str_val));
            if (strcmp(str_val, "mouse_left") == 0) {
                mapping->triggers.left_trigger_mode = TRIGGER_MODE_MOUSE;
            } else if (strcmp(str_val, "disabled") == 0) {
                mapping->triggers.left_trigger_mode = TRIGGER_MODE_DISABLED;
            } else {
                uint16_t key = config_parse_key(str_val);
                if (key != 0xFFFF) {
                    mapping->triggers.left_trigger_mode = TRIGGER_MODE_KEY;
                    mapping->triggers.left_trigger_key = key;
                }
            }
        }
        if ((p = find_key(triggers, "right"))) {
            parse_string(p, str_val, sizeof(str_val));
            if (strcmp(str_val, "mouse_right") == 0) {
                mapping->triggers.right_trigger_mode = TRIGGER_MODE_MOUSE;
            } else if (strcmp(str_val, "disabled") == 0) {
                mapping->triggers.right_trigger_mode = TRIGGER_MODE_DISABLED;
            } else {
                uint16_t key = config_parse_key(str_val);
                if (key != 0xFFFF) {
                    mapping->triggers.right_trigger_mode = TRIGGER_MODE_KEY;
                    mapping->triggers.right_trigger_key = key;
                }
            }
        }
        if ((p = find_key(triggers, "threshold"))) {
            parse_number(p, &num_val);
            mapping->triggers.threshold = (uint8_t)num_val;
        }
    }

    // Parse features
    const char *features = find_key(json, "features");
    if (features) {
        const char *rumble = find_key(features, "rumble");
        if (rumble) {
            if ((p = find_key(rumble, "enabled"))) {
                if (parse_bool(p, &bool_val)) mapping->features.rumble.enabled = bool_val;
            }
            if ((p = find_key(rumble, "button_feedback"))) {
                if (parse_bool(p, &bool_val)) mapping->features.rumble.button_feedback = bool_val;
            }
        }
        const char *turbo = find_key(features, "turbo");
        if (turbo) {
            if ((p = find_key(turbo, "enabled"))) {
                if (parse_bool(p, &bool_val)) mapping->features.turbo.enabled = bool_val;
            }
            if ((p = find_key(turbo, "rate"))) {
                parse_number(p, &num_val);
                mapping->features.turbo.rate = (uint8_t)num_val;
            }
        }
        const char *analog = find_key(features, "analog_keyboard");
        if (analog) {
            if ((p = find_key(analog, "enabled"))) {
                if (parse_bool(p, &bool_val)) mapping->features.analog_keyboard.enabled = bool_val;
            }
        }
    }

    // Parse advanced
    const char *advanced = find_key(json, "advanced");
    if (advanced) {
        if ((p = find_key(advanced, "log_level"))) {
            parse_string(p, str_val, sizeof(str_val));
            if (strcmp(str_val, "debug") == 0) mapping->log_level = LOG_LEVEL_DEBUG;
            else if (strcmp(str_val, "info") == 0) mapping->log_level = LOG_LEVEL_INFO;
            else if (strcmp(str_val, "warn") == 0) mapping->log_level = LOG_LEVEL_WARN;
            else if (strcmp(str_val, "error") == 0) mapping->log_level = LOG_LEVEL_ERROR;
            else if (strcmp(str_val, "none") == 0) mapping->log_level = LOG_LEVEL_NONE;
        }
        if ((p = find_key(advanced, "streaming_mode"))) {
            if (parse_bool(p, &bool_val)) mapping->streaming_mode = bool_val;
        }
        if ((p = find_key(advanced, "console_output"))) {
            if (parse_bool(p, &bool_val)) mapping->console_output_enabled = bool_val;
        }
    }

    // Parse output_mode
    if ((p = find_key(json, "output_mode"))) {
        parse_string(p, str_val, sizeof(str_val));
        if (strcmp(str_val, "midi") == 0) mapping->output_mode = OUTPUT_MODE_MIDI;
        else if (strcmp(str_val, "osc") == 0) mapping->output_mode = OUTPUT_MODE_OSC;
        else if (strcmp(str_val, "midi+osc") == 0 || strcmp(str_val, "osc+midi") == 0 ||
                 strcmp(str_val, "midi_osc") == 0) mapping->output_mode = OUTPUT_MODE_MIDI_OSC;
        else if (strcmp(str_val, "keyboard") == 0) mapping->output_mode = OUTPUT_MODE_KEYBOARD;
    }

    // Parse midi
    const char *midi = find_key(json, "midi");
    if (midi) {
        if ((p = find_key(midi, "device_name"))) {
            parse_string(p, str_val, sizeof(str_val));
            if (str_val[0] != '\0') {
                strncpy(mapping->midi.device_name, str_val, MIDI_DEVICE_NAME_MAX - 1);
                mapping->midi.device_name[MIDI_DEVICE_NAME_MAX - 1] = '\0';
            }
        }
        if ((p = find_key(midi, "channel"))) {
            parse_number(p, &num_val);
            if (num_val >= 1 && num_val <= 16) mapping->midi.channel = (uint8_t)(num_val - 1);
        }
        if ((p = find_key(midi, "velocity"))) {
            parse_number(p, &num_val);
            if (num_val >= 1 && num_val <= 127) mapping->midi.velocity = (uint8_t)num_val;
        }
        if ((p = find_key(midi, "invert_y"))) {
            if (parse_bool(p, &bool_val)) mapping->midi.invert_y = bool_val;
        }

        const char *notes = find_key(midi, "notes");
        if (notes) {
            parse_midi_num(notes, "a", &mapping->midi.note_a);
            parse_midi_num(notes, "b", &mapping->midi.note_b);
            parse_midi_num(notes, "x", &mapping->midi.note_x);
            parse_midi_num(notes, "y", &mapping->midi.note_y);
            parse_midi_num(notes, "lb", &mapping->midi.note_lb);
            parse_midi_num(notes, "rb", &mapping->midi.note_rb);
            parse_midi_num(notes, "ls", &mapping->midi.note_ls);
            parse_midi_num(notes, "rs", &mapping->midi.note_rs);
            parse_midi_num(notes, "view", &mapping->midi.note_view);
            parse_midi_num(notes, "menu", &mapping->midi.note_menu);
            parse_midi_num(notes, "dpad_up", &mapping->midi.note_dpad_up);
            parse_midi_num(notes, "dpad_down", &mapping->midi.note_dpad_down);
            parse_midi_num(notes, "dpad_left", &mapping->midi.note_dpad_left);
            parse_midi_num(notes, "dpad_right", &mapping->midi.note_dpad_right);
        }

        const char *ccs = find_key(midi, "ccs");
        if (ccs) {
            parse_midi_num(ccs, "left_stick_x", &mapping->midi.cc_left_x);
            parse_midi_num(ccs, "left_stick_y", &mapping->midi.cc_left_y);
            parse_midi_num(ccs, "right_stick_x", &mapping->midi.cc_right_x);
            parse_midi_num(ccs, "right_stick_y", &mapping->midi.cc_right_y);
            parse_midi_num(ccs, "left_trigger", &mapping->midi.cc_left_trigger);
            parse_midi_num(ccs, "right_trigger", &mapping->midi.cc_right_trigger);
        }
    }

    // Parse osc
    const char *osc = find_key(json, "osc");
    if (osc) {
        if ((p = find_key(osc, "host"))) {
            parse_string(p, str_val, sizeof(str_val));
            if (str_val[0] != '\0') {
                strncpy(mapping->osc.host, str_val, OSC_HOST_MAX - 1);
                mapping->osc.host[OSC_HOST_MAX - 1] = '\0';
            }
        }
        if ((p = find_key(osc, "port"))) {
            parse_number(p, &num_val);
            if (num_val >= 1 && num_val <= 65535) mapping->osc.port = (uint16_t)num_val;
        }
        // Prefix rebuilds every default address; explicit addresses override after
        if ((p = find_key(osc, "prefix"))) {
            char prefix[OSC_PREFIX_MAX] = {0};
            parse_string(p, prefix, sizeof(prefix));
            if (prefix[0] == '/') {
                config_build_osc_addresses(&mapping->osc, prefix);
            }
        }

        const char *addresses = find_key(osc, "addresses");
        if (addresses) {
            parse_osc_addr(addresses, "left_stick", mapping->osc.addr_left_stick);
            parse_osc_addr(addresses, "right_stick", mapping->osc.addr_right_stick);
            parse_osc_addr(addresses, "left_trigger", mapping->osc.addr_left_trigger);
            parse_osc_addr(addresses, "right_trigger", mapping->osc.addr_right_trigger);
            parse_osc_addr(addresses, "a", mapping->osc.addr_a);
            parse_osc_addr(addresses, "b", mapping->osc.addr_b);
            parse_osc_addr(addresses, "x", mapping->osc.addr_x);
            parse_osc_addr(addresses, "y", mapping->osc.addr_y);
            parse_osc_addr(addresses, "lb", mapping->osc.addr_lb);
            parse_osc_addr(addresses, "rb", mapping->osc.addr_rb);
            parse_osc_addr(addresses, "ls", mapping->osc.addr_ls);
            parse_osc_addr(addresses, "rs", mapping->osc.addr_rs);
            parse_osc_addr(addresses, "view", mapping->osc.addr_view);
            parse_osc_addr(addresses, "menu", mapping->osc.addr_menu);
            parse_osc_addr(addresses, "dpad_up", mapping->osc.addr_dpad_up);
            parse_osc_addr(addresses, "dpad_down", mapping->osc.addr_dpad_down);
            parse_osc_addr(addresses, "dpad_left", mapping->osc.addr_dpad_left);
            parse_osc_addr(addresses, "dpad_right", mapping->osc.addr_dpad_right);
        }
    }

    free(json);
    return 0;
}

int config_load_auto(const char *cli_path, ControllerMapping *mapping, char *loaded_path, size_t path_size) {
    // Priority 1: CLI argument
    if (cli_path && strlen(cli_path) > 0) {
        if (config_load(cli_path, mapping) == 0) {
            if (loaded_path) strncpy(loaded_path, cli_path, path_size);
            return 0;
        }
    }

    // Priority 2: Local config
    if (access(CONFIG_PATH_LOCAL, R_OK) == 0) {
        if (config_load(CONFIG_PATH_LOCAL, mapping) == 0) {
            if (loaded_path) strncpy(loaded_path, CONFIG_PATH_LOCAL, path_size);
            return 0;
        }
    }

    // Priority 3: User config
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }

    if (home) {
        char user_config[CONFIG_PATH_MAX];
        snprintf(user_config, sizeof(user_config), "%s%s", home, CONFIG_PATH_USER);

        if (access(user_config, R_OK) == 0) {
            if (config_load(user_config, mapping) == 0) {
                if (loaded_path) strncpy(loaded_path, user_config, path_size);
                return 0;
            }
        }
    }

    // Priority 4: Defaults
    config_get_defaults(mapping);
    if (loaded_path) strncpy(loaded_path, "(defaults)", path_size);
    return 0;
}

int config_reload_if_changed(const char *path, ControllerMapping *mapping, time_t *last_modified) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return -1;
    }

    if (st.st_mtime > *last_modified) {
        *last_modified = st.st_mtime;
        if (config_load(path, mapping) == 0) {
            LOG_INFO("Configuration reloaded");
            return 1;
        }
        return -1;
    }

    return 0;
}

/*******************************************************************************
 * Configuration Saving
 ******************************************************************************/
static const char* key_or_hex(uint16_t keycode, char *buf, size_t size) {
    const char *name = config_key_name(keycode);
    if (strcmp(name, "unknown") != 0) return name;
    snprintf(buf, size, "0x%02X", keycode);
    return buf;
}

static const char* stick_mode_str(StickMode mode) {
    switch (mode) {
        case STICK_MODE_WASD: return "wasd";
        case STICK_MODE_ARROWS: return "arrows";
        case STICK_MODE_MOUSE: return "mouse";
        default: return "disabled";
    }
}

static const char* trigger_str(TriggerMode mode, uint16_t key, bool is_left,
                               char *buf, size_t size) {
    switch (mode) {
        case TRIGGER_MODE_MOUSE: return is_left ? "mouse_left" : "mouse_right";
        case TRIGGER_MODE_KEY: return key_or_hex(key, buf, size);
        default: return "disabled";
    }
}

static const char* output_mode_str(OutputMode mode) {
    switch (mode) {
        case OUTPUT_MODE_MIDI: return "midi";
        case OUTPUT_MODE_OSC: return "osc";
        case OUTPUT_MODE_MIDI_OSC: return "midi+osc";
        default: return "keyboard";
    }
}

static const char* log_level_str(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "debug";
        case LOG_LEVEL_WARN: return "warn";
        case LOG_LEVEL_ERROR: return "error";
        case LOG_LEVEL_NONE: return "none";
        default: return "info";
    }
}

int config_save(const char *path, const ControllerMapping *m) {
    // Write to a temp file, then rename, so hot-reload never sees a partial file
    char tmp_path[CONFIG_PATH_MAX + 8];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);

    FILE *f = fopen(tmp_path, "w");
    if (!f) {
        LOG_ERROR("Could not write config file: %s", tmp_path);
        return -1;
    }

    char kb[8][16];

    fprintf(f, "{\n");
    fprintf(f, "  \"output_mode\": \"%s\",\n\n", output_mode_str(m->output_mode));

    fprintf(f, "  \"buttons\": {\n");
    fprintf(f, "    \"a\": \"%s\",\n", key_or_hex(m->buttons.key_a, kb[0], 16));
    fprintf(f, "    \"b\": \"%s\",\n", key_or_hex(m->buttons.key_b, kb[0], 16));
    fprintf(f, "    \"x\": \"%s\",\n", key_or_hex(m->buttons.key_x, kb[0], 16));
    fprintf(f, "    \"y\": \"%s\",\n", key_or_hex(m->buttons.key_y, kb[0], 16));
    fprintf(f, "    \"lb\": \"%s\",\n", key_or_hex(m->buttons.key_lb, kb[0], 16));
    fprintf(f, "    \"rb\": \"%s\",\n", key_or_hex(m->buttons.key_rb, kb[0], 16));
    fprintf(f, "    \"ls\": \"%s\",\n", key_or_hex(m->buttons.key_ls, kb[0], 16));
    fprintf(f, "    \"rs\": \"%s\",\n", key_or_hex(m->buttons.key_rs, kb[0], 16));
    fprintf(f, "    \"view\": \"%s\",\n", key_or_hex(m->buttons.key_view, kb[0], 16));
    fprintf(f, "    \"menu\": \"%s\"\n", key_or_hex(m->buttons.key_menu, kb[0], 16));
    fprintf(f, "  },\n\n");

    fprintf(f, "  \"left_stick\": {\n");
    fprintf(f, "    \"mode\": \"%s\",\n", stick_mode_str(m->sticks.left_stick_mode));
    fprintf(f, "    \"deadzone\": %d\n", m->sticks.deadzone);
    fprintf(f, "  },\n\n");

    fprintf(f, "  \"right_stick\": {\n");
    fprintf(f, "    \"mode\": \"%s\",\n", stick_mode_str(m->sticks.right_stick_mode));
    fprintf(f, "    \"sensitivity\": %.2f,\n", m->sticks.mouse_sensitivity);
    fprintf(f, "    \"curve\": %.2f,\n", m->sticks.mouse_curve);
    fprintf(f, "    \"smoothing\": %.2f\n", m->sticks.mouse_smoothing);
    fprintf(f, "  },\n\n");

    fprintf(f, "  \"triggers\": {\n");
    fprintf(f, "    \"left\": \"%s\",\n",
            trigger_str(m->triggers.left_trigger_mode, m->triggers.left_trigger_key, true, kb[0], 16));
    fprintf(f, "    \"right\": \"%s\",\n",
            trigger_str(m->triggers.right_trigger_mode, m->triggers.right_trigger_key, false, kb[0], 16));
    fprintf(f, "    \"threshold\": %d\n", m->triggers.threshold);
    fprintf(f, "  },\n\n");

    fprintf(f, "  \"midi\": {\n");
    fprintf(f, "    \"device_name\": \"%s\",\n", m->midi.device_name);
    fprintf(f, "    \"channel\": %d,\n", m->midi.channel + 1);
    fprintf(f, "    \"velocity\": %d,\n", m->midi.velocity);
    fprintf(f, "    \"invert_y\": %s,\n", m->midi.invert_y ? "true" : "false");
    fprintf(f, "    \"notes\": {\n");
    fprintf(f, "      \"a\": %d, \"b\": %d, \"x\": %d, \"y\": %d,\n",
            m->midi.note_a, m->midi.note_b, m->midi.note_x, m->midi.note_y);
    fprintf(f, "      \"lb\": %d, \"rb\": %d, \"view\": %d, \"menu\": %d,\n",
            m->midi.note_lb, m->midi.note_rb, m->midi.note_view, m->midi.note_menu);
    fprintf(f, "      \"ls\": %d, \"rs\": %d,\n", m->midi.note_ls, m->midi.note_rs);
    fprintf(f, "      \"dpad_up\": %d, \"dpad_down\": %d, \"dpad_left\": %d, \"dpad_right\": %d\n",
            m->midi.note_dpad_up, m->midi.note_dpad_down,
            m->midi.note_dpad_left, m->midi.note_dpad_right);
    fprintf(f, "    },\n");
    fprintf(f, "    \"ccs\": {\n");
    fprintf(f, "      \"left_stick_x\": %d, \"left_stick_y\": %d,\n",
            m->midi.cc_left_x, m->midi.cc_left_y);
    fprintf(f, "      \"right_stick_x\": %d, \"right_stick_y\": %d,\n",
            m->midi.cc_right_x, m->midi.cc_right_y);
    fprintf(f, "      \"left_trigger\": %d, \"right_trigger\": %d\n",
            m->midi.cc_left_trigger, m->midi.cc_right_trigger);
    fprintf(f, "    }\n");
    fprintf(f, "  },\n\n");

    fprintf(f, "  \"osc\": {\n");
    fprintf(f, "    \"host\": \"%s\",\n", m->osc.host);
    fprintf(f, "    \"port\": %u,\n", m->osc.port);
    fprintf(f, "    \"addresses\": {\n");
    fprintf(f, "      \"left_stick\": \"%s\",\n", m->osc.addr_left_stick);
    fprintf(f, "      \"right_stick\": \"%s\",\n", m->osc.addr_right_stick);
    fprintf(f, "      \"left_trigger\": \"%s\",\n", m->osc.addr_left_trigger);
    fprintf(f, "      \"right_trigger\": \"%s\",\n", m->osc.addr_right_trigger);
    fprintf(f, "      \"a\": \"%s\", \"b\": \"%s\", \"x\": \"%s\", \"y\": \"%s\",\n",
            m->osc.addr_a, m->osc.addr_b, m->osc.addr_x, m->osc.addr_y);
    fprintf(f, "      \"lb\": \"%s\", \"rb\": \"%s\", \"ls\": \"%s\", \"rs\": \"%s\",\n",
            m->osc.addr_lb, m->osc.addr_rb, m->osc.addr_ls, m->osc.addr_rs);
    fprintf(f, "      \"view\": \"%s\", \"menu\": \"%s\",\n", m->osc.addr_view, m->osc.addr_menu);
    fprintf(f, "      \"dpad_up\": \"%s\", \"dpad_down\": \"%s\",\n",
            m->osc.addr_dpad_up, m->osc.addr_dpad_down);
    fprintf(f, "      \"dpad_left\": \"%s\", \"dpad_right\": \"%s\"\n",
            m->osc.addr_dpad_left, m->osc.addr_dpad_right);
    fprintf(f, "    }\n");
    fprintf(f, "  },\n\n");

    fprintf(f, "  \"features\": {\n");
    fprintf(f, "    \"rumble\": {\n");
    fprintf(f, "      \"enabled\": %s,\n", m->features.rumble.enabled ? "true" : "false");
    fprintf(f, "      \"button_feedback\": %s\n", m->features.rumble.button_feedback ? "true" : "false");
    fprintf(f, "    },\n");
    fprintf(f, "    \"turbo\": {\n");
    fprintf(f, "      \"enabled\": %s,\n", m->features.turbo.enabled ? "true" : "false");
    fprintf(f, "      \"rate\": %d\n", m->features.turbo.rate);
    fprintf(f, "    },\n");
    fprintf(f, "    \"analog_keyboard\": {\n");
    fprintf(f, "      \"enabled\": %s\n", m->features.analog_keyboard.enabled ? "true" : "false");
    fprintf(f, "    }\n");
    fprintf(f, "  },\n\n");

    fprintf(f, "  \"advanced\": {\n");
    fprintf(f, "    \"log_level\": \"%s\",\n", log_level_str(m->log_level));
    fprintf(f, "    \"streaming_mode\": %s,\n", m->streaming_mode ? "true" : "false");
    fprintf(f, "    \"console_output\": %s\n", m->console_output_enabled ? "true" : "false");
    fprintf(f, "  }\n");
    fprintf(f, "}\n");

    fclose(f);

    if (rename(tmp_path, path) != 0) {
        LOG_ERROR("Could not move config into place: %s", path);
        unlink(tmp_path);
        return -1;
    }

    // When running elevated (admin relaunch), keep the file owned by the user
    // whose directory it lives in, so unprivileged edits still work later
    if (geteuid() == 0) {
        char dir[CONFIG_PATH_MAX];
        strncpy(dir, path, sizeof(dir) - 1);
        dir[sizeof(dir) - 1] = '\0';
        char *slash = strrchr(dir, '/');
        if (slash && slash != dir) {
            *slash = '\0';
            struct stat st;
            if (stat(dir, &st) == 0 && st.st_uid != 0) {
                chown(path, st.st_uid, st.st_gid);
            }
        }
    }

    LOG_INFO("Configuration saved to: %s", path);
    return 0;
}

/*******************************************************************************
 * Configuration Printing
 ******************************************************************************/
void config_print(const ControllerMapping *mapping) {
    const char *stick_mode_names[] = {"WASD", "Arrows", "Mouse", "Disabled"};
    const char *trigger_mode_names[] = {"Mouse", "Key", "Disabled"};

    const char *output_mode_names[] = {"Keyboard/Mouse", "MIDI", "OSC", "MIDI+OSC"};

    printf("Configuration:\n");
    printf("  Output mode: %s\n", output_mode_names[mapping->output_mode]);
    if (OUTPUT_HAS_MIDI(mapping->output_mode)) {
        printf("  MIDI device: %s (channel %d)\n", mapping->midi.device_name,
               mapping->midi.channel + 1);
        printf("  MIDI CCs: LX=%d LY=%d RX=%d RY=%d LT=%d RT=%d\n",
               mapping->midi.cc_left_x, mapping->midi.cc_left_y,
               mapping->midi.cc_right_x, mapping->midi.cc_right_y,
               mapping->midi.cc_left_trigger, mapping->midi.cc_right_trigger);
        printf("  MIDI notes: A=%d B=%d X=%d Y=%d (buttons ascending from A)\n",
               mapping->midi.note_a, mapping->midi.note_b,
               mapping->midi.note_x, mapping->midi.note_y);
    }
    if (OUTPUT_HAS_OSC(mapping->output_mode)) {
        printf("  OSC target: %s:%u\n", mapping->osc.host, mapping->osc.port);
        printf("  OSC sticks: %s | %s (floats x y, -1..1)\n",
               mapping->osc.addr_left_stick, mapping->osc.addr_right_stick);
        printf("  OSC triggers: %s | %s (float 0..1)\n",
               mapping->osc.addr_left_trigger, mapping->osc.addr_right_trigger);
        printf("  OSC buttons: %s ... (int 0/1)\n", mapping->osc.addr_a);
    }
    printf("  Left stick: %s\n", stick_mode_names[mapping->sticks.left_stick_mode]);
    printf("  Right stick: %s\n", stick_mode_names[mapping->sticks.right_stick_mode]);
    printf("  Left trigger: %s\n", trigger_mode_names[mapping->triggers.left_trigger_mode]);
    printf("  Right trigger: %s\n", trigger_mode_names[mapping->triggers.right_trigger_mode]);
    printf("  Deadzone: %d (%.1f%%)\n", mapping->sticks.deadzone,
           (mapping->sticks.deadzone / 32767.0f) * 100.0f);
    printf("  Mouse smoothing: %.2f\n", mapping->sticks.mouse_smoothing);
    printf("  Mouse sensitivity: %.1f\n", mapping->sticks.mouse_sensitivity);
    printf("  Mouse curve: %.1f\n", mapping->sticks.mouse_curve);
    printf("  Streaming mode: %s\n", mapping->streaming_mode ? "ENABLED" : "disabled");
    printf("  Rumble: %s\n", mapping->features.rumble.enabled ? "enabled" : "disabled");
    printf("  Turbo: %s\n", mapping->features.turbo.enabled ? "enabled" : "disabled");
}
