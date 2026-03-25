# ESP8266 LCD1602 DHT11 Monitor

Simple firmware project for `NodeMCU 1.0 (ESP-12E Module)` with `LCD1602 I2C` and `DHT11`.

## Wiring

- `LCD SDA` -> `D2 / GPIO4`
- `LCD SCL` -> `D1 / GPIO5`
- `LCD VCC` -> `3.3V`
- `LCD GND` -> `GND`
- `DHT11 S` -> `D5 / GPIO14`
- `DHT11 middle pin` -> `3.3V`
- `DHT11 -` -> `GND`

## Features

- Auto-detects LCD I2C address (`0x27`, `0x3F`, then full scan)
- Reads `DHT11` every 2.5 seconds
- Shows temperature and humidity on a 16x2 display
- Supports optional `Wi-Fi + NTP` clock screen when credentials are configured
- Prints diagnostics to `Serial` at `115200`
- Keeps code structure ready for future `Wi-Fi`, `NTP`, and `OTA`

## Optional Wi-Fi clock setup

To enable the clock screen, edit `include/config.h` and set:

```cpp
constexpr char kWifiSsid[] = "YOUR_WIFI";
constexpr char kWifiPassword[] = "YOUR_PASSWORD";
constexpr long kUtcOffsetSeconds = 0;
```

If `kWifiSsid` is empty, the firmware stays in offline sensor-only mode.

## Build and upload

```bash
.venv/bin/pio run
.venv/bin/pio run -t upload
.venv/bin/pio device monitor -b 115200
```

## Expected display output

Line 1:

```text
Temp: 24.0 C
```

Line 2:

```text
Hum: 51 %
```

If the sensor read fails:

```text
DHT11 error
Check wiring
```

When Wi-Fi and NTP are configured, the display alternates between:

```text
Temp: 24.0 C
Hum: 51 %
```

and:

```text
12:34:56
Clock via NTP
```

## Notes

- The contrast trimmer on most I2C backpacks changes text contrast, not backlight brightness.
- If the backlight is too bright, that usually needs a hardware fix on the backpack.
- Do not use `GPIO0` (`IO0`) for the sensor or LCD in this project.
