#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/ledc.h"

#define LED_GPIO GPIO_NUM_2
#define LEDC_FREQ 5000
#define LEDC_RES LEDC_TIMER_10_BIT


void app_main(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = LEDC_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num = LED_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };

    ledc_channel_config(&channel);

    while(1) {
        // Fade up
        for (int duty = 0; duty <= (1 << LEDC_RES) - 1; duty++) {
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        // Fade down
        for (int duty = (1 << LEDC_RES) - 1; duty >= 0; duty--) {
            ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

}
