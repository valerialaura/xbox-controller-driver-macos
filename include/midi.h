/*******************************************************************************
 * midi.h - CoreMIDI virtual source output (MIDI mode)
 ******************************************************************************/

#ifndef MIDI_H
#define MIDI_H

#include "types.h"

/*******************************************************************************
 * MIDI Backend (CoreMIDI virtual source)
 ******************************************************************************/

/**
 * Create the MIDI client and a virtual source with the given name.
 * The source appears as a MIDI input device in Audio MIDI Setup / DAWs.
 * Returns 0 on success, -1 on error.
 */
int midi_init(const char *device_name);

/**
 * Destroy the virtual source and client.
 */
void midi_cleanup(void);

/**
 * True once midi_init succeeded.
 */
bool midi_is_ready(void);

/**
 * Send a Control Change message.
 */
void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value);

/**
 * Send a Note On (on=true) or Note Off (on=false) message.
 */
void midi_send_note(uint8_t channel, uint8_t note, bool on, uint8_t velocity);

/*******************************************************************************
 * MIDI Translation (controller state -> MIDI messages)
 ******************************************************************************/

void process_buttons_midi(uint16_t buttons, InputState *state,
                          const ControllerMapping *config,
                          void (*rumble_callback)(void *ctx), void *rumble_ctx);

void process_triggers_midi(uint8_t left_trigger, uint8_t right_trigger,
                           InputState *state, const ControllerMapping *config);

void process_sticks_midi(int16_t left_x, int16_t left_y,
                         int16_t right_x, int16_t right_y,
                         InputState *state, const ControllerMapping *config);

/**
 * Map a post-deadzone stick axis (-32768..32767) to a CC value (0..127, center 64).
 */
uint8_t midi_map_axis(int16_t value);

/**
 * Reset dedupe/note state (0xFF = never sent).
 */
void midi_state_init(MidiState *state);

/**
 * Send Note Off for every note currently on (call on shutdown/mode switch).
 */
void midi_release_all(InputState *state, const ControllerMapping *config);

#endif // MIDI_H
