# pulse_width_modulation

An ESP-IDF project that fades the onboard LED in and out using the ESP32's LEDC (LED Control) peripheral to generate a PWM signal.

## What it does

Continuously fades the onboard LED from off to full brightness, then back to off, in a smooth loop using 1024 brightness steps.

## How PWM works

PWM (Pulse Width Modulation) dims an LED by rapidly switching it on and off. At 5 kHz the eye can't see the flicker — it perceives brightness based on how long the signal stays high per cycle (the duty cycle).

```
duty=0    → LED off
duty=512  → LED at 50% brightness
duty=1023 → LED at full brightness
```

## Hardware

No external wiring needed. The project runs on the **onboard LED (GPIO 2)** present on most ESP32 dev boards.

| Component | Details |
|---|---|
| Board | ESP32 dev board |
| LED | Onboard LED (GPIO 2) |

## Requirements

- [ESP-IDF v5.2](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/get-started/index.html)
- ESP32 dev board

## Build and flash

```bash
. ~/esp/esp-idf/export.sh
idf.py -p /dev/YOUR_PORT flash monitor
```

Replace `/dev/YOUR_PORT` with your board's serial port.

Press `Ctrl+]` to exit the monitor.

## Key concepts

- **LEDC peripheral** — ESP32 hardware module dedicated to generating PWM signals
- **Timer** — sets the PWM frequency (5 kHz here)
- **Channel** — connects the timer to a GPIO pin and controls the duty cycle
- **10-bit resolution** — 1024 discrete brightness levels (0–1023)
