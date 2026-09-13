# ✈ SkyWatch

**Live ADS-B aircraft tracker for the Waveshare ESP32-S3-Knob-Touch-LCD-1.8.**

Track every plane within 10 km of your home in real time — radar sweep, alert screen, airline badge list, and real aircraft photos pulled live from Planespotters.net.

---

## Hardware

| Item | Detail |
|------|--------|
| Board | Waveshare ESP32-S3-Knob-Touch-LCD-1.8 |
| Chip | ESP32-S3 (QFN56), 8 MB embedded octal PSRAM |
| Flash | **16 MB**, quad I/O — confirmed with `esptool flash_id` against the physical device, not assumed |
| Display | ST7789 IPS 240×280 SPI (LovyanGFX) |
| Input | Rotary encoder + push button |
| Storage | LittleFS on a custom-partitioned ~11.9 MB data partition — see [Flash layout](#flash-layout) |

---

## Features

- **Radar sweep** — animated arm, blips coloured by distance (red < 2 km / amber < 4 km / green)
- **Alert screen** — fires automatically when a plane enters your alert radius
- **Flight list** — scroll all nearby flights with knob rotation
- **Detail screen** — real aircraft photo (Planespotters.net) + full data grid
- **Custom screensaver** — upload your own JPEG logo via the web portal
- **Night mode** — auto-dims to 10 % after 10 minutes of inactivity
- **Config portal** — hold knob 5 s → AP `SkyWatch-Setup` → browser setup page
- **Personal records** — closest approach, fastest, highest (NVS, survives reflash)

---

## Project structure

The firmware is split into one folder per module. Every module follows the
same shape, so it's obvious where to add or change functionality:

```
<module>/
├── inc/   # public headers for the module (#include "xxx.h" from anywhere)
└── src/   # implementation (.cpp) — omitted for header-only modules
```

```
SkyWatch/
├── SkyWatch.ino            # Main sketch — setup() + loop() + state machine
├── Makefile                 # Build/flash via arduino-cli (see below)
├── config/inc/               # Pins, colours, constants, ScreenState enum (header-only)
├── flight/                   # Flight struct + shared flight-list storage
│   ├── inc/flight.h
│   └── src/flight.cpp
├── airline/                  # Airline DB (27 carriers) + badge colour lookup
│   ├── inc/airlines.h
│   └── src/airlines.cpp
├── display/                  # LovyanGFX LGFX class + draw helpers
│   ├── inc/display.h
│   └── src/display.cpp
├── screen/                   # Full-screen renderers (radar/alert/list/detail)
│   ├── inc/screens.h
│   └── src/screens.cpp
├── haversine/                 # Great-circle distance + ETA math
│   ├── inc/haversine.h
│   └── src/haversine.cpp
├── wifi_manager/              # WiFi connect + captive config portal
│   ├── inc/wifi_manager.h
│   └── src/wifi_manager.cpp
├── opensky/                   # OpenSky API fetch + parse + distance filter
│   ├── inc/opensky.h
│   └── src/opensky.cpp
├── records/                   # NVS-backed personal records
│   ├── inc/records.h
│   └── src/records.cpp
├── aircraft_photo/            # Planespotters API + JPEG fetch/draw
│   ├── inc/aircraft_photo.h
│   └── src/aircraft_photo.cpp
├── knob/                      # Rotary encoder + debounced button
│   ├── inc/knob.h
│   └── src/knob.cpp
├── screensaver/                # LittleFS logo or built-in branding
│   ├── inc/screensaver.h
│   └── src/screensaver.cpp
└── data/
    ├── config.json            # Default config (uploaded via LittleFS)
    └── logo.jpg                # Screensaver logo (optional — see below)
```

`config`, `flight`, `airline`, and `haversine` were header-only before this
split; `flight`, `airline`, and `haversine` now have real `.cpp`
implementations (this also fixed a latent bug — `g_flights` / `g_flight_count`
/ `g_selected_idx` / `g_closest_idx` were declared `extern` but never
actually defined anywhere, which would have failed at link time under any
build that isn't the Arduino IDE's single-folder sketch model).

**Adding a new module:** create `<name>/inc/<name>.h` and
`<name>/src/<name>.cpp`, then add `<name>` to the `MODULES` list at the top
of the [Makefile](Makefile). `#include "<name>.h"` works from any other
module without a relative path — the Makefile adds every module's `inc/` to
the compiler's include path.

---

## Building

### Option A — Makefile (recommended for forking)

The Makefile wraps [`arduino-cli`](https://arduino.github.io/arduino-cli/latest/installation/)
and assembles the module tree into a buildable sketch automatically — no
manual include-path setup needed.

```bash
# 1. Install arduino-cli, then one-time setup:
make libs        # installs the ESP32 core + LovyanGFX + ArduinoJson

# 2. Compile:
make build        # → build/output/SkyWatch.ino.bin

# 3. Flash firmware:
make upload PORT=/dev/ttyUSB0     # macOS: /dev/cu.usbmodemXXXX

# 4. Flash the LittleFS data/ image (default config.json, logo, etc.):
make fs PORT=/dev/ttyUSB0

# 5. Serial monitor:
make monitor PORT=/dev/ttyUSB0
```

Run `make help` for the full target list. The board FQBN (flash mode/size,
partition scheme, PSRAM, upload speed) and the LittleFS partition
offset/size are variables at the top of the [Makefile](Makefile) — check
them against your installed ESP32 core with
`arduino-cli board details -b esp32:esp32:esp32s3 --full` before your first
flash, since option keys can shift between core versions. See
[Flash layout](#flash-layout) below for why these specific values.

### Option B — Arduino IDE

The IDE only auto-compiles files that sit directly in the sketch folder, so
it won't see the `<module>/src/*.cpp` files on its own. Either:

- Use **arduino-cli** (Option A) instead, or
- Point the IDE at extra include paths yourself via
  `File → Preferences → compiler.cpp.extra_flags` (or a `platform.local.txt`
  override) set to the same `-I` list the Makefile builds from each
  module's `inc/` folder.

Required libraries (install via Arduino Library Manager either way):

| Library | Version tested |
|---------|---------------|
| **LovyanGFX** | 1.x |
| **ArduinoJson** | 7.x |
| ESP32 Arduino core | 3.x (includes WiFi, WebServer, LittleFS, HTTPClient, Preferences) |

Board: **ESP32S3 Dev Module** (or Waveshare ESP32-S3) · Flash mode **QIO
80MHz** · Flash size **16MB** · Partition Scheme **Custom** (picks up
[`partitions.csv`](partitions.csv) automatically since it sits next to
`SkyWatch.ino`) · PSRAM **OPI PSRAM** · Upload speed **921600**.

---

## Flash layout

The board's flash was originally assumed to be 4 MB (the ESP32 Arduino
core's own board default) — that was wrong. Reading it directly off the
physical device with `esptool flash_id` confirmed **16 MB**, quad-I/O, with
8 MB of embedded octal PSRAM. Building against the unconfigured 4 MB
default meant the firmware was sitting at 88–91% of a ~1.3 MB app
partition that didn't actually exist on the chip — false pressure, not a
real constraint.

[`partitions.csv`](partitions.csv) is a custom table sized for the real 16 MB
and for what SkyWatch actually needs — not one of the ESP32 core's bundled
16 MB presets, since all of them either reserve a second OTA app slot
SkyWatch has no code path to ever write to (halving usable space for
nothing) or label the data partition `fat` instead of `spiffs` (the label
the Arduino `LittleFS` library looks for by default — a `fat`-labeled
partition means `LittleFS.begin()` silently fails to find it):

| Partition | Offset | Size | Purpose |
|---|---|---|---|
| `nvs` | `0x9000` | 20 KB | WiFi credentials / `Preferences` storage |
| `otadata` | `0xe000` | 8 KB | Kept for bootloader compatibility (see below) |
| `app0` | `0x10000` | **4 MB** | Firmware — current build uses ~1.16 MB (~28%) |
| `spiffs` | `0x410000` | **~11.9 MB** | LittleFS — `config.json`, `logo.jpg`, future data |
| `coredump` | `0xff0000` | 64 KB | Crash dumps |

No second OTA slot: SkyWatch has no OTA-update code, so mirroring the app
image in a second slot would only waste ~4 MB for a feature that doesn't
exist. This follows the ESP32 core's own `no_ota.csv` convention — a single
`ota_0` slot plus `otadata`, rather than a `factory`-type slot — so the
bootloader's boot-slot-selection logic still behaves exactly as it does on
every other ESP32 Arduino board.

If you flash the factory-original firmware back (see
[`SkyWatch_factory_backup/`](../SkyWatch_factory_backup) — a sibling
folder, not part of this repo), you overwrite this table with the factory
one (dual 3 MB OTA slots + a ~10 MB SPIFFS partition at a *different*
offset) — the two aren't compatible, don't mix a SkyWatch app build with
the factory partition table or vice versa. Re-flashing SkyWatch's own
`make build`/`make upload` always writes its own table back out.

---

## Screensaver logo

[`data/logo.jpg`](data/logo.jpg) is shown on the screensaver instead of the
built-in "SkyWatch" branding whenever it's present on the device's LittleFS
(see [screensaver/src/screensaver.cpp](screensaver/src/screensaver.cpp)).
It ships to the device as part of the LittleFS image — `make fs` (or the
Arduino IDE's LittleFS Data Upload tool) flashes it alongside `config.json`.

`screensaver.cpp` draws the JPEG at its native pixel size, centered, with
**no automatic scale-to-fit** — so the source image must already be sized
to the 240×280 panel before it's converted. To swap in a different logo:

```bash
sips -Z 240 -s format jpeg -s formatOptions 85 your-logo.png --out data/logo.jpg
```

`-Z 240` scales the longer edge to 240 px, preserving aspect ratio — a
landscape logo ends up centered with black letterboxing top/bottom; a
portrait one fills closer to the full 240×280. Keep it under ~200 KB (the
size `screensaverDraw()` checks before decoding).

Owners can also replace it later without reflashing firmware, via the
config portal's **Custom Screensaver Logo** upload (writes to `/logo.jpg`
on LittleFS directly).

---

## Configure via web portal

On first boot (or when WiFi credentials are missing) the screensaver shows automatically.

**To open the setup portal:**

1. Hold the knob button for **5 seconds** on the radar or screensaver screen.
2. The display shows **"SkyWatch-Setup"** AP details.
3. Connect your phone/laptop to the `SkyWatch-Setup` WiFi (password: `skywatch1`).
4. Open `http://192.168.4.1` in a browser.
5. Fill in your WiFi credentials, location name, home coordinates, and scan radius.
6. Optionally upload a custom JPEG screensaver logo.
7. Press **Save & Connect** — the device restarts and connects.

**Finding your coordinates:** Google Maps → right-click your home → copy the lat/lon shown.

---

## Knob controls

| Action | Result |
|--------|--------|
| Rotate | Scroll flight list (from any screen) |
| Short press on screensaver/radar | Open flight list |
| Short press on list | Open detail screen |
| Short press on detail | Back to list |
| Short press on alert | Open detail |
| **Long press (5 s) on radar/screensaver** | **Open config portal** |
| Long press on list/alert/detail | Back to radar |

---

## Data sources

| Source | API | Auth |
|--------|-----|------|
| [OpenSky Network](https://opensky-network.org) | `api/states/all?lamin=…` | None (free) |
| [Planespotters.net](https://www.planespotters.net) | `api.planespotters.net/pub/photos/hex/{icao24}` | None (free) |

OpenSky is polled every **10 seconds**. Rate limit (anonymous): 400 requests/day — well within limits at this interval.

---

## Customising for a new customer

1. Flash the same firmware binary (no recompile needed).
2. Upload LittleFS data once (or leave as-is — portal creates `config.json`).
3. Hand the device to the customer — they configure WiFi + coordinates via the portal.
4. Customer uploads their own screensaver logo via the portal.

All per-device data lives in LittleFS and NVS, completely separate from flash firmware.

---

## Licence

MIT — do whatever you like, no warranty.
