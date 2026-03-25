# ESP8266 LCD1602 DHT11 Monitor

Simple firmware project for `NodeMCU 1.0 (ESP-12E Module)` with `LCD1602 I2C`, `DHT11`, optional `Wi-Fi + NTP`, OTA, and web setup.

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
- Starts an AP setup portal when Wi-Fi is not configured
- Exposes a web UI at `/` and JSON status at `/status`
- Enables Arduino OTA after joining Wi-Fi
- Prints diagnostics to `Serial` at `115200`
- Keeps code structure ready for future `Wi-Fi`, `NTP`, and `OTA`

## First boot and setup mode

- If no Wi-Fi is saved, the device starts AP mode: `ESP8266-Setup`
- Connect to it and open `http://192.168.4.1`
- Save Wi-Fi SSID, password, and UTC offset
- The device restarts and joins your network

## Optional default Wi-Fi values

You can also prefill defaults in `include/config.h`:

```cpp
constexpr char kWifiSsid[] = "YOUR_WIFI";
constexpr char kWifiPassword[] = "YOUR_PASSWORD";
constexpr long kUtcOffsetSeconds = 0;
```

Saved config from the web UI takes priority over compile-time defaults.

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

When the device is waiting for setup, the display alternates between the sensor screen and:

```text
Setup AP mode
192.168.4.1
```

## Web endpoints

- `/` - setup page for Wi-Fi and UTC offset
- `/status` - JSON status with IP, mode, OTA state, and time sync state

## OTA update

- After the device joins Wi-Fi, Arduino OTA is enabled automatically
- OTA hostname format: `esp8266-weather-<chipid>`
- Upload from Arduino IDE or another OTA-capable workflow on the same LAN

## Notes

- The contrast trimmer on most I2C backpacks changes text contrast, not backlight brightness.
- If the backlight is too bright, that usually needs a hardware fix on the backpack.
- Do not use `GPIO0` (`IO0`) for the sensor or LCD in this project.
