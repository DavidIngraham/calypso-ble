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

#include "esp_ble_conn_mgr.h"

#define GATT_ENV_SNS_UUID 0x181A
uint16_t attribute_handle[CONFIG_BT_NIMBLE_MAX_CONNECTIONS];

static const char *MAIN_TAG = "app_main";

static void handle_incoming_data(void *event_data)
{
    esp_ble_conn_data_t *conn_data = (esp_ble_conn_data_t *)event_data;
    switch (conn_data->type) {
    case BLE_CONN_UUID_TYPE_16:
        ESP_LOGI(MAIN_TAG, "%u", conn_data->uuid.uuid16);
        break;
    case BLE_CONN_UUID_TYPE_32:
        ESP_LOG_BUFFER_HEX(MAIN_TAG, &conn_data->uuid, sizeof(conn_data->uuid.uuid32));
        break;
    case BLE_CONN_UUID_TYPE_128:
        ESP_LOG_BUFFER_HEX(MAIN_TAG, &conn_data->uuid, BLE_UUID128_VAL_LEN);
        break;
    default:
        break;
    }
    ESP_LOG_BUFFER_CHAR(MAIN_TAG, conn_data->data, conn_data->data_len);

    esp_ble_conn_data_t inbuff = {
        .type = BLE_CONN_UUID_TYPE_16,
        .uuid = {
            .uuid16 = GATT_ENV_SNS_UUID,
        },
        .data = NULL,
        .data_len = 0,
    };
    esp_err_t rc = esp_ble_conn_read(&inbuff);
    if (rc == 0) {
        ESP_LOGI(MAIN_TAG, "Read data success!");
        ESP_LOG_BUFFER_CHAR(MAIN_TAG, inbuff.data, inbuff.data_len);
    } else {
        ESP_LOGE(MAIN_TAG, "Error in reading characteristic rc=%d", rc);
    }
}

static void read_data()
{
    esp_ble_conn_data_t inbuff = {
        .type = BLE_CONN_UUID_TYPE_16,
        .uuid = {
            .uuid16 = GATT_ENV_SNS_UUID,
        },
        .data = NULL,
        .data_len = 0,
    };
    esp_err_t rc = esp_ble_conn_read(&inbuff);
    if (rc == 0) {
        ESP_LOGI(MAIN_TAG, "Read data success!");
        ESP_LOG_BUFFER_CHAR(MAIN_TAG, inbuff.data, inbuff.data_len);
    } else {
        ESP_LOGE(MAIN_TAG, "Error in reading characteristic rc=%d", rc);
    }
}

static void subscribe()
{
    esp_ble_conn_data_t inbuff = {
        .type = BLE_CONN_UUID_TYPE_16,
        .uuid = {
            .uuid16 = GATT_ENV_SNS_UUID,
        },
        .data = NULL,
        .data_len = 0,
    };
    esp_err_t rc = esp_ble_conn_subscribe(ESP_BLE_CONN_DESC_CIENT_CONFIG, &inbuff);
    if (rc == 0) {
        ESP_LOGI(MAIN_TAG, "Read data success!");
        ESP_LOG_BUFFER_CHAR(MAIN_TAG, inbuff.data, inbuff.data_len);
    } else {
        ESP_LOGE(MAIN_TAG, "Error in reading characteristic rc=%d", rc);
    }
}

static void app_ble_conn_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data)
{
    if (base != BLE_CONN_MGR_EVENTS) {
        return;
    }

    switch (id) {
    case ESP_BLE_CONN_EVENT_CONNECTED:
        ESP_LOGI(MAIN_TAG, "ESP_BLE_CONN_EVENT_CONNECTED\n");
        break;
    case ESP_BLE_CONN_EVENT_DISCONNECTED:
        ESP_LOGI(MAIN_TAG, "ESP_BLE_CONN_EVENT_DISCONNECTED\n");
        break;
    case ESP_BLE_CONN_EVENT_DATA_RECEIVE:
        ESP_LOGI(MAIN_TAG, "ESP_BLE_CONN_EVENT_DATA_RECEIVE\n");
        //handle_incoming_data(event_data);
        break;
    case ESP_BLE_CONN_EVENT_DISC_COMPLETE:
        ESP_LOGI(MAIN_TAG, "ESP_BLE_CONN_EVENT_DISC_COMPLETE\n");
        vTaskDelay(1000);
        subscribe();
        break;
    default:
        break;
    }
}

void app_main(void)
{
    esp_ble_conn_config_t config = {
        .device_name = "ULTRASONIC",
        .broadcast_data = "CALYPSO"
    };

    esp_err_t ret;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_event_loop_create_default();
    esp_event_handler_register(BLE_CONN_MGR_EVENTS, ESP_EVENT_ANY_ID, app_ble_conn_event_handler, NULL);

    esp_ble_conn_init(&config);
    if (esp_ble_conn_start() != ESP_OK) {
        ESP_LOGI(MAIN_TAG, "Conection start failed: Shutting Down");
        esp_ble_conn_stop();
        esp_ble_conn_deinit();
        esp_event_handler_unregister(BLE_CONN_MGR_EVENTS, ESP_EVENT_ANY_ID, app_ble_conn_event_handler);
    }
}