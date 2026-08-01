/*******************************************************************************
 * test_config.c - Configuration unit tests
 ******************************************************************************/

#include "test_framework.h"
#include "../include/types.h"
#include "../include/config.h"

/*******************************************************************************
 * Default Configuration Tests
 ******************************************************************************/
TEST(config_defaults_buttons) {
    ControllerMapping mapping;
    config_get_defaults(&mapping);

    ASSERT_EQ(mapping.buttons.key_a, 0x31);  // Space
    ASSERT_EQ(mapping.buttons.key_b, 0x08);  // C
    ASSERT_EQ(mapping.buttons.key_x, 0x0F);  // R
    ASSERT_EQ(mapping.buttons.key_y, 0x03);  // F
    TEST_PASS();
}

TEST(config_defaults_sticks) {
    ControllerMapping mapping;
    config_get_defaults(&mapping);

    ASSERT_EQ(mapping.sticks.left_stick_mode, STICK_MODE_WASD);
    ASSERT_EQ(mapping.sticks.right_stick_mode, STICK_MODE_MOUSE);
    ASSERT_EQ(mapping.sticks.deadzone, DEFAULT_DEADZONE);
    ASSERT_FLOAT_EQ(mapping.sticks.mouse_sensitivity, DEFAULT_SENSITIVITY, 0.001f);
    TEST_PASS();
}

TEST(config_defaults_triggers) {
    ControllerMapping mapping;
    config_get_defaults(&mapping);

    ASSERT_EQ(mapping.triggers.left_trigger_mode, TRIGGER_MODE_MOUSE);
    ASSERT_EQ(mapping.triggers.right_trigger_mode, TRIGGER_MODE_MOUSE);
    ASSERT_EQ(mapping.triggers.threshold, DEFAULT_THRESHOLD);
    TEST_PASS();
}

TEST(config_defaults_features) {
    ControllerMapping mapping;
    config_get_defaults(&mapping);

    ASSERT_TRUE(mapping.features.rumble.enabled);
    ASSERT_TRUE(mapping.features.turbo.enabled);
    ASSERT_FALSE(mapping.features.analog_keyboard.enabled);
    TEST_PASS();
}

/*******************************************************************************
 * Key Parsing Tests
 ******************************************************************************/
TEST(config_parse_key_letters) {
    ASSERT_EQ(config_parse_key("a"), 0x00);
    ASSERT_EQ(config_parse_key("A"), 0x00);  // Case insensitive
    ASSERT_EQ(config_parse_key("z"), 0x06);
    ASSERT_EQ(config_parse_key("w"), 0x0D);
    ASSERT_EQ(config_parse_key("s"), 0x01);
    TEST_PASS();
}

TEST(config_parse_key_special) {
    ASSERT_EQ(config_parse_key("space"), 0x31);
    ASSERT_EQ(config_parse_key("SPACE"), 0x31);  // Case insensitive
    ASSERT_EQ(config_parse_key("return"), 0x24);
    ASSERT_EQ(config_parse_key("enter"), 0x24);
    ASSERT_EQ(config_parse_key("tab"), 0x30);
    ASSERT_EQ(config_parse_key("escape"), 0x35);
    ASSERT_EQ(config_parse_key("esc"), 0x35);
    TEST_PASS();
}

TEST(config_parse_key_modifiers) {
    ASSERT_EQ(config_parse_key("left_shift"), 0x38);
    ASSERT_EQ(config_parse_key("shift"), 0x38);
    ASSERT_EQ(config_parse_key("left_control"), 0x3B);
    ASSERT_EQ(config_parse_key("ctrl"), 0x3B);
    ASSERT_EQ(config_parse_key("left_option"), 0x3A);
    ASSERT_EQ(config_parse_key("alt"), 0x3A);
    TEST_PASS();
}

TEST(config_parse_key_arrows) {
    ASSERT_EQ(config_parse_key("up"), 0x7E);
    ASSERT_EQ(config_parse_key("down"), 0x7D);
    ASSERT_EQ(config_parse_key("left"), 0x7B);
    ASSERT_EQ(config_parse_key("right"), 0x7C);
    TEST_PASS();
}

TEST(config_parse_key_hex) {
    ASSERT_EQ(config_parse_key("0x31"), 0x31);
    ASSERT_EQ(config_parse_key("0x7E"), 0x7E);
    ASSERT_EQ(config_parse_key("0xFF"), 0xFF);
    TEST_PASS();
}

TEST(config_parse_key_invalid) {
    ASSERT_EQ(config_parse_key("invalid_key_name"), 0xFFFF);
    ASSERT_EQ(config_parse_key(""), 0xFFFF);
    ASSERT_EQ(config_parse_key(NULL), 0xFFFF);
    TEST_PASS();
}

/*******************************************************************************
 * Key Name Tests
 ******************************************************************************/
TEST(config_key_name) {
    ASSERT_STR_EQ(config_key_name(0x00), "a");
    ASSERT_STR_EQ(config_key_name(0x31), "space");
    ASSERT_STR_EQ(config_key_name(0x7E), "up");
    TEST_PASS();
}

/*******************************************************************************
 * Save/Load Round-Trip Tests
 ******************************************************************************/
TEST(config_save_load_roundtrip) {
    const char *path = "/tmp/xbox-driver-test-config.json";

    ControllerMapping original;
    config_get_defaults(&original);
    original.output_mode = OUTPUT_MODE_MIDI_OSC;
    original.buttons.key_a = 0x24;                    // return
    original.sticks.deadzone = 5000;
    original.sticks.left_stick_mode = STICK_MODE_ARROWS;
    original.triggers.left_trigger_mode = TRIGGER_MODE_KEY;
    original.triggers.left_trigger_key = 0x06;        // z
    original.midi.channel = 9;                        // channel 10 in JSON
    original.midi.velocity = 100;
    original.midi.invert_y = true;
    original.midi.note_a = 60;
    original.midi.cc_left_x = 70;
    original.osc.port = 7500;
    config_build_osc_addresses(&original.osc, "/pad");
    original.features.rumble.enabled = false;
    original.log_level = LOG_LEVEL_WARN;

    ASSERT_EQ(config_save(path, &original), 0);

    ControllerMapping loaded;
    ASSERT_EQ(config_load(path, &loaded), 0);
    remove(path);

    ASSERT_EQ(loaded.output_mode, OUTPUT_MODE_MIDI_OSC);
    ASSERT_EQ(loaded.buttons.key_a, 0x24);
    ASSERT_EQ(loaded.sticks.deadzone, 5000);
    ASSERT_EQ(loaded.sticks.left_stick_mode, STICK_MODE_ARROWS);
    ASSERT_EQ(loaded.triggers.left_trigger_mode, TRIGGER_MODE_KEY);
    ASSERT_EQ(loaded.triggers.left_trigger_key, 0x06);
    ASSERT_EQ(loaded.midi.channel, 9);
    ASSERT_EQ(loaded.midi.velocity, 100);
    ASSERT_TRUE(loaded.midi.invert_y);
    ASSERT_EQ(loaded.midi.note_a, 60);
    ASSERT_EQ(loaded.midi.cc_left_x, 70);
    ASSERT_EQ(loaded.osc.port, 7500);
    ASSERT_STR_EQ(loaded.osc.addr_left_stick, "/pad/stick/left");
    ASSERT_STR_EQ(loaded.osc.addr_dpad_right, "/pad/dpad/right");
    ASSERT_FALSE(loaded.features.rumble.enabled);
    ASSERT_EQ(loaded.log_level, LOG_LEVEL_WARN);
    TEST_PASS();
}

/*******************************************************************************
 * Main
 ******************************************************************************/
int main(void) {
    TEST_SUITE("Configuration");

    printf("\n-- Default Configuration Tests --\n");
    RUN_TEST(config_defaults_buttons);
    RUN_TEST(config_defaults_sticks);
    RUN_TEST(config_defaults_triggers);
    RUN_TEST(config_defaults_features);

    printf("\n-- Key Parsing Tests --\n");
    RUN_TEST(config_parse_key_letters);
    RUN_TEST(config_parse_key_special);
    RUN_TEST(config_parse_key_modifiers);
    RUN_TEST(config_parse_key_arrows);
    RUN_TEST(config_parse_key_hex);
    RUN_TEST(config_parse_key_invalid);

    printf("\n-- Key Name Tests --\n");
    RUN_TEST(config_key_name);

    printf("\n-- Save/Load Round-Trip Tests --\n");
    RUN_TEST(config_save_load_roundtrip);

    TEST_SUMMARY();
    return TEST_EXIT_CODE();
}
