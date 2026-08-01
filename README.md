# Xbox One Controller Driver for macOS

A userspace driver that translates Xbox One controller input to keyboard/mouse events, MIDI messages, or OSC over UDP. Works system-wide in any application.

## Features

- Menu bar app with connection status
- **MIDI mode**: the controller appears as a virtual MIDI device ("Xbox Controller") in any DAW — sticks and triggers send continuous CCs, buttons send notes
- **OSC mode**: streams sticks/triggers as floats (full 16-bit resolution) and buttons as ints over UDP — for Max/MSP, Max for Live, SuperCollider, TouchDesigner, etc. Can run simultaneously with MIDI mode
- JSON configuration with hot-reload (no rebuild needed)
- Automatic reconnection when controller is unplugged/replugged
- Rumble feedback on button press
- Customizable button mappings, stick modes, mouse sensitivity

## Default Mapping

- Left stick: WASD keys
- Right stick: Mouse movement
- Triggers: Mouse clicks (left/right)
- A/B/X/Y: Space, C, R, F
- Bumpers: Q, E
- D-pad: Arrow keys

## Requirements

- macOS (tested on Tahoe 26.1)
- Xbox One controller with USB cable (Model 1697 confirmed)
- Homebrew with libusb and pkg-config

## Installation

```bash
brew install libusb pkg-config
make app
open "Xbox Controller Driver.app"
```

That builds a double-clickable menu-bar app — no terminal needed. macOS holds the
controller's USB interface exclusively, so on launch the app asks for your password
once and relaunches itself with the privileges needed to claim it.

Prefer the terminal? `make simulator && sudo ./simulator` still works, as does the
CLI-only `make simulator-cli`.

Accessibility permissions are only needed for keyboard/mouse mode (System Settings >
Privacy & Security > Accessibility). MIDI and OSC modes need no permissions.

## Menu Bar

Click the controller icon for:

- **Output Mode** — switch between Keyboard/Mouse, MIDI, OSC, and MIDI+OSC
- **Settings…** — edit the common options in a window (output mode, MIDI channel /
  velocity / invert-Y / device name, OSC host / port / address prefix, stick deadzone,
  rumble). Saving writes the config file; the driver picks it up within a second
- **Open Config File** — opens the JSON for per-button notes, CCs, and OSC addresses
- **Reload Configuration** — force an immediate reload

The icon turns green when the controller is connected.

## Configuration

The settings window and the JSON file are two views of the same thing — the file is
the source of truth, and hand-edits and UI edits coexist (saving from the window
preserves fields it doesn't manage). Changes are picked up automatically while running.

On first run the app writes `~/.config/xbox-controller/config.json`. You can also pass
`--config <path>`, or keep a `./controller.json` next to the binary; the bundled
examples in `config/` are good starting points.

```json
{
  "buttons": { "a": "space", "b": "c", "x": "r", "y": "f" },
  "left_stick": { "mode": "wasd", "deadzone": 8000 },
  "right_stick": { "mode": "mouse", "sensitivity": 1.5, "curve": 1.8 },
  "triggers": { "left": "mouse_left", "right": "mouse_right" }
}
```

You can also place your config at `~/.config/xbox-controller/config.json`.

Key names: letters (a-z), numbers (0-9), space, tab, escape, return, left_shift, left_control, up, down, left, right, f1-f12, mouse_left, mouse_right.

## Build Targets

```bash
make app             # Double-clickable menu bar app (recommended)
make simulator       # Menu bar binary, run from terminal with sudo
make simulator-cli   # CLI-only version (no menu bar)
make test            # Run unit tests
make clean           # Remove build artifacts
```

## MIDI Mode (use the controller in Ableton Live or any DAW)

Set `"output_mode": "midi"` in your config, or run with the bundled example:

```bash
sudo ./simulator --config config/controller-midi.json
```

The driver creates a virtual MIDI source named **Xbox Controller** (no Accessibility permission needed in this mode). Default mapping:

| Control | MIDI message |
|---|---|
| Left stick X/Y | CC 20 / CC 21 |
| Right stick X/Y | CC 22 / CC 23 |
| Left / right trigger | CC 24 / CC 25 (continuous 0-127) |
| A, B, X, Y | Notes 36-39 |
| LB, RB, View, Menu | Notes 40-43 |
| Stick clicks (LS, RS) | Notes 44-45 |
| D-pad | Notes 46-49 |

Stick axes send 0-127 with center at 64. Everything is remappable in the `midi` section of the config (`channel` 1-16, `velocity`, `invert_y`, per-button `notes`, per-axis `ccs`) — hot-reload applies changes while running. The one exception is `device_name`: the virtual source is created once at startup, so renaming it requires a restart.

**In Ableton Live:** Settings → Link, Tempo & MIDI → MIDI Ports → enable **Remote** (for MIDI-mapping knobs/faders) and **Track** (for playing instruments) on the *Xbox Controller* input. Then use MIDI Map Mode (Cmd+M), move a stick or press a button, and click the parameter you want it mapped to.

**MIDI source doesn't appear in the DAW?** If you run with `sudo` from a normal terminal it inherits your login session and works. If it still doesn't show up, try running without `sudo` — USB access sometimes works without it.

## OSC Mode (Max/MSP, SuperCollider, TouchDesigner...)

Set `"output_mode": "osc"` (or `"midi+osc"` to run both outputs at once), or use the bundled examples:

```bash
sudo ./simulator --config config/controller-osc.json       # OSC only
sudo ./simulator --config config/controller-midi-osc.json  # MIDI + OSC together
```

Messages are sent over UDP (default `127.0.0.1:9000`), one message per control change, only when values change:

| Address | Arguments |
|---|---|
| `/xbox/stick/left`, `/xbox/stick/right` | two floats: x, y in -1..1 (full 16-bit resolution, deadzone applied, up/right positive) |
| `/xbox/trigger/left`, `/xbox/trigger/right` | one float: 0..1 |
| `/xbox/button/a` ... `/xbox/button/menu` | one int: 0 or 1 |
| `/xbox/dpad/up` ... `/xbox/dpad/right` | one int: 0 or 1 |

Host, port, and every address are configurable in the `osc` section (hot-reload applies changes while running). `prefix` renames the whole namespace in one line; entries under `addresses` override individual controls and win over the prefix:

```json
{
  "output_mode": "osc",
  "osc": {
    "host": "127.0.0.1",
    "port": 9000,
    "prefix": "/pad",
    "addresses": { "left_stick": "/mysynth/xy" }
  }
}
```

**In Max / Max for Live:** `[udpreceive 9000]` → `[route /xbox/stick/left /xbox/trigger/left ...]` → `[unpack f f]` for sticks. On the first controller packet the driver sends the complete state (all buttons, sticks, triggers) so your patch starts in sync.

Note: the Xbox One controller has no gyroscope or accelerometer — sticks, triggers, and buttons are everything the hardware reports.

## Streaming Mode

For game streaming apps like Moonlight or Parsec, enable streaming mode in your config:

```json
{
  "advanced": { "streaming_mode": true }
}
```

## Troubleshooting

**Keys not working:** Add your terminal to Accessibility permissions in System Settings.

**Stick drift:** Increase the deadzone value in your config (default 8000, about 24%).

**Mouse too fast/slow:** Adjust sensitivity (default 1.5, higher = faster).

**Controller not found:** Make sure it's plugged in via USB. The app keeps polling, so
you can plug it in after launching — the icon turns green when it connects.

**"Administrator privileges needed":** Expected — macOS claims the controller's USB
interface, so the driver needs root to take it. Click Relaunch as Admin. You'll briefly
see two menu bar icons while the new instance starts; the old one exits on its own.

**Relaunch didn't start:** Check `~/Library/Logs/xbox-controller-driver.log`. You can
always fall back to `sudo ./simulator` in a terminal.

## Limitations

- Only tested with Xbox One Model 1697
- Simulates keyboard/mouse, not a virtual gamepad
- Some games that require a real controller won't work

## How It Works

The driver communicates with the controller via libusb using Microsoft's GIP protocol, then injects keyboard/mouse events through the macOS Accessibility API. This approach works because macOS blocks virtual HID device creation but allows event injection for assistive technology.

## License

MIT
