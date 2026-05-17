# project0_blink

An introductory ESP-IDF project demonstrating FreeRTOS tasks and inter-task communication via a queue.

## What it does

- **LED A (GPIO 2)** — onboard LED, blinks at a fixed 500 ms rate
- **LED B (GPIO 4)** — external LED, starts blinking at 200 ms then speeds up to 50 ms after 5 seconds
- The speed change is sent from `app_main` to `blink_task_b` using a FreeRTOS queue

## Hardware

| Component | Details |
|---|---|
| Board | ESP32 dev board |
| LED A | Onboard LED (GPIO 2, no wiring needed) |
| LED B | External LED on GPIO 4 → 330Ω resistor → GND |

## Requirements

- [ESP-IDF v5.2](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/get-started/index.html)
- ESP32 board

## Build and flash

```bash
. ~/esp/esp-idf/export.sh
idf.py -p /dev/YOUR_PORT flash monitor
```

Replace `/dev/YOUR_PORT` with your board's serial port.


## Expected serial output

```
I (313) main_task: Calling app_main()
I (5313) blink: Speed changed
I (5313) main_task: Returned from app_main()
```
