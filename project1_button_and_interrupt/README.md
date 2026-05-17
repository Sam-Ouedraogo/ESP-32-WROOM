# Project 1 — Button + GPIO Interrupt (ESP32 / ESP-IDF)

Interrupt-driven button input that toggles an LED. No polling — the CPU sleeps until the hardware fires an interrupt, making this the correct pattern for any time-sensitive GPIO input.

Part of a progressive ESP-IDF learning series built around the ESP32 DevKit.

---

## Hardware

- ESP32 DevKit (any variant)
- 1× LED
- 1× 220Ω resistor
- Breadboard + jumper wires

The button used is the **BOOT button** already on the DevKit (`GPIO0`) — no extra button needed.

---

## Wiring

```
ESP32 GPIO2  →  220Ω resistor  →  LED anode (+)
LED cathode (-)  →  GND

BOOT button (GPIO0) is onboard — no wiring needed
```

---

## How It Works

Polling checks a pin repeatedly on a timer loop and can miss short events. Interrupts are hardware-triggered — the CPU is notified the instant the pin changes, regardless of what else is running.

```
Button pressed
  → falling edge on GPIO0
    → hardware fires interrupt
      → button_isr() runs (IRAM, fast)
        → GPIO number pushed into FreeRTOS queue
          → button_task() wakes up
            → LED toggled
            → task sleeps again
```

The ISR does the minimum — only queues a notification. All logic runs in the task where blocking and logging are safe.


---

## Build & Flash

Requires [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) installed and environment sourced.

```bash
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

Press the **BOOT button** on the board. The LED toggles and the serial monitor prints:

```
I (XXXX) btn: Button pressed, LED ON
I (XXXX) btn: Button pressed, LED OFF
```

Exit monitor: `Ctrl + ]`

---

## Key Code Notes

**Why `IRAM_ATTR`**
ISRs must live in internal RAM, not flash. Flash cache can be suspended during write operations — an ISR in flash would crash the chip.

**Why `xQueueSendFromISR` not `xQueueSend`**
Standard FreeRTOS calls are not safe inside an ISR. The `FromISR` variants are interrupt-context safe.

**Why `GPIO_INTR_NEGEDGE`**
With the pull-up enabled, the pin sits HIGH at rest and goes LOW when pressed. The interrupt fires on that HIGH→LOW transition (negative edge), not on release.

**Why `portMAX_DELAY`**
The task blocks indefinitely with zero CPU usage until a message arrives. This is the correct FreeRTOS pattern for event-driven tasks.

---

## Requirements

- ESP-IDF v5.0+
- Target: ESP32 (also compatible with ESP32-S3 with pin adjustments)
