/*
 * SPDX-FileCopyrightText: 2019-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

/* Core OS APIs */
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* BLE */
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "ble_utils.h"
#include "gatt_peer.h"
#include "blecent_gap.h"

#define GATT_HR_UUID 0x180D
#define GATT_HRCP_UUID 0x2A39

#define DEVICE_NAME "ULTRASONIC"
#define MANUFACTURER_NAME "CALYPSO"

static const char *tag = "app_main";

static SemaphoreHandle_t xSemaphore;
void ble_store_config_init(void);

/* Wind data will arrive as a ten byte notification */
static void handle_wind_data(uint8_t wind_data[10])
{
    float wind_speed_ms = ((uint16_t)((wind_data[1] << 8) + wind_data[0]))/100.0f;
    uint16_t wind_direction_deg = (wind_data[3] << 8) + wind_data[2];
    uint8_t battery_pcnt = wind_data[4] * 10;
    int8_t air_temp_c = wind_data[5] - 100;

    ESP_LOGI(tag, "Dir: %u, Spd: %f, Bat: %u, Temp: %i", wind_direction_deg, wind_speed_ms, battery_pcnt, air_temp_c);
}

static void process_incoming_notification(struct ble_gap_event *event)
{
    if (event->type != BLE_GAP_EVENT_NOTIFY_RX) {
        ESP_LOGE(tag, "Invalid Notification Event");
        return;
    }

    if (OS_MBUF_PKTLEN(event->notify_rx.om) == 10) {
      // Wind Update Received
        uint8_t wind_data[10];
        memcpy(wind_data, event->notify_rx.om->om_data, OS_MBUF_PKTLEN(event->notify_rx.om));
        handle_wind_data(wind_data);
    }else{
        ESP_LOGI(tag, "ESP_GATTC_NOTIFY_EVT, unexpected value received (length!=10)");
    }
}

static void blecent_on_sync(void)
{
    int rc;

    /* Make sure we have proper identity address set (public preferred) */
    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Begin scanning for a peripheral to connect to. */
    blecent_init_scan();
}

static void blecent_on_reset(int reason)
{
    ESP_LOGE(tag, "Resetting state; reason=%d", reason);
}

void blecent_host_task(void *param)
{
    ESP_LOGI(tag, "BLE Host Task Started");
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore);

    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();
    vSemaphoreDelete(xSemaphore);
    nimble_port_freertos_deinit();
}

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

     ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(tag, "Failed to init nimble %d ", ret);
        return;
    }

    /* Configure the host. */
    ble_hs_cfg.reset_cb = blecent_on_reset;
    ble_hs_cfg.sync_cb = blecent_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Provide GAP manager with notification callback function specific to our application*/
    blecent_set_notify_cb(process_incoming_notification);

    /* Initialize data structures to track connected peers. */
    int rc = peer_init(MYNEWT_VAL(BLE_MAX_CONNECTIONS), 64, 64, 64);
    assert(rc == 0);

    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set("ESP-NMEA");
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    /* Begin the nimble host task */
    nimble_port_freertos_init(blecent_host_task);

}