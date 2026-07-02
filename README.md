# YouTube Stats Counter

An ESP32-based display that shows projected channel stats from a configurable JSON endpoint on a MAX7219 LED matrix. No IDE required after the first flash — configure everything through a built-in web portal and push firmware updates over Wi-Fi.

<div align="center">
  <img src="https://raw.githubusercontent.com/justinmahar/YouTube-Subscriber-Counter/refs/heads/main/Images/Thumbnail2.png" width="560" alt="YouTube Subscriber Counter V2.0">
</div>

---

## ⚡ [Install via browser](https://justinmahar.github.io/YouTube-Subscriber-Counter/) — no IDE needed

---

## Features

- Shows subscribers, views, watch hours, or any combination on a 4-module MAX7219 matrix
- Projects displayed values every second from per-minute growth rates between API refreshes
- **Captive portal setup** — on first boot the ESP32 broadcasts a hotspot; connect and configure everything from your phone or browser, no code changes needed
- **Persistent config** — credentials saved to NVS flash, survive reboots and firmware updates
- **Wi-Fi scanner** — scan nearby networks and tap to auto-fill the SSID field
- **IP display on boot** — scrolls your local IP across the matrix for 5 seconds so you always know where to reach the config page
- **In-browser reconfigure** — visit the device IP any time to change Wi-Fi or stats endpoint settings; leave the Wi-Fi fields blank to keep saved values
- **OTA firmware updates** — upload a compiled `.bin` straight from the config page, no USB cable needed after first flash
- **Milestone animations** — celebratory LED animations when projected stats cross round-number thresholds (see [Milestones](#milestones))
- **Holiday easter eggs** — themed animations and scrolling greetings on holidays and special dates (see [Holidays](#holidays))

---

## Milestones

When a stat crosses a milestone threshold, the matrix plays a dedicated animation before returning to the normal display. Each metric has its own set of animations so subscribers, views, and watch hours feel distinct at every tier.

**19 animations total** — 3 metrics × 6 tiers, plus a views-only +100M apex tier.

### Tier thresholds

Subscribers and watch hours use these boundaries:

| Tier | Interval   | Example values      |
| ---- | ---------- | ------------------- |
| 1    | 100        | 100, 200, 300, …    |
| 2    | 1,000      | 1K, 2K, 3K, …       |
| 3    | 10,000     | 10K, 20K, 30K, …    |
| 4    | 100,000    | 100K, 200K, 300K, … |
| 5    | 1,000,000  | 1M, 2M, 3M, …       |
| 6    | 10,000,000 | 10M, 20M, 30M, …    |

**Views** use the same boundaries for tiers 1–6 but animations start at tier 3 (+10K). Crossings at +100 and +1K views do not play a milestone animation. Views also have an apex tier at **+100M** (100M, 200M, …) — subscribers and watch hours do not.

Tier 1 fires on every 100; higher tiers fire on their own boundaries (e.g. 1,000, 10,000, …). When multiple tiers align (e.g. 1,000 subs), the highest matching tier animation plays.

### Animation inventory

| #   | Metric      | Tier | Threshold | Status                                            |
| --- | ----------- | ---- | --------- | ------------------------------------------------- |
| 1   | Subscribers | 1    | 100       | Done (border chase)                               |
| 2   | Subscribers | 2    | 1K        | Done (ripple + rain)                              |
| 3   | Subscribers | 3    | 10K       | Done (epic multi-phase)                           |
| 4   | Subscribers | 4    | 100K      | Done (fireworks + explosions)                     |
| 5   | Subscribers | 5    | 1M        | Done (trinity blast + firestorm)                  |
| 6   | Subscribers | 6    | 10M       | Done (ultimate finale)                            |
| 7   | Views       | 1    | 100       | — (no animation)                                  |
| 8   | Views       | 2    | 1K        | — (no animation)                                  |
| 9   | Views       | 3    | 10K       | Done (two eyes open + blink)                      |
| 10  | Views       | 4    | 100K      | Done (dilated eyes + view rain)                   |
| 11  | Views       | 5    | 1M        | Done (crowd eyes → giant stare + deluge)          |
| 12  | Views       | 6    | 10M       | Done (gravity well + nova grid + plasma)          |
| 13  | Views       | 7    | 100M      | Done (twin singularity + omniscience + hypernova) |
| 14  | Watch hours | 1    | 100       | Done (hourglass drains top → bottom)              |
| 15  | Watch hours | 2    | 1K        | Done (sand stream + tick sweep + ripple)          |
| 16  | Watch hours | 3    | 10K       | Done (twin hourglasses build, sync drain, blasts) |
| 17  | Watch hours | 4    | 100K      | Done (triple hourglasses + megablast finale)      |
| 18  | Watch hours | 5    | 1M        | Done (triple drain + firework barrage)            |
| 19  | Watch hours | 6    | 10M       | Done (ultimate fullscreen + twin barrages)        |

Animations trigger off projected stat values between API refreshes.

---

## Holidays

On matching calendar dates (America/Eastern), the matrix plays a themed animation and scrolls a short greeting. Animations repeat every 5 minutes while the holiday is active.

The device does not use NTP. It seeds its clock from `serverTimeUnix` in each stats API response, then advances locally between refreshes — the same approach used for stat projection.

### Supported dates

| Date | Holiday |
| ---- | ------- |
| Jan 1 | New Year's Day |
| Feb 2 | Groundhog Day |
| Feb 14 | Valentine's Day |
| 3rd Mon in Feb | Presidents Day |
| Feb 29 | Leap Day |
| Mar 14 | Pi Day |
| Mar 17 | St. Patrick's Day |
| Easter Sunday | Easter (computed) |
| Apr 1 | April Fools' Day |
| Apr 22 | Earth Day |
| May 5 | Cinco de Mayo |
| Last Mon in May | Memorial Day |
| 3rd Sun in Jun | Father's Day |
| Jul 4 | Independence Day |
| Aug 8 | Creator Cat Day |
| Aug 12 | Justin's birthday |
| Sep 12 | Dad's birthday |
| 1st Mon in Sep | Labor Day |
| Sep 19 | Talk Like a Pirate Day |
| Sep 29 | National Coffee Day |
| Oct 31 | Halloween |
| Nov 11 | Veterans Day |
| 4th Thu in Nov | Thanksgiving |
| Dec 25 | Christmas |
| Dec 31 | New Year's Eve |

### Development preview

In `Firmware-PIO/include/holiday_easter_eggs.h`:

- `PREVIEW_HOLIDAYS` — cycle every holiday animation on boot
- `RUN_HOLIDAY_PREVIEW_ON_BOOT` — preview a single holiday on boot (when `PREVIEW_HOLIDAYS` is false)

---

## Hardware

### Electronics

| Component                      | Link                                                    |
| ------------------------------ | ------------------------------------------------------- |
| ESP32 Dev Board                | [AliExpress](https://s.click.aliexpress.com/e/_opLIWvk) |
| MAX7219 Dot Matrix (4 modules) | [AliExpress](https://s.click.aliexpress.com/e/_oo3TdS6) |

### Hardware

| Component         | Link                                                     |
| ----------------- | -------------------------------------------------------- |
| M3 Thread Inserts | [AliExpress](https://s.click.aliexpress.com/e/_c2Iun0o1) |
| M3x8 Screws       | [AliExpress](https://s.click.aliexpress.com/e/_oogbRPM)  |
| Acrylic Sheet     | [AliExpress](https://s.click.aliexpress.com/e/_oorPPai)  |
| 6x3mm Magnets     | [AliExpress](https://s.click.aliexpress.com/e/_c3qP6N2t) |

### Battery / Wireless Version (optional)

| Component     | Link                                                     |
| ------------- | -------------------------------------------------------- |
| Battery       | [AliExpress](https://s.click.aliexpress.com/e/_opFpjhg)  |
| BMS           | [AliExpress](https://s.click.aliexpress.com/e/_c4sosls1) |
| On/Off Switch | [AliExpress](https://s.click.aliexpress.com/e/_oDqU0l8)  |

> Hardware affiliate links are from the original project by [The Printing Pilot](https://github.com/ThePrintingPilot/YouTube-Subscriber-Counter) — they help support that project at no extra cost to you.

<div align="center">
  <img src="https://github.com/justinmahar/YouTube-Subscriber-Counter/blob/main/Images/ExplodedView.gif?raw=true" width="650" alt="YouTube Subscriber Counter V2.0 Exploded View">
</div>

---


## Download the 3D Files


[![Printables](https://img.shields.io/badge/Printables-FA6831?style=for-the-badge&logoColor=white)](https://www.printables.com/model/1756251-youtube-subscriber-v20)
[![MakerWorld](https://img.shields.io/badge/MakerWorld-000000?style=for-the-badge&logoColor=white)](https://makerworld.com/en/models/2941691-youtube-subscriber-v2-0#profileId-3294669)


---

### Wiring

Battery Version:

<div align="center">
  <img src="https://raw.githubusercontent.com/justinmahar/YouTube-Subscriber-Counter/refs/heads/main/Images/Circuit%20Battery.png" width="650" alt="Wiring - Battery Version">
</div>

Regular Version:

<div align="center">
  <img src="https://raw.githubusercontent.com/justinmahar/YouTube-Subscriber-Counter/refs/heads/main/Images/Circuit%20Regular.png" width="650" alt="Wiring - Regular Version">
</div>

---

## First-Time Setup

### 1. Prepare a stats endpoint

The device fetches a user-provided endpoint that returns this JSON shape.

```json
{
  "serverTimeUnix": 1781918014,
  "baseline": {
    "subscribersStartedAtUnix": 1781917014,
    "totalViewsStartedAtUnix": 1781917014,
    "watchHoursStartedAtUnix": 1781917014,
    "subscribers": 10169,
    "totalViews": 2748484,
    "watchHours": 0
  },
  "metrics28Days": {
    "subscribers": 405,
    "views": 177568,
    "watchHours": 0
  },
  "adjusted": {
    "asOfUnix": 1781918014,
    "subscribers": 10170,
    "totalViews": 2748557,
    "watchHours": 0
  }
}
```

Required fields:

- `serverTimeUnix` — current server time; used for stat projection timing and holiday date checks
- `baseline` — starting whole-number stats plus a Unix timestamp per stat (`subscribersStartedAtUnix`, `totalViewsStartedAtUnix`, `watchHoursStartedAtUnix`); each stat projects from its own baseline time
- `metrics28Days` — subscriber, view, and watch-hour gains over the last 28 days (projection rates)
- `adjusted` — projected whole-number stats as of `asOfUnix`

`growthRatesPerMinute` is optional and is not required by the firmware.

### 2. Flash the firmware

**Option A — Browser installer (recommended)**

No IDE needed. Click the link below, connect your ESP32 via USB, and hit Install:

➡ **[Install via browser](https://justinmahar.github.io/YouTube-Subscriber-Counter/)**

<div align="center">
  <img src="https://raw.githubusercontent.com/justinmahar/YouTube-Subscriber-Counter/refs/heads/main/Images/webinstaller.png" width="250" alt="Config Page">
</div>

Requires Chrome or Edge on desktop.

**Option B — PlatformIO**

1. Open the `Firmware-PIO` folder in VS Code or Cursor (PlatformIO extension required)
2. Build and upload to your ESP32 board



### 3. Configure via the portal

1. Power on the device — the matrix shows `Setup`
2. On your phone or laptop, connect to the Wi-Fi network shown on the matrix — **`YouTubeCounter-Setup-XXXX`** (unique 4-character suffix per device)
3. A browser page opens automatically (or navigate to `192.168.4.1`)
4. Tap **Scan for networks**, pick your Wi-Fi, enter your password
5. Enter your stats endpoint, select which stats to show, and choose the refresh rate in minutes
6. Hit **Save & connect** — the device reboots and starts fetching stats


<div align="center">
  <img src="https://raw.githubusercontent.com/justinmahar/YouTube-Subscriber-Counter/refs/heads/main/Images/webconfig.png" width="250" alt="Config Page">
</div>

---

## After Setup

On every boot the device connects to your saved Wi-Fi and scrolls the assigned IP address across the matrix. Open that IP in any browser on the same network to:

- Change Wi-Fi network or password
- Update the stats endpoint, selected stats, or refresh rate
- Upload new firmware (`.bin`) without a USB cable

Leave the Wi-Fi fields blank when saving and the device keeps the previously stored network values.

---

## OTA Firmware Updates (device already flashed)

1. Build with PlatformIO — the OTA `.bin` is at `Firmware-PIO/.pio/build/esp32dev/firmware.bin`
2. Open the device IP in your browser
3. Scroll to **Firmware update**, pick the main firmware `.bin`, click **Upload firmware**
4. The matrix shows `OTA...` then `Rebooting` — done


---

## Development

All firmware source lives in `Firmware-PIO/`. Open that folder as your Cursor/VS Code workspace when building, flashing, or simulating.

### Build and flash (PlatformIO)

```bash
cd Firmware-PIO
pio run              # compile
pio run -t upload    # flash via USB
pio device monitor   # serial log at 9600 baud
```

### Wokwi simulator

Simulate the ESP32 + 4-module MAX7219 matrix without hardware.

**Requirements:** [PlatformIO](https://platformio.org/) and the [Wokwi for VS Code](https://marketplace.visualstudio.com/items?itemName=wokwi.wokwi-vscode) extension.

1. Open **`Firmware-PIO`** as your workspace root (where `wokwi.toml` lives). If the workspace root is the repo folder instead, Wokwi will not load port forwarding.
2. Build: `./scripts/build-firmware.sh` or `pio run`
3. Start: `Cmd+Shift+P` → **Wokwi: Start Simulator**
4. Keep the **simulator tab visible** — Wokwi pauses when you switch away.
5. Open **`http://localhost:8180`** (not `https://`) in your browser to access the simulated setup portal.
6. On first boot the firmware auto-connects to **`Wokwi-GUEST`** for simulator setup. Enter your private stats endpoint, choose stats, set refresh minutes, and save.

For development, `ENABLE_WOKWI_SETUP` in `Firmware-PIO/src/main.cpp` is set to `true` so the firmware tries `Wokwi-GUEST` before starting the normal setup hotspot. Set it to `false` for hardware-only builds if you do not want the display to show `Wokwi...` during setup.

If `localhost:8180` does not load:

- Confirm the workspace root is **`Firmware-PIO`**, not the parent repo folder.
- Stop and restart the simulator after changing `wokwi.toml`.
- Check the serial log for `Wokwi setup portal ready.` and `Open http://localhost:8180`.
- If you see `Setup AP` instead, the sim did not join `Wokwi-GUEST`; reset the ESP32 in the simulator and try again.

Circuit wiring is defined in `diagram.json` (DIN→GPIO23, CLK→GPIO18, CS→GPIO5, power via `V+` / `GND.2`). On first boot with no saved credentials, the matrix shows `Setup` and the device starts the config portal.

For private local endpoint values, keep them in an untracked `.env.local` or notes file and paste them into the portal. Do not commit production endpoint URLs.

### Web installer binaries

The [browser installer](https://justinmahar.github.io/YouTube-Subscriber-Counter/) uses pre-built flash images in `docs/`, referenced by `docs/manifest.json`:

| File                  | Source (after `pio run`)                          |
| --------------------- | ------------------------------------------------- |
| `docs/bootloader.bin` | `Firmware-PIO/.pio/build/esp32dev/bootloader.bin` |
| `docs/partitions.bin` | `Firmware-PIO/.pio/build/esp32dev/partitions.bin` |
| `docs/firmware.bin`   | `Firmware-PIO/.pio/build/esp32dev/firmware.bin`   |

To refresh the web installer after firmware changes:

```bash
cd Firmware-PIO && pio run
../scripts/update-web-installer.sh
```

Then bump the `"version"` field in `docs/manifest.json` if you are publishing a release, commit the updated `docs/*.bin` files, and push so GitHub Pages serves the new build.

**Note:** OTA updates on a flashed device use `firmware.bin` only (the app partition). The browser installer flashes the full image (bootloader + partition table + app).

---

## Credits

- Original project, 3D enclosure, and browser installer design by [**The Printing Pilot**](https://github.com/ThePrintingPilot/YouTube-Subscriber-Counter)
- Number formatting code by [The Swedish Maker](https://www.youtube.com/@TheSwedishMaker)

---

## License

MIT
