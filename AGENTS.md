# AGENTS.md

This repository contains a PlatformIO firmware project for `NodeMCU 1.0 (ESP-12E Module)` using `ESP8266`, `LCD1602 I2C`, `DHT11`, optional `Wi-Fi + NTP`, OTA, and a built-in web setup page.

## Project Overview

- Goal: display temperature and humidity on a 16x2 I2C LCD and allow optional clock/network/status features.
- Current scope: working firmware with modular structure, setup AP, web config, multi-screen LCD UX, status endpoint, and OTA.
- Future scope: richer status pages, captive portal improvements, and additional screens can be added without major rewrites.
- Framework: Arduino via PlatformIO.
- Primary target: `env:nodemcuv2`.

## Repository Layout

- `platformio.ini` - PlatformIO environment and dependencies.
- `include/config.h` - pins, timings, and hardware constants.
- `include/app_state.h` - shared runtime state.
- `include/app_config.h` - persisted Wi-Fi and timezone configuration.
- `include/config_store.h` - LittleFS-backed config persistence interface.
- `include/display.h` - LCD manager interface.
- `include/sensor.h` - DHT11 manager interface.
- `include/clock_service.h` - Wi-Fi, NTP, OTA, and web setup interface.
- `include/app.h` - top-level controller interface.
- `src/main.cpp` - Arduino entrypoints.
- `src/app.cpp` - app orchestration and timing.
- `src/config_store.cpp` - JSON config load/save via LittleFS.
- `src/clock_service.cpp` - Wi-Fi mode, NTP sync, OTA, and web handlers.
- `src/display.cpp` - LCD detection and rendering.
- `src/sensor.cpp` - DHT11 reads and serial logging.
- `README.md` - wiring and usage notes.

## Hardware Assumptions

- Board: `NodeMCU 1.0 (ESP-12E Module)`.
- LCD: `LCD1602` with I2C backpack.
- Sensor: `DHT11` module.
- Wiring defaults:
  - `LCD SDA -> D2 / GPIO4`
  - `LCD SCL -> D1 / GPIO5`
  - `DHT11 S -> D5 / GPIO14`
  - `DHT11 VCC -> 3.3V`
  - `DHT11 GND -> GND`
- Do not repurpose `GPIO0` / `IO0` or `RST` for application logic.

## Environment Setup

- This repo uses a local Python virtual environment for PlatformIO.
- If `.venv` is missing, create it with:

```bash
python3 -m venv .venv
.venv/bin/pip install platformio
```

- Prefer running PlatformIO through `.venv/bin/pio` in this repo.
- Do not assume global `pio` is installed.

## Build Commands

- Full build:

```bash
.venv/bin/pio run
```

- Build explicit environment:

```bash
.venv/bin/pio run -e nodemcuv2
```

- Clean build artifacts:

```bash
.venv/bin/pio run -t clean
```

- Clean and rebuild when dependencies or FS-related code changes significantly:

```bash
.venv/bin/pio run -t clean && .venv/bin/pio run
```

- Upload firmware to board:

```bash
.venv/bin/pio run -e nodemcuv2 -t upload
```

- Serial monitor at expected baud rate:

```bash
.venv/bin/pio device monitor -b 115200
```

## Test Commands

- There are no automated tests yet.
- The main verification path is successful firmware build plus on-device smoke testing.
- When tests are added, prefer PlatformIO test targets:

```bash
.venv/bin/pio test -e <env>
```

- Run a single test suite by filter when tests exist:

```bash
.venv/bin/pio test -e <env> -f <suite_name>
```

- Run a single test file pattern when supported by the suite layout:

```bash
.venv/bin/pio test -e <env> -f test_sensor
```

## Lint / Static Analysis

- No dedicated linter is configured yet.
- Treat a clean PlatformIO build as the minimum quality gate.
- If adding lint later, prefer lightweight embedded-friendly checks and document the command in this file.

## Manual Verification Checklist

- Firmware builds successfully for `nodemcuv2`.
- LCD initializes and detects address `0x27` or `0x3F`.
- Boot text appears on screen.
- DHT11 values appear in serial logs.
- Display updates with temperature and humidity.
- If Wi-Fi is not configured, the device starts AP mode `ESP8266-Setup`.
- `http://192.168.4.1` opens the setup page in AP mode.
- `/status` returns JSON status in both AP and STA modes.
- LCD rotates through sensor, time, and network/status screens when available.
- OTA becomes available after Wi-Fi connects successfully.
- If the sensor is disconnected, the LCD shows an error state.

## Code Style

- Use C++ compatible with Arduino/ESP8266 toolchains.
- Prefer small, focused translation units over one giant sketch file.
- Keep hardware constants in `config.h` as `constexpr` values.
- Avoid dynamic behavior unless it is justified by hardware libraries.
- Write code that is easy to debug over serial output.

## Imports and Includes

- In headers, include only what is needed for declarations.
- In `.cpp` files, include the matching project header first when practical.
- Prefer project headers with quotes and library headers with angle brackets.
- Do not add unused includes.
- Keep include groups simple: project headers, Arduino/framework headers, then library headers if needed.

## Formatting

- Use 2-space indentation.
- Keep braces on the same line for functions and control blocks.
- Keep lines reasonably short; optimize for readability on firmware code reviews.
- Use blank lines to separate logical sections, not every statement.
- Avoid comment noise; add comments only for non-obvious hardware or timing decisions.

## Types and Constants

- Use fixed-width integer types like `uint8_t` for pins, addresses, and compact hardware values.
- Use `unsigned long` for `millis()`-based timing values to match Arduino conventions.
- Use `constexpr` for compile-time constants.
- Use `struct` for simple data carriers such as sensor readings and app state.
- Use `enum class` if more app modes or screen types are introduced later.

## Naming Conventions

- Types: `PascalCase`.
- Functions and methods: `camelCase`.
- Private members: trailing underscore, e.g. `address_`.
- Constants in namespaces: `kName` style, e.g. `kDhtPin`.
- File names: lowercase with underscores only if needed; follow current repository naming.

## Error Handling

- Never assume the LCD or sensor is present.
- Fail soft: continue running and log errors over serial when possible.
- For sensor reads, always handle `NaN` explicitly.
- For I2C display setup, try known addresses before a scan.
- For filesystem-backed config, handle mount/load/save failures explicitly and keep running in a safe fallback mode.
- For Wi-Fi setup, prefer AP fallback over a dead-end boot state.
- Show short human-readable error messages on the LCD.
- Prefer returning status objects or booleans over hidden global failure states.

## State Management

- Keep runtime state centralized in `AppState` or similarly clear structures.
- Keep persisted credentials and timezone data in `AppConfig`, not in unrelated controllers.
- Separate hardware access from orchestration logic.
- Do not mix display formatting with low-level sensor reads.
- Keep network, OTA, web, and NTP concerns inside `ClockService` or adjacent modules.
- Throttle sensor polling and LCD refresh independently.
- Preserve the last valid values unless there is a strong reason to clear them.

## Embedded-Specific Guidelines

- Avoid frequent `String` churn in hot paths if code grows larger; for the current small project it is acceptable.
- Avoid blocking delays in the main loop except short startup settling delays.
- Use `millis()` scheduling instead of long `delay()` calls.
- Be careful with boot-sensitive ESP8266 pins.
- Keep RAM and flash usage visible after meaningful changes.
- Be conservative with heap churn from `String`, HTML generation, and JSON documents.
- Prefer AP fallback plus web setup over hardcoding secrets into the repository.

## Logging

- Use `Serial` at `115200`.
- Log startup state, LCD detection result, and sensor read failures.
- Log Wi-Fi mode transitions, config persistence failures, OTA start/end, and NTP sync.
- Keep log messages short and grep-friendly, e.g. `[LCD]`, `[DHT]`, `[APP]`.
- Do not spam serial output faster than the sensor refresh cadence.

## Git Workflow

- Primary branch is `main`.
- Use short-lived feature branches for changes.
- Prefer pull requests even for small firmware increments.
- Keep commits focused and descriptive.
- Do not commit build artifacts, `.pio`, or virtual environments.

## Agent Expectations

- Make the smallest safe change that solves the task.
- Build the firmware after non-trivial code changes.
- Report exact commands used for build and verification.
- If hardware behavior is uncertain, preserve current wiring assumptions in `README.md` and this file.
- If introducing Wi-Fi, OTA, storage, or web setup, update this file with the new commands and constraints.

## Cursor / Copilot Rules

- No `.cursor/rules/`, `.cursorrules`, or `.github/copilot-instructions.md` were present when this file was written.
- If such files are added later, their instructions should be merged into this document and treated as repository policy.
