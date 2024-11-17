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

typedef struct 
{
    /* data */
} blecent_subscription_list_t;


/** @brief Set the callback for handling notifications (optional)
 *
 * @param cb fucntion pointer to the desired callback
 */
void blecent_set_notify_cb(blecent_notify_fn *cb);

/** @brief Set the list of attributes to subscribe to
 *
 * @param cb fucntion pointer to the desired callback
 */
void blecent_set_subscription_list(blecent_subscription_list_t list);

/** @brief Configures and initiates GAP discovery scan
 */
void blecent_init_scan(void);


#ifdef __cplusplus
}
#endif

#endif