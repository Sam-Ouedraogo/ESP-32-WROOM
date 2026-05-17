#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#define I2C_SDA       GPIO_NUM_21
#define I2C_SCL       GPIO_NUM_22
#define I2C_FREQ_HZ   100000

// BMP280 I2C address — 0x76 when SDO pin is wired to GND, 0x77 when SDO to VCC
#define BMP280_ADDR   0x76

// BMP280 register addresses (from the datasheet)
#define REG_CHIP_ID   0xD0   // always reads 0x58 on a real BMP280
#define REG_RESET     0xE0   // write 0xB6 here to soft-reset the sensor
#define REG_CTRL_MEAS 0xF4   // sets oversampling and power mode
#define REG_PRESS_MSB 0xF7   // start of the 6-byte pressure+temperature block
#define REG_CALIB     0x88   // start of the 24-byte calibration block

#define BMP280_CHIP_ID  0x58
#define BMP280_RESET_VAL 0xB6

static const char *TAG = "bmp280";

// The BMP280 stores unique calibration constants in its non-volatile memory.
// These compensate for manufacturing variations between chips.
// They must be read once at startup and applied to every raw reading.
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
} bmp280_calib_t;

// Read `len` bytes from the sensor starting at `reg_addr`.
// i2c_master_transmit_receive does a write (sends the register address)
// immediately followed by a read — this is the standard I2C register-read pattern.
static esp_err_t bmp280_read(i2c_master_dev_handle_t dev, uint8_t reg_addr, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(dev, &reg_addr, 1, data, len, pdMS_TO_TICKS(100));
}

// Write a single byte value to a register
static esp_err_t bmp280_write(i2c_master_dev_handle_t dev, uint8_t reg_addr, uint8_t value) {
    uint8_t buf[2] = {reg_addr, value};
    return i2c_master_transmit(dev, buf, 2, pdMS_TO_TICKS(100));
}

// Parse the 24 calibration bytes into the struct.
// Values are stored little-endian (low byte first).
static void bmp280_read_calibration(i2c_master_dev_handle_t dev, bmp280_calib_t *c) {
    uint8_t b[24];
    bmp280_read(dev, REG_CALIB, b, 24);

    c->dig_T1 = (uint16_t)(b[1]  << 8 | b[0]);
    c->dig_T2 = (int16_t) (b[3]  << 8 | b[2]);
    c->dig_T3 = (int16_t) (b[5]  << 8 | b[4]);
    c->dig_P1 = (uint16_t)(b[7]  << 8 | b[6]);
    c->dig_P2 = (int16_t) (b[9]  << 8 | b[8]);
    c->dig_P3 = (int16_t) (b[11] << 8 | b[10]);
    c->dig_P4 = (int16_t) (b[13] << 8 | b[12]);
    c->dig_P5 = (int16_t) (b[15] << 8 | b[14]);
    c->dig_P6 = (int16_t) (b[17] << 8 | b[16]);
    c->dig_P7 = (int16_t) (b[19] << 8 | b[18]);
    c->dig_P8 = (int16_t) (b[21] << 8 | b[20]);
    c->dig_P9 = (int16_t) (b[23] << 8 | b[22]);
}

// Convert raw 20-bit ADC temperature to units of 0.01°C.
// Formula taken directly from BMP280 datasheet section 4.2.3.
// t_fine is an intermediate value that the pressure formula also needs.
static int32_t bmp280_compensate_temperature(int32_t adc_T, bmp280_calib_t *c, int32_t *t_fine) {
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)c->dig_T1 << 1))) * (int32_t)c->dig_T2) >> 11;
    int32_t var2 = (((((adc_T >> 4) - (int32_t)c->dig_T1) * ((adc_T >> 4) - (int32_t)c->dig_T1)) >> 12) * (int32_t)c->dig_T3) >> 14;
    *t_fine = var1 + var2;
    return (*t_fine * 5 + 128) >> 8;
}

// Convert raw 20-bit ADC pressure to Pa * 256 (divide result by 256 to get Pascals).
// Must be called after bmp280_compensate_temperature to have a valid t_fine.
static uint32_t bmp280_compensate_pressure(int32_t adc_P, bmp280_calib_t *c, int32_t t_fine) {
    int64_t var1, var2, p;
    var1 = (int64_t)t_fine - 128000;
    var2 = var1 * var1 * (int64_t)c->dig_P6;
    var2 += (var1 * (int64_t)c->dig_P5) << 17;
    var2 += ((int64_t)c->dig_P4) << 35;
    var1 = ((var1 * var1 * (int64_t)c->dig_P3) >> 8) + ((var1 * (int64_t)c->dig_P2) << 12);
    var1 = ((((int64_t)1 << 47) + var1) * (int64_t)c->dig_P1) >> 33;
    if (var1 == 0) return 0;  // avoid division by zero
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)c->dig_P9 * (p >> 13) * (p >> 13)) >> 25;
    var2 = ((int64_t)c->dig_P8 * p) >> 19;
    p = ((p + var1 + var2) >> 8) + ((int64_t)c->dig_P7 << 4);
    return (uint32_t)p;
}

void app_main(void) {
    // --- Initialize the I2C master bus ---
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = I2C_NUM_0,
        .sda_io_num        = I2C_SDA,
        .scl_io_num        = I2C_SCL,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,  // use ESP32 internal pull-ups
    };
    i2c_master_bus_handle_t bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus));

    // --- Register the BMP280 as a device on the bus ---
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BMP280_ADDR,
        .scl_speed_hz    = I2C_FREQ_HZ,
    };
    i2c_master_dev_handle_t bmp280;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus, &dev_cfg, &bmp280));

    // --- Verify we are talking to a real BMP280 ---
    uint8_t chip_id;
    bmp280_read(bmp280, REG_CHIP_ID, &chip_id, 1);
    if (chip_id != BMP280_CHIP_ID) {
        ESP_LOGE(TAG, "Wrong chip ID: 0x%02X (expected 0x58). Check wiring and address.", chip_id);
        return;
    }
    ESP_LOGI(TAG, "BMP280 detected (chip ID: 0x%02X)", chip_id);

    // --- Soft reset to put sensor in a known state ---
    bmp280_write(bmp280, REG_RESET, BMP280_RESET_VAL);
    vTaskDelay(pdMS_TO_TICKS(10));

    // --- Read factory calibration data ---
    bmp280_calib_t calib;
    bmp280_read_calibration(bmp280, &calib);

    // --- Start continuous measurement ---
    // ctrl_meas register: [7:5] temp oversampling x1, [4:2] pressure oversampling x1, [1:0] normal mode
    // Binary: 001 001 11 = 0x27
    bmp280_write(bmp280, REG_CTRL_MEAS, 0x27);

    while (1) {
        // Read 6 consecutive bytes starting at 0xF7:
        // [0]=press_msb [1]=press_lsb [2]=press_xlsb [3]=temp_msb [4]=temp_lsb [5]=temp_xlsb
        uint8_t raw[6];
        bmp280_read(bmp280, REG_PRESS_MSB, raw, 6);

        // Reassemble 20-bit values — data is left-aligned across 3 bytes, lower 4 bits unused
        int32_t adc_P = (int32_t)((raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4));
        int32_t adc_T = (int32_t)((raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4));

        int32_t t_fine;
        int32_t temp_raw   = bmp280_compensate_temperature(adc_T, &calib, &t_fine);
        uint32_t press_raw = bmp280_compensate_pressure(adc_P, &calib, t_fine);

        // temp_raw is in 0.01°C units → divide by 100 for °C
        // press_raw is Pa * 256 → divide by 256 for Pa, then by 100 for hPa
        float temperature  = temp_raw / 100.0f;
        float pressure_hpa = (press_raw / 256.0f) / 100.0f;

        ESP_LOGI(TAG, "Temperature: %.2f C | Pressure: %.2f hPa", temperature, pressure_hpa);

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
