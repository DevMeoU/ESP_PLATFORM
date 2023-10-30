/*
 * app_accesspoint.h
 *
 *  Created on: Dec 12, 2019
 *      Author: Admin
 */

#ifndef MAIN_APP_WIFI_MODE_H_
#define MAIN_APP_WIFI_MODE_H_

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_netif.h"
#include "esp_smartconfig.h"
#include "app_time_server.h"
#include "app_gpio.h"
#include "app_mqtt.h"
#include "app_ota.h"

#define APP_WIFI_SOFTAP_PASS 		""
#define APP_WIFI_STA_HOST_NAME		"Vconnex"

extern bool bSoftApOver;

typedef union {
	uint32_t u32Ip;
	uint8_t pu8Ip[4];
} app_wifi_ip_t;

typedef enum {
	APP_WIFI_SOFT_AP,
	APP_WIFI_STATION,
	APP_WIFI_SOFT_AP_STATION,
} app_wifi_mode_t;

extern bool g_espGotIp;

void app_wifi_init();
bool app_wifi_config_mode_end();
void app_wifi_softap();
void app_wifif_softap_prepare(wifi_config_t* p_wifi_config);
const char* app_wifi_get_mac_str();
const char* app_wifi_get_broadcast_ip();

bool app_wifi_udp_ready();
void app_wifi_set_station(const char* ssid, const char* pass);
void app_wifi_refresh();

#endif /* MAIN_APP_WIFI_MODE_H_ */
