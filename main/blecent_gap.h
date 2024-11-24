/*
 * Library for managing GATT Peer attributes
 *
 * Based on NIMBLE blecent example app
 */

#ifndef H_BLECENT_GAP_
#define H_BLECENT_GAP_

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define BLECENT_MAX_SUBSCRIPTIONS 10

/** @brief Callback for handling recieved Notification
 *
 * @param event notify_rx GAP event
 */
typedef void blecent_notify_fn(struct ble_gap_event *event);

/** @brief Struct for storing Generic Attribute Subscription parameters
 */
struct blecent_subscription_t {
    ble_uuid_t* svc_uuid;
    ble_uuid_t* attr_uuid;
};


/** @brief Set the callback for handling notifications (optional)
 *
 * @param cb fucntion pointer to the desired callback
 */
void blecent_set_notify_cb(blecent_notify_fn *cb);

/** @brief Set the list of attributes to subscribe to
 *
 * @param cb fucntion pointer to the desired callback
 */
void blecent_set_subscription_list(struct blecent_subscription_t *list[BLECENT_MAX_SUBSCRIPTIONS]);

/** @brief Configures and initiates GAP discovery scan
 */
void blecent_init_scan(void);


#ifdef __cplusplus
}
#endif

#endif