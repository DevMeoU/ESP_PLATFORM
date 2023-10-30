/*
 * app_ble.h
 *
 *  Created on: Feb 22, 2021
 *      Author: laian
 */

#ifndef __APP_BLE_H
#define __APP_BLE_H

#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_bt.h"
#include "esp_blufi_api.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"

extern bool g_blufiConnected;
extern bool g_blufiOver;

void  app_blufi_init();
void  app_blufi_deinit();

void  app_bluedroid_init();
void  app_bt_controller_init();

void  app_ble_blufi_begin();
bool  app_ble_blufi_end();
bool  app_ble_blufi_force_end();
void  app_ble_blufi_request_end();
void  app_ble_blufi_unlimited();

void  app_ble_blufi_send_custom_msg(const uint8_t* msg, int msg_len);
void  app_ble_blufi_handle_custom_msg(const uint8_t* msg, int msg_len);

void  app_ble_send_wifi_conn_report(void);

bool app_ble_blufi_is_unlimit();

#define APP_ESP_ERROR_RETURN( __ERR, tag, format, ...) \
	do {\
		if (__ERR != ESP_OK) {\
			LREP_ERROR(tag, format, ##__VA_ARGS__);\
			return;\
		}\
	} while(0)

#endif /* __APP_BLE_H */
