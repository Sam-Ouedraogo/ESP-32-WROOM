# project5_wifi_station_and_http_requests

An ESP-IDF project that connects the ESP32 to a Wi-Fi network as a station and makes an HTTP GET request.

## What it does

1. Connects to a Wi-Fi network
2. Waits until an IP address is assigned
3. Makes an HTTP GET request to a URL
4. Prints the HTTP response status code to the serial monitor

```
I (4904) wifi: Connected
I (5594) https: Status: 200
```

## How it works

**Wi-Fi connection lifecycle:**

The ESP32 Wi-Fi stack is event-driven. Rather than polling for connection status, the app registers an event handler that reacts to three events:

| Event | Action |
|---|---|
| `WIFI_EVENT_STA_START` | Call `esp_wifi_connect()` to begin connecting |
| `WIFI_EVENT_STA_DISCONNECTED` | Automatically reconnect |
| `IP_EVENT_STA_GOT_IP` | Set a FreeRTOS event group bit to signal the main task |

`app_main` blocks on `xEventGroupWaitBits` until the `WIFI_CONNECTED_BIT` is set, then proceeds to make the HTTP request.

## Hardware

No extra hardware needed. Uses the ESP32's built-in Wi-Fi radio.

## Requirements

- [ESP-IDF v5.2](https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/get-started/index.html)
- ESP32 dev board
- 2.4 GHz Wi-Fi network

## Configuration

Before building, set your Wi-Fi credentials in the source file:

```c
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"
```

> Do not commit real credentials to a public repository.

## Build and flash

```bash
. ~/esp/esp-idf/export.sh
idf.py -p /dev/YOUR_PORT flash monitor
```

Replace `/dev/YOUR_PORT` with your board's serial port. Press `Ctrl+]` to exit the monitor.
