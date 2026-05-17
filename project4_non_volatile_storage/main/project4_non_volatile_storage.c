#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"


void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error opening NVS handle!\n");
        return;

    }
    int32_t count = 0;
    err = nvs_get_i32(my_handle, "Boot count", &count);
    count++;
    printf("Boot count: %ld\n", count);
    ESP_LOGI("NVS", "Boot count: %ld", count);
    err = nvs_set_i32(my_handle, "Boot count", count);
    if (err != ESP_OK) {
        printf("Error setting Boot count!\n");
    }
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        printf("Error committing changes to NVS!\n");
    }
    nvs_close(my_handle);
}
