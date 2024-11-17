/*
 * Library for formatting BLE connection information
 *
 * Based on NIMBLE blecent example app
 */

#ifndef H_BLE_UTILS_
#define H_BLE_UTILS_

#pragma once
#ifdef __cplusplus
extern "C" {
#endif

void print_bytes(const uint8_t *bytes, int len);
void print_mbuf(const struct os_mbuf *om);
char *addr_str(const void *addr);
void print_uuid(const ble_uuid_t *uuid);
void print_conn_desc(const struct ble_gap_conn_desc *desc);
void print_adv_fields(const struct ble_hs_adv_fields *fields);

#ifdef __cplusplus
}
#endif

#endif