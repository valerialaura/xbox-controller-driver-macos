/*******************************************************************************
 * midi.c - CoreMIDI virtual source output
 *
 * Creates a virtual MIDI source that appears as an input device system-wide
 * (Audio MIDI Setup, Ableton Live, etc.). No special permissions required.
 ******************************************************************************/

#include "../../include/midi.h"
#include "../../include/log.h"
#include <CoreMIDI/CoreMIDI.h>
#include <mach/mach_time.h>

/*******************************************************************************
 * Backend State
 ******************************************************************************/
static MIDIClientRef g_client = 0;
static MIDIEndpointRef g_source = 0;
static bool g_ready = false;

// Fixed unique ID so DAW mappings survive restarts. CoreMIDI falls back to a
// generated ID if this collides with another device.
#define MIDI_SOURCE_UNIQUE_ID 0x58421697  // "XB" + model 1697

int midi_init(const char *device_name) {
    if (g_ready) return 0;

    OSStatus status = MIDIClientCreate(CFSTR("XboxControllerDriver"), NULL, NULL, &g_client);
    if (status != noErr) {
        LOG_ERROR("MIDIClientCreate failed: %d", (int)status);
        return -1;
    }

    CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, device_name,
                                                 kCFStringEncodingUTF8);
    if (!name) name = CFSTR("Xbox Controller");

    status = MIDISourceCreateWithProtocol(g_client, name, kMIDIProtocol_1_0, &g_source);
    CFRelease(name);
    if (status != noErr) {
        LOG_ERROR("MIDISourceCreateWithProtocol failed: %d", (int)status);
        MIDIClientDispose(g_client);
        g_client = 0;
        return -1;
    }

    MIDIObjectSetIntegerProperty(g_source, kMIDIPropertyUniqueID, MIDI_SOURCE_UNIQUE_ID);

    g_ready = true;
    LOG_INFO("MIDI virtual source created: %s", device_name);
    return 0;
}

void midi_cleanup(void) {
    if (g_source) {
        MIDIEndpointDispose(g_source);
        g_source = 0;
    }
    if (g_client) {
        MIDIClientDispose(g_client);
        g_client = 0;
    }
    g_ready = false;
}

bool midi_is_ready(void) {
    return g_ready;
}

/*******************************************************************************
 * Message Sending (UMP, MIDI 1.0 protocol)
 ******************************************************************************/
static void send_channel_voice(uint8_t status_byte, uint8_t data1, uint8_t data2) {
    if (!g_ready) return;

    // MIDI 1.0 channel voice message in UMP: type 0x2, group 0
    UInt32 word = ((UInt32)0x2 << 28) |
                  ((UInt32)status_byte << 16) |
                  ((UInt32)(data1 & 0x7F) << 8) |
                  (UInt32)(data2 & 0x7F);

    MIDIEventList list;
    MIDIEventPacket *packet = MIDIEventListInit(&list, kMIDIProtocol_1_0);
    packet = MIDIEventListAdd(&list, sizeof(list), packet, mach_absolute_time(), 1, &word);
    if (packet) {
        MIDIReceivedEventList(g_source, &list);
    }
}

void midi_send_cc(uint8_t channel, uint8_t cc, uint8_t value) {
    send_channel_voice(0xB0 | (channel & 0x0F), cc, value);
}

void midi_send_note(uint8_t channel, uint8_t note, bool on, uint8_t velocity) {
    if (on) {
        send_channel_voice(0x90 | (channel & 0x0F), note, velocity);
    } else {
        send_channel_voice(0x80 | (channel & 0x0F), note, 0);
    }
}
