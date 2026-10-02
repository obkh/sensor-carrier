#include "sht31.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef SENSOR_SDA_GPIO
#define SENSOR_SDA_GPIO 21
#endif
#ifndef SENSOR_SCL_GPIO
#define SENSOR_SCL_GPIO 22
#endif

static const char *TAG = "sensor_carrier";

static int bus_write(void *context, uint8_t address,
                     const uint8_t *data, size_t length)
{
    (void)address;
    i2c_master_dev_handle_t device = context;
    return i2c_master_transmit(device, data, length, 1000) == ESP_OK ? 0 : -1;
}

static int bus_read(void *context, uint8_t address,
                    uint8_t *data, size_t length)
{
    (void)address;
    i2c_master_dev_handle_t device = context;
    return i2c_master_receive(device, data, length, 1000) == ESP_OK ? 0 : -1;
}

void app_main(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = SENSOR_SDA_GPIO,
        .scl_io_num = SENSOR_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    i2c_master_bus_handle_t master_bus;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &master_bus));
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHT31_ADDRESS,
        .scl_speed_hz = 100000,
    };
    i2c_master_dev_handle_t device;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(master_bus, &device_config, &device));
    sht31_bus_t sensor = {.context = device, .write = bus_write, .read = bus_read};
    for (;;) {
        sht31_sample_t sample;
        if (sht31_start_measurement(&sensor) == SHT31_OK) {
            vTaskDelay(pdMS_TO_TICKS(20));
            if (sht31_read_measurement(&sensor, &sample) == SHT31_OK) {
                long magnitude = sample.temperature_millideg_c < 0
                    ? -(long)sample.temperature_millideg_c
                    : (long)sample.temperature_millideg_c;
                ESP_LOGI(TAG, "temperature=%s%ld.%03ld C humidity=%lu.%03lu %%RH",
                         sample.temperature_millideg_c < 0 ? "-" : "",
                         magnitude / 1000,
                         magnitude % 1000,
                         (unsigned long)(sample.humidity_millipercent / 1000),
                         (unsigned long)(sample.humidity_millipercent % 1000));
            } else {
                ESP_LOGW(TAG, "read or CRC failure");
            }
        } else {
            ESP_LOGW(TAG, "I2C command failed");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
