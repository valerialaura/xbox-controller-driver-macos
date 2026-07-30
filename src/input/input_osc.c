/*******************************************************************************
 * input_osc.c - Controller state -> OSC message translation
 *
 * Sticks: two floats per stick (x, y in -1..1, full 16-bit resolution).
 * Triggers: one float each (0..1). Buttons: one int each (0/1).
 * Keeps its own dedupe state so it can run alongside the MIDI mode.
 ******************************************************************************/

#include "../../include/osc.h"
#include "../../include/input.h"
#include "../../include/log.h"

/*******************************************************************************
 * Button Processing (ints)
 ******************************************************************************/
void process_buttons_osc(uint16_t buttons, InputState *state,
                         const ControllerMapping *config,
                         void (*rumble_callback)(void *ctx), void *rumble_ctx) {
    const OscMapping *osc = &config->osc;

    struct {
        uint16_t mask;
        const char *addr;
    } button_map[] = {
        {XBOX_BTN_A, osc->addr_a},
        {XBOX_BTN_B, osc->addr_b},
        {XBOX_BTN_X, osc->addr_x},
        {XBOX_BTN_Y, osc->addr_y},
        {XBOX_BTN_LB, osc->addr_lb},
        {XBOX_BTN_RB, osc->addr_rb},
        {XBOX_BTN_LS, osc->addr_ls},
        {XBOX_BTN_RS, osc->addr_rs},
        {XBOX_BTN_VIEW, osc->addr_view},
        {XBOX_BTN_MENU, osc->addr_menu},
        {XBOX_BTN_DPAD_UP, osc->addr_dpad_up},
        {XBOX_BTN_DPAD_DOWN, osc->addr_dpad_down},
        {XBOX_BTN_DPAD_LEFT, osc->addr_dpad_left},
        {XBOX_BTN_DPAD_RIGHT, osc->addr_dpad_right}
    };

    for (int i = 0; i < XBOX_BTN_COUNT; i++) {
        bool is_pressed = (buttons & button_map[i].mask) != 0;
        bool was_pressed = (state->osc.prev_buttons & button_map[i].mask) != 0;

        if (is_pressed != was_pressed || !state->osc.buttons_primed) {
            osc_send_i(button_map[i].addr, is_pressed ? 1 : 0);

            if (is_pressed && !was_pressed && config->features.rumble.enabled &&
                config->features.rumble.button_feedback && rumble_callback) {
                rumble_callback(rumble_ctx);
            }
        }
    }

    state->osc.prev_buttons = buttons;
    state->osc.buttons_primed = true;
}

/*******************************************************************************
 * Trigger Processing (floats 0..1)
 ******************************************************************************/
void process_triggers_osc(uint8_t left_trigger, uint8_t right_trigger,
                          InputState *state, const ControllerMapping *config) {
    const OscMapping *osc = &config->osc;

    // Triggers are swapped in the GIP packet (see process_triggers)
    uint8_t rt = left_trigger;
    uint8_t lt = right_trigger;

    if (lt != state->osc.last_lt || !state->osc.triggers_primed) {
        osc_send_f(osc->addr_left_trigger, (float)lt / 255.0f);
        state->osc.last_lt = lt;
    }
    if (rt != state->osc.last_rt || !state->osc.triggers_primed) {
        osc_send_f(osc->addr_right_trigger, (float)rt / 255.0f);
        state->osc.last_rt = rt;
    }
    state->osc.triggers_primed = true;
}

/*******************************************************************************
 * Stick Processing (float pairs, -1..1, up/right positive)
 ******************************************************************************/
static float axis_to_float(int16_t v) {
    float f = (float)v / (float)STICK_MAX;
    if (f > 1.0f) f = 1.0f;
    if (f < -1.0f) f = -1.0f;
    return f;
}

void process_sticks_osc(int16_t left_x, int16_t left_y,
                        int16_t right_x, int16_t right_y,
                        InputState *state, const ControllerMapping *config) {
    const OscMapping *osc = &config->osc;

    apply_deadzone(&left_x, &left_y, config->sticks.deadzone);
    apply_deadzone(&right_x, &right_y, config->sticks.deadzone);

    // Axes are swapped in the controller - swap them back (see input.c)
    int16_t lx = left_y, ly = left_x;
    int16_t rx = right_y, ry = right_x;

    if (lx != state->osc.last_lx || ly != state->osc.last_ly || !state->osc.sticks_primed) {
        osc_send_ff(osc->addr_left_stick, axis_to_float(lx), axis_to_float(ly));
        state->osc.last_lx = lx;
        state->osc.last_ly = ly;
    }
    if (rx != state->osc.last_rx || ry != state->osc.last_ry || !state->osc.sticks_primed) {
        osc_send_ff(osc->addr_right_stick, axis_to_float(rx), axis_to_float(ry));
        state->osc.last_rx = rx;
        state->osc.last_ry = ry;
    }
    state->osc.sticks_primed = true;
}

/*******************************************************************************
 * Release All (zero out held buttons on shutdown/mode switch)
 ******************************************************************************/
void osc_release_all(InputState *state, const ControllerMapping *config) {
    if (!osc_is_ready() || !state->osc.buttons_primed) return;

    LOG_INFO("Releasing all OSC buttons...");
    // Re-run button processing with an all-released bitmask
    process_buttons_osc(0, state, config, NULL, NULL);
}
