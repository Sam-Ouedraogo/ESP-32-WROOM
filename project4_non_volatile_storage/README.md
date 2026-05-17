# project4_non_volatile_storage

An ESP-IDF project demonstrating NVS (Non-Volatile Storage) — a key-value store in flash memory that persists across reboots and power cycles.

## What it does

Tracks how many times the ESP32 has booted and prints the count to the serial monitor on every restart. The count survives power cuts — it never resets unless the NVS partition is explicitly erased.

```
I (312) NVS: Boot count: 1
--- reset ---
I (312) NVS: Boot count: 2
--- reset ---
I (312) NVS: Boot count: 3
```

## How NVS works

NVS is stored in a dedicated partition in flash memory, separate from your application code. It works like a persistent dictionary:

- Data is organized into **namespaces** (like folders) to avoid key collisions between components
- Each entry is a **key-value pair** — keys are strings, values can be integers, strings, or blobs
- A **commit** must be called after writes to actually save changes to flash
- The partition survives firmware updates — flashing new code does not erase NVS

## Hardware

No extra hardware needed. Runs entirely on the ESP32.

## Requirements

- [ESP-IDF v5.2](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/get-started/index.html)
- ESP32 dev board

## Build and flash

```bash
. ~/esp/esp-idf/export.sh
idf.py -p /dev/YOUR_PORT flash monitor
```

Replace `/dev/YOUR_PORT` with your board's serial port. Press `Ctrl+]` to exit the monitor.

## How to test

1. Flash the firmware and watch the monitor — boot count starts at 1
2. Press the **EN (Reset)** button on the board — count increments to 2
3. Unplug and replug the USB cable — count increments again
4. Flash new firmware — count continues from where it left off (NVS is preserved)

If the count ever resets to 1 unexpectedly, the NVS partition was erased — this can happen if you run `idf.py erase-flash`.
