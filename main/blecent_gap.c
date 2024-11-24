/*
 * Library for managing GAP connections in nimble on ESP32
 *
 * Based on NIMBLE blecent example app
 */

#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

#include "blecent_gap.h"
#include "ble_utils.h"
#include "gatt_peer.h"

#define PEER_ADDR "ADDR_ANY"

static const char *tag = "blecent_gap";

static uint8_t peer_addr[6];
static int mbuf_len_total;

blecent_notify_fn * notify_callback = NULL;

static int blecent_gap_event(struct ble_gap_event *event, void *arg);
static void blecent_connect_if_interesting(const struct ble_gap_disc_desc *disc);
static void blecent_on_disc_complete(const struct peer *peer, int status, void *arg);
static int blecent_should_connect(const struct ble_gap_disc_desc *disc);
static int blecent_gap_event(struct ble_gap_event *event, void *arg);
static void blecent_on_disc_complete(const struct peer *peer, int status, void *arg);
static int blecent_subscribe_all(const struct peer *peer);
static int blecent_subscribe_uuid16(const struct peer *peer, uint16_t svc_uuid, uint16_t chr_uuid);

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that is
 * established.  central uses the same callback for all connections.
 *
 * @param event                 The event being signalled.
 * @param arg                   Application-specified argument; unused by
 *                                  gattc.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int blecent_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        ESP_LOGI(tag, "Event DISC ");
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data,
                                     event->disc.length_data);
        if (rc != 0) {
            return 0;
        }

        /* An advertisement report was received during GAP discovery. */
        print_adv_fields(&fields);

        /* Try to connect to the advertiser if it looks interesting. */
        blecent_connect_if_interesting(&event->disc);
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        if (event->connect.status == 0) {
            /* Connection successfully established. */
            /* XXX Set packet length in controller for better throughput */
            ESP_LOGI(tag, "Connection established ");

            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            print_conn_desc(&desc);

            /*
            rc = ble_gap_update_params(event->connect.conn_handle, &conn_params);
            if (rc != 0) {
                ESP_LOGE(tag, "Failed to update params; rc = %d", rc);
            }
            */

            /* Remember peer. */
            rc = peer_add(event->connect.conn_handle);
            if (rc != 0) {
                ESP_LOGE(tag, "Failed to add peer; rc = %d", rc);
                return 0;
            }

            /* Perform service discovery. */
            rc = peer_disc_all(event->connect.conn_handle,
                               blecent_on_disc_complete, NULL);
            if (rc != 0) {
                ESP_LOGE(tag, "Failed to discover services; rc = %d", rc);
                return 0;
            }
        } else {
            /* Connection attempt failed; resume scanning. */
            ESP_LOGE(tag, "Error: Connection failed; status = %d",
                     event->connect.status);
            blecent_init_scan();
        }

        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        /* Connection terminated. */
        ESP_LOGI(tag, "disconnect; reason=%d", event->disconnect.reason);
        print_conn_desc(&event->disconnect.conn);
        ESP_LOGI(tag, " ");

        /* Forget about peer. */
        peer_delete(event->disconnect.conn.conn_handle);
        /* Resume scanning. */
        blecent_init_scan();
        return 0;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        ESP_LOGI(tag, "discovery complete; reason = %d",
                 event->disc_complete.reason);
        return 0;

    case BLE_GAP_EVENT_NOTIFY_RX:
        /* Peer sent us a notification or indication. */
        mbuf_len_total = mbuf_len_total + OS_MBUF_PKTLEN(event->notify_rx.om);
        ESP_LOGI(tag, "received %s; conn_handle = %d attr_handle = %d "
                 "attr_len = %d ; Total length = %d",
                 event->notify_rx.indication ?
                 "indication" :
                 "notification",
                 event->notify_rx.conn_handle,
                 event->notify_rx.attr_handle,
                 OS_MBUF_PKTLEN(event->notify_rx.om),
                 mbuf_len_total);

        if (notify_callback != NULL) {
            notify_callback(event);
        }
        return 0;

    default:
        return 0;
    }
}

/**
 * Called when service discovery of the specified peer has completed.
 */
static void blecent_on_disc_complete(const struct peer *peer, int status, void *arg)
{

    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        ESP_LOGE(tag, "Error: Service discovery failed; status=%d "
                 "conn_handle=%d\n", status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    /* Service discovery has completed successfully.  Now we have a complete
     * list of services, characteristics, and descriptors that the peer
     * supports.
     */
    ESP_LOGI(tag, "Service discovery complete; status=%d "
             "conn_handle=%d\n", status, peer->conn_handle);

    /* Now perform three GATT procedures against the peer: read,
     * write, and subscribe to notifications depending upon user input.
     */
    //blecent_read_write_subscribe(peer);
    blecent_subscribe_all(peer);
}


/**
 * Indicates whether we should try to connect to the sender of the specified
 * advertisement.  The function returns a positive result if the device
 * advertises connectability and support for the THRPT service i.e. 0x0001.
 */
static int blecent_should_connect(const struct ble_gap_disc_desc *disc)
{
    struct ble_hs_adv_fields fields;
    int rc;

    rc = ble_hs_adv_parse_fields(&fields, disc->data, disc->length_data);
    if (rc != 0) {
        return 0;
    }

    if (strlen(PEER_ADDR) && (strncmp(PEER_ADDR, "ADDR_ANY", strlen("ADDR_ANY")) != 0)) {
        ESP_LOGI(tag, "Peer address from config: %s", PEER_ADDR);
        /* Convert string to address */
        sscanf(PEER_ADDR, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
               &peer_addr[5], &peer_addr[4], &peer_addr[3],
               &peer_addr[2], &peer_addr[1], &peer_addr[0]);
        if (memcmp(peer_addr, disc->addr.val, sizeof(disc->addr.val)) != 0) {
            return 0;
        }
    }
    
    char serv_name[] = "ULTRASONIC";
    if (fields.name != NULL) {
        ESP_LOGI(tag, "Device Name = %s", (char *)fields.name);

        if (memcmp(fields.name, serv_name, fields.name_len) == 0) {
            ESP_LOGI(tag, "central connect to `UTLRASONIC` success");
            return 1;
        }
    }
    return 0;
}

/**
 * Connects to the sender of the specified advertisement of it looks
 * interesting.  A device is "interesting" if it passes the critera from blecent_should_connect
 */
static void blecent_connect_if_interesting(const struct ble_gap_disc_desc *disc)
{
    uint8_t own_addr_type;
    int rc;

    /* Don't do anything if we don't care about this advertiser. */
    if (!blecent_should_connect(disc)) {
        return;
    }

#if !(MYNEWT_VAL(BLE_HOST_ALLOW_CONNECT_WITH_SCAN))
    /* Scanning must be stopped before a connection can be initiated. */
    rc = ble_gap_disc_cancel();
    if (rc != 0) {
        MODLOG_DFLT(DEBUG, "Failed to cancel scan; rc=%d\n", rc);
        return;
    }
#endif
    
    /* Figure out address to use for connect (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(tag, "error determining address type; rc=%d", rc);
        return;
    }

    /* Try to connect the the advertiser.  Allow 30 seconds (30000 ms) for
     * timeout.
     */
    rc = ble_gap_connect(own_addr_type, &disc->addr, 30000, NULL,
                         blecent_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(tag, "Error: Failed to connect to device; addr_type=%d "
                 "addr=%s; rc=%d\n",
                 disc->addr.type, addr_str(disc->addr.val), rc);
        return;
    }
}
static int blecent_subscribe_all(const struct peer *peer)
{
    for(int i=0; i < BLECENT_MAX_SUBSCRIPTIONS; i++) {
        break;
    }
    //FIXME
    return blecent_subscribe_uuid16(peer, 0x180D, 0x2A39);
}

static int blecent_subscribe_uuid16(const struct peer *peer, uint16_t svc_uuid, uint16_t chr_uuid)
{
    const struct peer_dsc *dsc;

    dsc = peer_dsc_find_uuid(peer,
                             BLE_UUID16_DECLARE(svc_uuid),
                             BLE_UUID16_DECLARE(chr_uuid),
                             BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16));
    
    if (dsc == NULL) {
        ESP_LOGE(tag, "Error: Peer doesn't support the requested characteristic");
        goto err;
    }

    uint8_t value[2] = {1, 0};/*To subscribe to notifications*/
    int rc;

    rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                              value, sizeof value, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(tag, "Error: Failed to subscribe to characteristic; "
                 "rc = %d", rc);
        goto err;
    }

    return 0;
err:
    /* Terminate the connection. */
    return ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}

void blecent_init_scan(void)
{
    uint8_t own_addr_type;
    struct ble_gap_disc_params disc_params;
    int rc;

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        ESP_LOGE(tag, "error determining address type; rc=%d", rc);
        return;
    }

    /* Tell the controller to filter duplicates; we don't want to process
     * repeated advertisements from the same device.
     */
    disc_params.filter_duplicates = 1;

    /**
     * Perform a passive scan.  I.e., don't send follow-up scan requests to
     * each advertiser.
     */
    disc_params.passive = 1;

    /* Use defaults for the rest of the parameters. */
    disc_params.itvl = 0;
    disc_params.window = 0;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
                      blecent_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(tag, "Error initiating GAP discovery procedure; rc=%d",
                 rc);
    }
}

void blecent_set_notify_cb(blecent_notify_fn *cb)
{
    notify_callback = cb;
}

void blecent_set_subscription_list(struct blecent_subscription_t *subscription_list[BLECENT_MAX_SUBSCRIPTIONS])
{

}