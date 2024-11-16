/*
 * SPDX-FileCopyrightText: 2019-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define GATT_ENV_SNS_UUID 0x181A
#define DEVICE_NAME "ULTRASONIC"
#define MANUFACTURER_NAME "CALYPSO"

static const char *MAIN_TAG = "app_main";


void app_main(void)
{

    esp_err_t ret;

    // Initialize NVS (Required for BLE Calibrations params)
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(MAIN_TAG, "HELLO WORLD");
    vTaskDelay(1000);
    ESP_LOGI(MAIN_TAG, "HELLO WORLD AGAIn");
}