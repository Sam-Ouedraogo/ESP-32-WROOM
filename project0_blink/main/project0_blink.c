// FreeRTOS core headers — required for tasks and delays
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// FreeRTOS queue — lets tasks send data to each other safely
#include "freertos/queue.h"
// ESP-IDF GPIO driver — controls the physical pins on the board
#include "driver/gpio.h"
// ESP-IDF logging — prints messages over serial (visible in `idf.py monitor`)
#include "esp_log.h"

// GPIO 2 is the onboard LED on most ESP32 dev boards
#define LED_A  GPIO_NUM_2
// GPIO 4 requires an external LED wired with a ~330Ω resistor to GND
#define LED_B  GPIO_NUM_4

// Tag used to label log messages, e.g. "I (1234) blink: Speed changed"
static const char *TAG = "blink";

// A queue that holds uint32_t values (blink speed in ms).
// Declared globally so both app_main and blink_task_b can access it.
static QueueHandle_t cmd_queue;

// Task A: blinks LED_A at a fixed 500 ms on / 500 ms off rate.
// The void *arg parameter is required by FreeRTOS but unused here.
void blink_task_a(void *arg) {
    // Set the pin as an output so we can drive it high/low
    gpio_set_direction(LED_A, GPIO_MODE_OUTPUT);
    while (1) {
        gpio_set_level(LED_A, 1);                // turn LED on
        vTaskDelay(pdMS_TO_TICKS(500));          // wait 500 ms
        gpio_set_level(LED_A, 0);                // turn LED off
        vTaskDelay(pdMS_TO_TICKS(500));          // wait 500 ms
        // pdMS_TO_TICKS converts milliseconds to FreeRTOS tick counts
    }
}

// Task B: blinks LED_B at a speed that can be updated via the queue.
void blink_task_b(void *arg) {
    gpio_set_direction(LED_B, GPIO_MODE_OUTPUT);
    uint32_t speed_ms = 200;    // default blink period in milliseconds
    while (1) {
        // Check the queue for a new speed — the 0 means "don't wait, just check".
        // If a value is available it overwrites speed_ms; otherwise speed_ms is unchanged.
        xQueueReceive(cmd_queue, &speed_ms, 0);

        gpio_set_level(LED_B, 1);
        vTaskDelay(pdMS_TO_TICKS(speed_ms));
        gpio_set_level(LED_B, 0);
        vTaskDelay(pdMS_TO_TICKS(speed_ms));
    }
}

// app_main is the entry point for ESP-IDF — equivalent to main() in standard C.
void app_main(void) {
    // Create a queue that can hold up to 5 items, each the size of a uint32_t
    cmd_queue = xQueueCreate(5, sizeof(uint32_t));

    // Spawn task A — stack size 2048 bytes, priority 5
    xTaskCreate(blink_task_a, "blink_a", 2048, NULL, 5, NULL);
    // Spawn task B — same priority, so both tasks share CPU time equally
    xTaskCreate(blink_task_b, "blink_b", 2048, NULL, 5, NULL);

    // Wait 5 seconds while the two tasks run at their default speeds
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Send a new speed to task B through the queue.
    // Task B will pick this up on its next loop iteration and blink faster.
    uint32_t new_speed = 50;
    xQueueSend(cmd_queue, &new_speed, 0);

    // Log a message — visible in the serial monitor as:
    // I (5023) blink: Speed changed
    ESP_LOGI(TAG, "Speed changed");
}
