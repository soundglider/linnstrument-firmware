# Build and Flash

How to build the firmware, flash it to your LinnStrument 128, and
recover if something goes wrong.

For what's in the firmware, see [README.md](../README.md) and
[ARCHITECTURE.md](ARCHITECTURE.md). For the phase-by-phase implementation
order, see [ROADMAP.md](ROADMAP.md).

## Hardware

- **LinnStrument 128** — the 8-row × 16-column variant. (200 variant
  shares this firmware via runtime detection, but the layout in
  [LAYOUT.md](LAYOUT.md) assumes 128.)
- **USB cable** plugged into the LinnStrument's **native USB port**
  (the silver one, **not** the programming port). Flashing the
  programming port requires `bossac` and is not the documented path
  here.

## Toolchain

| Tool                    | Version    | Why                                        |
|-------------------------|------------|--------------------------------------------|
| Arduino IDE             | **1.8.x**  | Stock firmware's documented version. The 2.x IDE has not been fully validated against the LinnStrument firmware tree. |
| Arduino SAM Boards pkg  | **1.6.11** | The version the stock firmware compiles against. Newer SAM packages may emit warnings or change linker behavior. |

Both are installed via Arduino IDE's **Tools → Board → Boards Manager**.
Pin both versions on whatever machine you develop on. Mac, Windows, or
Linux all work; document any quirks of your system in
[ROADMAP.md § Phase 1](ROADMAP.md#phase-1--toolchain--stock-flash).

## First-time setup

1. Install Arduino IDE 1.8.x from
   <https://www.arduino.cc/en/software> (the "Legacy IDE 1.8.X"
   download). Do **not** install IDE 2.x for this project.
2. Open the IDE, then **Tools → Board → Boards Manager…**.
3. Search "SAM". Install **Arduino SAM Boards (32-bits ARM Cortex-M3)**
   version **1.6.11**. If a newer version installed automatically, use
   the version dropdown to downgrade.
4. **Tools → Board → Arduino ARM (32-bits) Boards → Arduino Due
   (Native USB Port)**.
5. Plug in the LinnStrument via its native USB port.
6. **Tools → Port →** the port that just appeared (named "Arduino Due"
   or similar; on macOS / Linux it's a `/dev/tty.usbmodem*` device, on
   Windows a `COM*` port).

The driver setup on Windows may require the bundled driver — see
[`LinnStrument Windows Driver.zip`](../LinnStrument%20Windows%20Driver.zip)
at the repo root.

## Building the firmware

The sketch is `linnstrument-firmware.ino` at the repo root. Arduino IDE
compiles every `.ino` file in the sketch's directory together, so all
`ls_*.ino` files (stock and our additions) link automatically.

1. **File → Open** → `linnstrument-firmware.ino` from your local clone.
2. Confirm board / port (above).
3. **Sketch → Verify/Compile** (Ctrl + R). First compile takes ~30–60 s.
4. If it compiles clean, **Sketch → Upload** (Ctrl + U). The IDE
   resets the Due, uploads via `bossac`, and verifies. Takes ~15 s.
5. The LinnStrument reboots into the new firmware automatically.

If verify fails, the error is almost always in `ls_chord_*` files —
the stock firmware compiles untouched.

## Verifying a flash

After upload, the LinnStrument re-initializes. Quick smoke tests
depending on what's been implemented:

- **Phase 1 (stock baseline):** the device behaves like a normal
  LinnStrument — playing cells produces standard MIDI notes on the
  default channels.
- **Phase 4+ (chord trigger in):** see
  [ROADMAP.md § Verification at each phase](ROADMAP.md#verification-at-each-phase).

## Serial debug

`linnstrument-firmware.ino`'s `setup()` enables a USB-serial console at
115200 baud. Our additions write one line per significant event (touch
dispatched into zone X at cell Y, chord template Z resolved, etc.).

To read:

- **Arduino IDE:** **Tools → Serial Monitor**. Set baud rate to
  **115200**.
- **From a terminal:** any USB-serial reader (`screen /dev/tty.usbmodem* 115200`
  on macOS/Linux, PuTTY on Windows) works.

The serial console does **not** stream MIDI — that goes out the
native USB port's MIDI endpoint. Use a separate MIDI monitor (MIDI-OX,
Pocket MIDI, mido CLI) to inspect the MIDI stream.

## Reverting to stock firmware

Roger Linn Design distributes signed binaries of the stock firmware at
<https://www.rogerlinndesign.com/support>. Reverting is the path we
practise once during Phase 1 so it's known to work:

1. Download the matching stock binary for your LinnStrument (128 or 200).
2. In Arduino IDE: **Tools → LinnStrument → Upload Linn Firmware…**
   (the stock firmware tree ships an LinnStrument-specific menu hook
   that handles this; if absent on a fresh checkout, use `bossac` with
   the binary directly).
3. Flash. The device reverts to the version you uploaded.

Keep one signed stock binary checked into a `firmware-recovery/`
directory (or somewhere outside the repo if you'd rather not version
the binary) so reverting works even without internet.

## Reproducibility

The single source of truth for the firmware's behavior is the C++ /
Arduino-IDE artefact at the repo root. There are no other generated
files, no codegen, no preprocessing steps. A clean clone + matching
toolchain reproduces the same binary byte-for-byte.

If you're capturing builds for distribution, the Arduino IDE 1.8 menu
**Sketch → Export compiled Binary** writes a `.bin` next to the sketch
that can be flashed directly with `bossac` on any machine, without
re-installing the IDE.
