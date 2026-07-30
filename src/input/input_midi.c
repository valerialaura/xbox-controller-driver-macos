/*******************************************************************************
 * input_midi.c - Controller state -> MIDI message translation
 *
 * Mirrors the keyboard/mouse processors in input.c, but emits MIDI notes
 * (buttons) and CCs (sticks, triggers) through the CoreMIDI virtual source.
 ******************************************************************************/

#include "../../include/midi.h"
#include "../../include/input.h"
#include "../../include/log.h"
#include <string.h>

void midi_state_init(MidiState *state) {
    memset(state->last_cc, 0xFF, sizeof(state->last_cc));
    memset(state->notes_on, 0, sizeof(state->notes_on));
}

/*******************************************************************************
 * CC Dedupe
 ******************************************************************************/
static void send_cc_deduped(InputState *state, const MidiMapping *midi,
                            uint8_t cc, uint8_t value) {
    if (cc > 127) return;
    if (state->midi.last_cc[cc] == value) return;
    state->midi.last_cc[cc] = value;
    midi_send_cc(midi->channel, cc, value);
}

/*******************************************************************************
 * Axis Mapping
 ******************************************************************************/
uint8_t midi_map_axis(int16_t value) {
    // -32767..32767 -> 0..127 with exact center 64 (deadzone zeroes the center)
    if (value <= -32767) return 0;
    int32_t mapped = (((int32_t)value + 32767) * 127 + 32767) / 65534;
    if (mapped > 127) mapped = 127;
    return (uint8_t)mapped;
}

/*******************************************************************************
 * Button Processing (notes)
 ******************************************************************************/
void process_buttons_midi(uint16_t buttons, InputState *state,
                          const ControllerMapping *config,
                          void (*rumble_callback)(void *ctx), void *rumble_ctx) {
    const MidiMapping *midi = &config->midi;

    struct {
        uint16_t mask;
        uint8_t note;
    } button_map[] = {
        {XBOX_BTN_A, midi->note_a},
        {XBOX_BTN_B, midi->note_b},
        {XBOX_BTN_X, midi->note_x},
        {XBOX_BTN_Y, midi->note_y},
        {XBOX_BTN_LB, midi->note_lb},
        {XBOX_BTN_RB, midi->note_rb},
        {XBOX_BTN_LS, midi->note_ls},
        {XBOX_BTN_RS, midi->note_rs},
        {XBOX_BTN_VIEW, midi->note_view},
        {XBOX_BTN_MENU, midi->note_menu},
        {XBOX_BTN_DPAD_UP, midi->note_dpad_up},
        {XBOX_BTN_DPAD_DOWN, midi->note_dpad_down},
        {XBOX_BTN_DPAD_LEFT, midi->note_dpad_left},
        {XBOX_BTN_DPAD_RIGHT, midi->note_dpad_right}
    };

    for (int i = 0; i < XBOX_BTN_COUNT; i++) {
        bool is_pressed = (buttons & button_map[i].mask) != 0;
        bool was_pressed = (state->prev_buttons & button_map[i].mask) != 0;

        if (is_pressed != was_pressed) {
            uint8_t note = button_map[i].note & 0x7F;
            midi_send_note(midi->channel, note, is_pressed, midi->velocity);
            state->midi.notes_on[note] = is_pressed;

            if (is_pressed && config->features.rumble.enabled &&
                config->features.rumble.button_feedback && rumble_callback) {
                rumble_callback(rumble_ctx);
            }
        }
    }

    state->prev_buttons = buttons;
}

/*******************************************************************************
 * Trigger Processing (CCs)
 ******************************************************************************/
void process_triggers_midi(uint8_t left_trigger, uint8_t right_trigger,
                           InputState *state, const ControllerMapping *config) {
    const MidiMapping *midi = &config->midi;

    // Triggers are swapped in the GIP packet (see process_triggers)
    send_cc_deduped(state, midi, midi->cc_right_trigger, left_trigger >> 1);
    send_cc_deduped(state, midi, midi->cc_left_trigger, right_trigger >> 1);

    state->prev_left_trigger = right_trigger;
    state->prev_right_trigger = left_trigger;
}

/*******************************************************************************
 * Stick Processing (CCs)
 ******************************************************************************/
static void process_stick_axes_midi(int16_t x, int16_t y, InputState *state,
                                    const MidiMapping *midi,
                                    uint8_t cc_x, uint8_t cc_y) {
    // Axes are swapped in the controller - swap them back (see input.c)
    int16_t temp = x;
    x = y;
    y = temp;

    if (midi->invert_y) y = (y == -32768) ? 32767 : -y;

    send_cc_deduped(state, midi, cc_x, midi_map_axis(x));
    send_cc_deduped(state, midi, cc_y, midi_map_axis(y));
}

void process_sticks_midi(int16_t left_x, int16_t left_y,
                         int16_t right_x, int16_t right_y,
                         InputState *state, const ControllerMapping *config) {
    apply_deadzone(&left_x, &left_y, config->sticks.deadzone);
    apply_deadzone(&right_x, &right_y, config->sticks.deadzone);

    state->dz_left_stick_x = left_x;
    state->dz_left_stick_y = left_y;
    state->dz_right_stick_x = right_x;
    state->dz_right_stick_y = right_y;

    process_stick_axes_midi(left_x, left_y, state, &config->midi,
                            config->midi.cc_left_x, config->midi.cc_left_y);
    process_stick_axes_midi(right_x, right_y, state, &config->midi,
                            config->midi.cc_right_x, config->midi.cc_right_y);

    state->current_left_stick_x = left_x;
    state->current_left_stick_y = left_y;
    state->current_right_stick_x = right_x;
    state->current_right_stick_y = right_y;
}

/*******************************************************************************
 * Release All (note-offs on shutdown/mode switch)
 ******************************************************************************/
void midi_release_all(InputState *state, const ControllerMapping *config) {
    if (!midi_is_ready()) return;

    LOG_INFO("Releasing all MIDI notes...");
    for (int note = 0; note < 128; note++) {
        if (state->midi.notes_on[note]) {
            midi_send_note(config->midi.channel, (uint8_t)note, false, 0);
            state->midi.notes_on[note] = false;
        }
    }
}
