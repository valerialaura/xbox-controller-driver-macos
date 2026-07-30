/*******************************************************************************
 * osc.h - OSC over UDP output (OSC mode)
 ******************************************************************************/

#ifndef OSC_H
#define OSC_H

#include "types.h"

/*******************************************************************************
 * OSC Backend (UDP socket, minimal OSC 1.0 encoding)
 ******************************************************************************/

/**
 * Create the UDP socket and resolve the target host/port.
 * Returns 0 on success, -1 on error.
 */
int osc_init(const char *host, uint16_t port);

/**
 * Re-resolve the target (for config hot-reload). Returns 0 on success.
 */
int osc_set_target(const char *host, uint16_t port);

/**
 * Close the socket.
 */
void osc_cleanup(void);

/**
 * True once osc_init succeeded.
 */
bool osc_is_ready(void);

/**
 * Send a message with one int32 argument (buttons: 0/1).
 */
void osc_send_i(const char *address, int32_t value);

/**
 * Send a message with one float argument (triggers: 0..1).
 */
void osc_send_f(const char *address, float value);

/**
 * Send a message with two float arguments (sticks: x, y in -1..1).
 */
void osc_send_ff(const char *address, float a, float b);

/*******************************************************************************
 * OSC Translation (controller state -> OSC messages)
 ******************************************************************************/

void process_buttons_osc(uint16_t buttons, InputState *state,
                         const ControllerMapping *config,
                         void (*rumble_callback)(void *ctx), void *rumble_ctx);

void process_triggers_osc(uint8_t left_trigger, uint8_t right_trigger,
                          InputState *state, const ControllerMapping *config);

void process_sticks_osc(int16_t left_x, int16_t left_y,
                        int16_t right_x, int16_t right_y,
                        InputState *state, const ControllerMapping *config);

/**
 * Send 0 for every button currently held (call on shutdown/mode switch).
 */
void osc_release_all(InputState *state, const ControllerMapping *config);

#endif // OSC_H
