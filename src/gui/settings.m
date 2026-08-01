/*******************************************************************************
 * settings.m - Settings window (Objective-C implementation)
 *
 * Essentials-only editor. Reads the config JSON on open, writes it back via
 * config_save() on Save; the driver's hot-reload applies the changes. Fields
 * the window doesn't manage (per-button mappings etc.) are preserved because
 * Save re-loads the file fresh before applying the UI values.
 ******************************************************************************/

#import <Cocoa/Cocoa.h>
#include "../../include/settings.h"
#include "../../include/driver.h"
#include "../../include/config.h"

/*******************************************************************************
 * Layout Constants
 ******************************************************************************/
#define WIN_W        460
#define WIN_H        532
#define LABEL_X      20
#define LABEL_W      140
#define CONTROL_X    170
#define CONTROL_W    260
#define ROW_H        28
#define ROW_GAP      8

/*******************************************************************************
 * SettingsController
 ******************************************************************************/
@interface SettingsController : NSObject <NSWindowDelegate>
@property (nonatomic, strong) NSWindow *window;
@property (nonatomic, assign) DriverContext *ctx;
@property (nonatomic, copy) NSString *loadedPrefix;

@property (nonatomic, strong) NSPopUpButton *modePopup;
@property (nonatomic, strong) NSPopUpButton *channelPopup;
@property (nonatomic, strong) NSTextField *velocityField;
@property (nonatomic, strong) NSButton *invertYCheck;
@property (nonatomic, strong) NSTextField *deviceNameField;
@property (nonatomic, strong) NSTextField *hostField;
@property (nonatomic, strong) NSTextField *portField;
@property (nonatomic, strong) NSTextField *prefixField;
@property (nonatomic, strong) NSSlider *deadzoneSlider;
@property (nonatomic, strong) NSTextField *deadzoneValueLabel;
@property (nonatomic, strong) NSButton *rumbleCheck;
@end

@implementation SettingsController

/*******************************************************************************
 * Control Builders
 ******************************************************************************/
- (NSTextField *)addLabel:(NSString *)text atY:(CGFloat)y {
    NSTextField *label = [NSTextField labelWithString:text];
    label.frame = NSMakeRect(LABEL_X, y + 3, LABEL_W, 20);
    label.alignment = NSTextAlignmentRight;
    [self.window.contentView addSubview:label];
    return label;
}

- (NSTextField *)addSectionHeader:(NSString *)text atY:(CGFloat)y {
    NSTextField *label = [NSTextField labelWithString:text];
    label.frame = NSMakeRect(LABEL_X, y, WIN_W - 2 * LABEL_X, 20);
    label.font = [NSFont boldSystemFontOfSize:13];
    label.textColor = [NSColor secondaryLabelColor];
    [self.window.contentView addSubview:label];
    return label;
}

- (NSTextField *)addCaption:(NSString *)text atY:(CGFloat)y {
    NSTextField *label = [NSTextField labelWithString:text];
    label.frame = NSMakeRect(CONTROL_X, y, CONTROL_W, 16);
    label.font = [NSFont systemFontOfSize:10];
    label.textColor = [NSColor tertiaryLabelColor];
    [self.window.contentView addSubview:label];
    return label;
}

- (NSTextField *)addField:(CGFloat)y width:(CGFloat)width {
    NSTextField *field = [[NSTextField alloc] initWithFrame:NSMakeRect(CONTROL_X, y, width, 24)];
    [self.window.contentView addSubview:field];
    return field;
}

/*******************************************************************************
 * Window Construction
 ******************************************************************************/
- (void)buildWindow {
    NSRect frame = NSMakeRect(0, 0, WIN_W, WIN_H);
    self.window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    self.window.title = @"Xbox Controller Driver Settings";
    self.window.releasedWhenClosed = NO;
    self.window.delegate = self;
    [self.window center];

    CGFloat y = WIN_H - 48;

    // --- Output mode ---
    [self addLabel:@"Output Mode:" atY:y];
    self.modePopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(CONTROL_X, y, CONTROL_W, 26)];
    [self.modePopup addItemsWithTitles:@[@"Keyboard / Mouse", @"MIDI", @"OSC", @"MIDI + OSC"]];
    [self.window.contentView addSubview:self.modePopup];
    y -= ROW_H + ROW_GAP + 6;

    // --- MIDI section ---
    [self addSectionHeader:@"MIDI" atY:y];
    y -= ROW_H;

    [self addLabel:@"Channel:" atY:y];
    self.channelPopup = [[NSPopUpButton alloc] initWithFrame:NSMakeRect(CONTROL_X, y, 90, 26)];
    for (int i = 1; i <= 16; i++) {
        [self.channelPopup addItemWithTitle:[NSString stringWithFormat:@"%d", i]];
    }
    [self.window.contentView addSubview:self.channelPopup];
    y -= ROW_H;

    [self addLabel:@"Note Velocity:" atY:y];
    self.velocityField = [self addField:y width:60];
    y -= ROW_H;

    [self addLabel:@"" atY:y];
    self.invertYCheck = [NSButton checkboxWithTitle:@"Invert stick Y axis"
                                             target:nil action:nil];
    self.invertYCheck.frame = NSMakeRect(CONTROL_X, y, CONTROL_W, 24);
    [self.window.contentView addSubview:self.invertYCheck];
    y -= ROW_H;

    [self addLabel:@"Device Name:" atY:y];
    self.deviceNameField = [self addField:y width:CONTROL_W];
    y -= 20;
    [self addCaption:@"Name change requires restarting the app" atY:y];
    y -= ROW_H;

    // --- OSC section ---
    [self addSectionHeader:@"OSC" atY:y];
    y -= ROW_H;

    [self addLabel:@"Host:" atY:y];
    self.hostField = [self addField:y width:CONTROL_W];
    y -= ROW_H;

    [self addLabel:@"Port:" atY:y];
    self.portField = [self addField:y width:80];
    y -= ROW_H;

    [self addLabel:@"Address Prefix:" atY:y];
    self.prefixField = [self addField:y width:CONTROL_W];
    y -= 20;
    [self addCaption:@"Changing it rebuilds every address (e.g. /pad/stick/left)" atY:y];
    y -= ROW_H;

    // --- General section ---
    [self addSectionHeader:@"General" atY:y];
    y -= ROW_H;

    [self addLabel:@"Stick Deadzone:" atY:y];
    self.deadzoneSlider = [[NSSlider alloc] initWithFrame:NSMakeRect(CONTROL_X, y, CONTROL_W - 60, 24)];
    self.deadzoneSlider.minValue = 0;
    self.deadzoneSlider.maxValue = 16000;
    self.deadzoneSlider.target = self;
    self.deadzoneSlider.action = @selector(deadzoneChanged:);
    [self.window.contentView addSubview:self.deadzoneSlider];
    self.deadzoneValueLabel = [NSTextField labelWithString:@"24%"];
    self.deadzoneValueLabel.frame = NSMakeRect(CONTROL_X + CONTROL_W - 52, y + 3, 52, 20);
    [self.window.contentView addSubview:self.deadzoneValueLabel];
    y -= ROW_H;

    [self addLabel:@"" atY:y];
    self.rumbleCheck = [NSButton checkboxWithTitle:@"Rumble on button press"
                                            target:nil action:nil];
    self.rumbleCheck.frame = NSMakeRect(CONTROL_X, y, CONTROL_W, 24);
    [self.window.contentView addSubview:self.rumbleCheck];

    // --- Bottom buttons ---
    NSButton *jsonButton = [NSButton buttonWithTitle:@"Edit Mappings (JSON)..."
                                              target:self
                                              action:@selector(openJson:)];
    jsonButton.frame = NSMakeRect(16, 14, 180, 30);
    [self.window.contentView addSubview:jsonButton];

    NSButton *cancelButton = [NSButton buttonWithTitle:@"Cancel"
                                                target:self
                                                action:@selector(cancel:)];
    cancelButton.frame = NSMakeRect(WIN_W - 190, 14, 84, 30);
    cancelButton.keyEquivalent = @"\033";
    [self.window.contentView addSubview:cancelButton];

    NSButton *saveButton = [NSButton buttonWithTitle:@"Save"
                                              target:self
                                              action:@selector(save:)];
    saveButton.frame = NSMakeRect(WIN_W - 100, 14, 84, 30);
    saveButton.keyEquivalent = @"\r";
    [self.window.contentView addSubview:saveButton];
}

/*******************************************************************************
 * Populate From Config
 ******************************************************************************/

// Derive the common prefix from the left-stick address, if it follows the
// "<prefix>/stick/left" pattern; empty string otherwise
- (NSString *)derivePrefix:(const ControllerMapping *)mapping {
    NSString *addr = [NSString stringWithUTF8String:mapping->osc.addr_left_stick];
    NSString *suffix = @"/stick/left";
    if ([addr hasSuffix:suffix] && addr.length > suffix.length) {
        return [addr substringToIndex:addr.length - suffix.length];
    }
    return @"";
}

- (void)populate {
    ControllerMapping mapping;
    if (config_load(self.ctx->config_path, &mapping) != 0) {
        mapping = self.ctx->config;
    }

    [self.modePopup selectItemAtIndex:(NSInteger)mapping.output_mode];
    [self.channelPopup selectItemAtIndex:mapping.midi.channel];
    self.velocityField.integerValue = mapping.midi.velocity;
    self.invertYCheck.state = mapping.midi.invert_y ? NSControlStateValueOn : NSControlStateValueOff;
    self.deviceNameField.stringValue = [NSString stringWithUTF8String:mapping.midi.device_name];
    self.hostField.stringValue = [NSString stringWithUTF8String:mapping.osc.host];
    self.portField.integerValue = mapping.osc.port;
    self.loadedPrefix = [self derivePrefix:&mapping];
    self.prefixField.stringValue = self.loadedPrefix;
    self.deadzoneSlider.integerValue = mapping.sticks.deadzone;
    self.rumbleCheck.state = mapping.features.rumble.enabled ? NSControlStateValueOn : NSControlStateValueOff;
    [self deadzoneChanged:self.deadzoneSlider];
}

/*******************************************************************************
 * Actions
 ******************************************************************************/
- (void)deadzoneChanged:(NSSlider *)sender {
    self.deadzoneValueLabel.stringValue =
        [NSString stringWithFormat:@"%.0f%%", (sender.doubleValue / 32767.0) * 100.0];
}

- (void)openJson:(id)sender {
    (void)sender;
    NSString *path = [NSString stringWithUTF8String:self.ctx->config_path];
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:path]];
}

- (void)cancel:(id)sender {
    (void)sender;
    [self.window close];
}

- (void)save:(id)sender {
    (void)sender;

    // Load fresh so hand-edits to fields this window doesn't manage survive
    ControllerMapping mapping;
    if (config_load(self.ctx->config_path, &mapping) != 0) {
        mapping = self.ctx->config;
    }

    mapping.output_mode = (OutputMode)self.modePopup.indexOfSelectedItem;

    mapping.midi.channel = (uint8_t)self.channelPopup.indexOfSelectedItem;
    NSInteger velocity = self.velocityField.integerValue;
    if (velocity >= 1 && velocity <= 127) mapping.midi.velocity = (uint8_t)velocity;
    mapping.midi.invert_y = (self.invertYCheck.state == NSControlStateValueOn);
    const char *deviceName = self.deviceNameField.stringValue.UTF8String;
    if (deviceName && deviceName[0] != '\0') {
        strncpy(mapping.midi.device_name, deviceName, MIDI_DEVICE_NAME_MAX - 1);
        mapping.midi.device_name[MIDI_DEVICE_NAME_MAX - 1] = '\0';
    }

    const char *host = self.hostField.stringValue.UTF8String;
    if (host && host[0] != '\0') {
        strncpy(mapping.osc.host, host, OSC_HOST_MAX - 1);
        mapping.osc.host[OSC_HOST_MAX - 1] = '\0';
    }
    NSInteger port = self.portField.integerValue;
    if (port >= 1 && port <= 65535) mapping.osc.port = (uint16_t)port;

    NSString *prefix = self.prefixField.stringValue;
    if (![prefix isEqualToString:self.loadedPrefix] && [prefix hasPrefix:@"/"] &&
        prefix.length < OSC_PREFIX_MAX) {
        config_build_osc_addresses(&mapping.osc, prefix.UTF8String);
    }

    mapping.sticks.deadzone = (int16_t)self.deadzoneSlider.integerValue;
    mapping.features.rumble.enabled = (self.rumbleCheck.state == NSControlStateValueOn);

    if (config_save(self.ctx->config_path, &mapping) != 0) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Could Not Save Settings"];
        [alert setInformativeText:[NSString stringWithFormat:@"Failed to write %s",
                                   self.ctx->config_path]];
        [alert setAlertStyle:NSAlertStyleCritical];
        [alert runModal];
        return;
    }

    [self.window close];
}

@end

/*******************************************************************************
 * Global State + C Interface
 ******************************************************************************/
static SettingsController *g_settings = nil;

void settings_show(void *driver_ctx) {
    @autoreleasepool {
        if (!g_settings) {
            g_settings = [[SettingsController alloc] init];
            g_settings.ctx = (DriverContext *)driver_ctx;
            [g_settings buildWindow];
        }
        [g_settings populate];
        [NSApp activateIgnoringOtherApps:YES];
        [g_settings.window makeKeyAndOrderFront:nil];
    }
}
