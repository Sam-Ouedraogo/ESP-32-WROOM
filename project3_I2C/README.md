# project3_I2C

An ESP-IDF project that reads temperature and pressure from a BMP280 sensor over I2C, implementing the full communication stack from scratch without any external component library.

## What it does

Reads temperature and pressure from a BMP280 every 2 seconds and prints them to the serial monitor.

```
I (5312) bmp280: Temperature: 24.31 C | Pressure: 1013.25 hPa
```

## How it works

Rather than using a pre-built library, this project communicates with the BMP280 directly:

1. Initializes the I2C master bus on GPIO 21/22
2. Verifies the sensor by reading its chip ID register (must return `0x58`)
3. Reads 24 bytes of factory calibration constants unique to each chip
4. Reads raw 20-bit ADC values for pressure and temperature
5. Applies the compensation formulas from the BMP280 datasheet to convert raw values into real units

Temperature must be compensated first — it produces an intermediate value (`t_fine`) that the pressure formula also depends on.

## Hardware

| Component | Details |
|---|---|
| Board | ESP32 dev board |
| Sensor | BMP280 breakout board |

## Wiring

| BMP280 Pin | ESP32 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |
| SDO | GND (sets I2C address to 0x76) |

> If SDO is wired to VCC instead of GND, change `BMP280_ADDR` to `0x77` in the source file.

## Requirements

- [ESP-IDF v5.2](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/get-started/index.html)
- ESP32 dev board
- BMP280 breakout board

## Build and flash

```bash
. ~/esp/esp-idf/export.sh
idf.py -p /dev/YOUR_PORT flash monitor
```

Replace `/dev/YOUR_PORT` with your board's serial port. Press `Ctrl+]` to exit the monitor.

## Troubleshooting

**`I2C transaction failed` / `Wrong chip ID: 0x00`**
The sensor is not responding. Check your wiring and confirm VCC is connected to 3.3V, not 5V. Try changing `BMP280_ADDR` to `0x77` if SDO is not tied to GND.
