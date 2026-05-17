#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define BUTTON_GPIO GPIO_NUM_0 // GPIO pin for the button
#define LED_GPIO GPIO_NUM_2    // GPIO pin for the LED

static QueueHandle_t gpio_evt_queue;

static void IRAM_ATTR button_isr(void *arg){
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void button_task(void *arg){
    uint32_t io_num;
    static int state = 0;
    while (1){
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)){
            state ^= 1;
            gpio_set_level(LED_GPIO, state);
            ESP_LOGI("btn", "Button pressed, LED %s", state ? "ON" : "OFF");
        }
    }
}


void app_main(void)
{

    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    
    gpio_config_t btn = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,  // trigger on falling edge
    };


    gpio_config(&btn);
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, (void*)BUTTON_GPIO);
    xTaskCreate(button_task, "button", 2048, NULL, 10, NULL);

}

