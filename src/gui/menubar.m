/*******************************************************************************
 * menubar.m - Menu bar application (Objective-C implementation)
 *
 * The menu never mutates the running driver directly: mode switches and
 * settings edits go through config_save() on the JSON file, and the driver's
 * hot-reload picks them up within a second.
 ******************************************************************************/

#import <Cocoa/Cocoa.h>
#include "../../include/menubar.h"
#include "../../include/settings.h"
#include "../../include/driver.h"
#include "../../include/config.h"

/*******************************************************************************
 * AppDelegate
 ******************************************************************************/
@interface AppDelegate : NSObject <NSApplicationDelegate, NSMenuDelegate>
@property (nonatomic, strong) NSStatusItem *statusItem;
@property (nonatomic, strong) NSMenu *statusMenu;
@property (nonatomic, strong) NSMenuItem *statusMenuItem;
@property (nonatomic, strong) NSMenu *modeMenu;
@property (nonatomic, assign) void *context;
@property (nonatomic, assign) MenuBarReloadCallback reloadCallback;
@property (nonatomic, assign) MenuBarQuitCallback quitCallback;
@property (nonatomic, assign) BOOL isConnected;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    [self setupStatusBar];
}

- (void)setupStatusBar {
    // Create status bar item
    self.statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];

    // Set initial icon
    [self updateIcon];

    // Create menu
    self.statusMenu = [[NSMenu alloc] init];
    [self.statusMenu setDelegate:self];

    // Status item (disabled, just shows info)
    self.statusMenuItem = [[NSMenuItem alloc] initWithTitle:@"Status: Initializing..."
                                                     action:nil
                                              keyEquivalent:@""];
    [self.statusMenuItem setEnabled:NO];
    [self.statusMenu addItem:self.statusMenuItem];

    [self.statusMenu addItem:[NSMenuItem separatorItem]];

    // Output Mode submenu
    NSMenuItem *modeItem = [[NSMenuItem alloc] initWithTitle:@"Output Mode"
                                                      action:nil
                                               keyEquivalent:@""];
    self.modeMenu = [[NSMenu alloc] init];
    NSArray *modeTitles = @[@"Keyboard / Mouse", @"MIDI", @"OSC", @"MIDI + OSC"];
    for (NSUInteger i = 0; i < modeTitles.count; i++) {
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:modeTitles[i]
                                                      action:@selector(selectOutputMode:)
                                               keyEquivalent:@""];
        [item setTarget:self];
        [item setTag:(NSInteger)i];  // tag == OutputMode enum value
        [self.modeMenu addItem:item];
    }
    [modeItem setSubmenu:self.modeMenu];
    [self.statusMenu addItem:modeItem];

    // Settings window
    NSMenuItem *settingsItem = [[NSMenuItem alloc] initWithTitle:@"Settings..."
                                                          action:@selector(openSettings:)
                                                   keyEquivalent:@","];
    [settingsItem setTarget:self];
    [self.statusMenu addItem:settingsItem];

    [self.statusMenu addItem:[NSMenuItem separatorItem]];

    // Open Config File
    NSMenuItem *openConfigItem = [[NSMenuItem alloc] initWithTitle:@"Open Config File"
                                                            action:@selector(openConfigFile:)
                                                     keyEquivalent:@"o"];
    [openConfigItem setTarget:self];
    [self.statusMenu addItem:openConfigItem];

    // Reload Config
    NSMenuItem *reloadItem = [[NSMenuItem alloc] initWithTitle:@"Reload Configuration"
                                                        action:@selector(reloadConfig:)
                                                 keyEquivalent:@"r"];
    [reloadItem setTarget:self];
    [self.statusMenu addItem:reloadItem];

    [self.statusMenu addItem:[NSMenuItem separatorItem]];

    // About
    NSMenuItem *aboutItem = [[NSMenuItem alloc] initWithTitle:@"About Xbox Controller Driver"
                                                       action:@selector(showAbout:)
                                                keyEquivalent:@""];
    [aboutItem setTarget:self];
    [self.statusMenu addItem:aboutItem];

    [self.statusMenu addItem:[NSMenuItem separatorItem]];

    // Quit
    NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit"
                                                      action:@selector(quitApp:)
                                               keyEquivalent:@"q"];
    [quitItem setTarget:self];
    [self.statusMenu addItem:quitItem];

    self.statusItem.menu = self.statusMenu;
}

// Refresh the radio checkmarks from the live config every time the menu opens
- (void)menuWillOpen:(NSMenu *)menu {
    if (menu != self.statusMenu) return;
    DriverContext *ctx = (DriverContext *)self.context;
    if (!ctx) return;
    for (NSMenuItem *item in self.modeMenu.itemArray) {
        [item setState:(item.tag == (NSInteger)ctx->config.output_mode)
                           ? NSControlStateValueOn : NSControlStateValueOff];
    }
}

- (void)updateIcon {
    NSImage *icon;

    if (self.isConnected) {
        // Green controller icon when connected
        icon = [NSImage imageWithSystemSymbolName:@"gamecontroller.fill"
                         accessibilityDescription:@"Connected"];
        if (@available(macOS 12.0, *)) {
            NSImageSymbolConfiguration *config = [NSImageSymbolConfiguration configurationWithPaletteColors:@[[NSColor systemGreenColor]]];
            icon = [icon imageWithSymbolConfiguration:config];
        }
    } else {
        // Gray controller icon when disconnected
        icon = [NSImage imageWithSystemSymbolName:@"gamecontroller"
                         accessibilityDescription:@"Disconnected"];
        if (@available(macOS 12.0, *)) {
            NSImageSymbolConfiguration *config = [NSImageSymbolConfiguration configurationWithPaletteColors:@[[NSColor systemGrayColor]]];
            icon = [icon imageWithSymbolConfiguration:config];
        }
    }

    // Fallback if symbol not available
    if (!icon) {
        icon = [[NSImage alloc] initWithSize:NSMakeSize(18, 18)];
        [icon lockFocus];
        [[NSColor labelColor] set];
        NSBezierPath *path = [NSBezierPath bezierPathWithOvalInRect:NSMakeRect(2, 2, 14, 14)];
        if (self.isConnected) {
            [[NSColor systemGreenColor] setFill];
        } else {
            [[NSColor systemGrayColor] setFill];
        }
        [path fill];
        [icon unlockFocus];
    }

    [icon setTemplate:NO];
    self.statusItem.button.image = icon;
}

- (void)selectOutputMode:(NSMenuItem *)sender {
    DriverContext *ctx = (DriverContext *)self.context;
    if (!ctx) return;

    // Edit the file, not the driver - hot-reload applies it
    ControllerMapping mapping;
    if (config_load(ctx->config_path, &mapping) != 0) {
        mapping = ctx->config;
    }
    mapping.output_mode = (OutputMode)sender.tag;
    config_save(ctx->config_path, &mapping);
}

- (void)openSettings:(id)sender {
    (void)sender;
    settings_show(self.context);
}

- (void)openConfigFile:(id)sender {
    (void)sender;
    DriverContext *ctx = (DriverContext *)self.context;
    if (!ctx) return;
    NSString *path = [NSString stringWithUTF8String:ctx->config_path];
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:path]];
}

- (void)reloadConfig:(id)sender {
    (void)sender;
    if (self.reloadCallback) {
        self.reloadCallback(self.context);
    }
}

- (void)showAbout:(id)sender {
    (void)sender;

    // Bring app to front before showing modal dialog
    [NSApp activateIgnoringOtherApps:YES];

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Xbox Controller Driver"];
    [alert setInformativeText:@"A macOS driver for Xbox One controllers.\n\nTranslates controller input to keyboard/mouse events, MIDI, or OSC.\n\nVersion 2.1"];
    [alert setAlertStyle:NSAlertStyleInformational];
    [alert addButtonWithTitle:@"OK"];

    // Run modal on main thread
    dispatch_async(dispatch_get_main_queue(), ^{
        [alert runModal];
    });
}

- (void)quitApp:(id)sender {
    (void)sender;
    if (self.quitCallback) {
        self.quitCallback(self.context);
    }
    [NSApp terminate:nil];
}

- (void)setStatusText:(NSString *)status {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.statusMenuItem setTitle:[NSString stringWithFormat:@"Status: %@", status]];
    });
}

- (void)setConnectedState:(BOOL)connected {
    dispatch_async(dispatch_get_main_queue(), ^{
        self.isConnected = connected;
        [self updateIcon];
    });
}

// Escape a string for embedding inside an AppleScript double-quoted literal
- (NSString *)appleScriptEscape:(NSString *)s {
    NSString *out = [s stringByReplacingOccurrencesOfString:@"\\" withString:@"\\\\"];
    return [out stringByReplacingOccurrencesOfString:@"\"" withString:@"\\\""];
}

- (void)promptAdminRelaunch {
    DriverContext *ctx = (DriverContext *)self.context;
    [NSApp activateIgnoringOtherApps:YES];

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Administrator Privileges Needed"];
    [alert setInformativeText:@"macOS holds the controller's USB interface exclusively. "
                              @"The driver needs administrator privileges to claim it.\n\n"
                              @"Relaunch now? You'll be asked for your password once."];
    [alert setAlertStyle:NSAlertStyleWarning];
    [alert addButtonWithTitle:@"Relaunch as Admin"];
    [alert addButtonWithTitle:@"Quit"];

    if ([alert runModal] != NSAlertFirstButtonReturn) {
        [NSApp terminate:nil];
        return;
    }

    NSString *exe = [[NSBundle mainBundle] executablePath];
    NSString *cfg = ctx ? [NSString stringWithUTF8String:ctx->config_path] : @"";
    NSString *logPath = [NSHomeDirectory() stringByAppendingPathComponent:
                         @"Library/Logs/xbox-controller-driver.log"];

    // `launchctl asuser` keeps the elevated process inside the user's GUI
    // session. Without it the root process lands in launchd's context with no
    // WindowServer connection and dies before the menu bar item appears.
    NSString *shellCmd = [NSString stringWithFormat:
        @"/bin/launchctl asuser %u '%@' --config '%@' >>'%@' 2>&1 &",
        getuid(), exe, cfg, logPath];
    NSString *script = [NSString stringWithFormat:@"do shell script \"%@\" with administrator privileges",
                        [self appleScriptEscape:shellCmd]];

    NSTask *task = [[NSTask alloc] init];
    task.executableURL = [NSURL fileURLWithPath:@"/usr/bin/osascript"];
    task.arguments = @[@"-e", script];
    task.terminationHandler = ^(NSTask *t) {
        if (t.terminationStatus != 0) {
            // Password dialog cancelled: keep this instance alive (menu and
            // settings still work; there's just no controller input)
            return;
        }
        // Give the elevated instance a moment, then retire this one only if it
        // actually came up - otherwise the user would be left with nothing
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(3 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
            if ([self elevatedInstanceRunning:exe]) {
                [NSApp terminate:nil];
            } else {
                [self reportRelaunchFailure:logPath];
            }
        });
    };

    NSError *error = nil;
    if (![task launchAndReturnError:&error]) {
        NSLog(@"Failed to run osascript: %@", error);
    }
}

// True if a process other than us is running the same executable
- (BOOL)elevatedInstanceRunning:(NSString *)exePath {
    NSTask *pgrep = [[NSTask alloc] init];
    pgrep.executableURL = [NSURL fileURLWithPath:@"/usr/bin/pgrep"];
    pgrep.arguments = @[@"-f", exePath];
    NSPipe *pipe = [NSPipe pipe];
    pgrep.standardOutput = pipe;
    pgrep.standardError = [NSPipe pipe];

    NSError *error = nil;
    if (![pgrep launchAndReturnError:&error]) return NO;
    NSData *data = [pipe.fileHandleForReading readDataToEndOfFile];
    [pgrep waitUntilExit];

    NSString *out = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    pid_t self_pid = getpid();
    for (NSString *line in [out componentsSeparatedByString:@"\n"]) {
        NSString *trimmed = [line stringByTrimmingCharactersInSet:
                             [NSCharacterSet whitespaceCharacterSet]];
        if (trimmed.length && (pid_t)trimmed.intValue != self_pid) return YES;
    }
    return NO;
}

- (void)reportRelaunchFailure:(NSString *)logPath {
    menubar_set_status("Relaunch failed - see log");

    NSAlert *alert = [[NSAlert alloc] init];
    [alert setMessageText:@"Relaunch Did Not Start"];
    [alert setInformativeText:[NSString stringWithFormat:
        @"The elevated instance failed to start. Details may be in:\n%@\n\n"
        @"You can still run it from a terminal with:\nsudo '%@'",
        logPath, [[NSBundle mainBundle] executablePath]]];
    [alert setAlertStyle:NSAlertStyleWarning];
    [alert addButtonWithTitle:@"OK"];
    [alert runModal];
}

@end

/*******************************************************************************
 * Global State
 ******************************************************************************/
static AppDelegate *g_appDelegate = nil;

/*******************************************************************************
 * C Interface Implementation
 ******************************************************************************/
void menubar_init(void *context, MenuBarReloadCallback reload_cb, MenuBarQuitCallback quit_cb) {
    @autoreleasepool {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];

        g_appDelegate = [[AppDelegate alloc] init];
        g_appDelegate.context = context;
        g_appDelegate.reloadCallback = reload_cb;
        g_appDelegate.quitCallback = quit_cb;
        g_appDelegate.isConnected = NO;

        [NSApp setDelegate:g_appDelegate];
    }
}

void menubar_set_status(const char *status) {
    if (g_appDelegate && status) {
        @autoreleasepool {
            NSString *statusStr = [NSString stringWithUTF8String:status];
            [g_appDelegate setStatusText:statusStr];
        }
    }
}

void menubar_set_connected(bool connected) {
    if (g_appDelegate) {
        [g_appDelegate setConnectedState:connected ? YES : NO];
    }
}

void menubar_prompt_admin_relaunch(void) {
    if (g_appDelegate) {
        dispatch_async(dispatch_get_main_queue(), ^{
            [g_appDelegate promptAdminRelaunch];
        });
    }
}

void menubar_run(void) {
    @autoreleasepool {
        [NSApp run];
    }
}

void menubar_quit(void) {
    dispatch_async(dispatch_get_main_queue(), ^{
        [NSApp terminate:nil];
    });
}
