/*
 * app_wifi_mode.c
 *
 *  Created on: Dec 12, 2019
 *      Author: Admin
 */

#include "app_wifi_mode.h"
#include "app_time_server.h"
#include "app_ble.h"
#include "config.h"

static void app_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

static int iAppWifiStaRetryNum = 0;
static uint8_t pu8Mac[6] = {0};
static char    strMac[20] = {0};
static char    strBroadCastIp[64] = {0};
static char    strStationIp[64] = {0};

esp_netif_t* g_esp_sta_netif = NULL;
esp_netif_t* g_esp_ap_netif = NULL;
bool g_espGotIp = false;

bool 	bGotSSID = false;
uint64_t configCancelTime = 0;

static TaskHandle_t xSoftApConfigHanle = NULL;
static void task_softap(void* parm);
bool bSoftApOver  = true;
static int softApCount = 0;

wifi_config_t  wifi_cfg;
static int iWifiMode;
void app_wifi_init() {
	ESP_ERROR_CHECK(esp_netif_init());
	esp_event_loop_create_default();

	g_esp_sta_netif = esp_netif_create_default_wifi_sta();
	g_esp_ap_netif = esp_netif_create_default_wifi_ap();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
#ifndef WIFI_FIXED
    if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg) == ESP_OK) {
    	LREP(__FUNCTION__, "Found ssid:\"%s\"",     (const char*) wifi_cfg.sta.ssid);
		LREP(__FUNCTION__, "Found password:\"%s\"", (const char*) wifi_cfg.sta.password);
		LREP(__FUNCTION__, "Scan method %s" , wifi_cfg.sta.scan_method == WIFI_FAST_SCAN ? "WIFI_FAST_SCAN" : "WIFI_ALL_CHANNEL_SCAN");
		LREP(__FUNCTION__, "bssid_set %s, %02x:%02x:%02x:%02x:%02x:%02x", wifi_cfg.sta.bssid_set ? "YES" : "NO"
				, wifi_cfg.sta.bssid[0], wifi_cfg.sta.bssid[1], wifi_cfg.sta.bssid[2], wifi_cfg.sta.bssid[3], wifi_cfg.sta.bssid[4], wifi_cfg.sta.bssid[5]);
    }
#else
    strcpy((char*)wifi_cfg.sta.ssid, WIFI_FIXED_SSID);
    strcpy((char*)wifi_cfg.sta.password, WIFI_FIXED_PASS);
#endif
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &app_wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &app_wifi_event_handler, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
#ifdef WIFI_FIXED
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
#endif
    iWifiMode = APP_WIFI_STATION;
    ESP_ERROR_CHECK(esp_wifi_start());
}

bool app_wifi_config_mode_end() {
	if (!bSoftApOver)  {
		LREP_WARNING(__FUNCTION__,"");
		esp_wifi_set_mode(WIFI_MODE_STA);
		iWifiMode = APP_WIFI_STATION;
		TASK_DELETE_SAFE(xSoftApConfigHanle);
		bSoftApOver = true;
		if (app_mqtt_check_cloud_setting() == false) {
			 app_ble_blufi_unlimited(); g_bRequestBluFi = true;
		}
		else {
			app_mqtt_start();
		}

		gpio_set_led_normal();
		return true;
	}
	return app_ble_blufi_end();
}

void app_wifi_softap() {
	LREP(__FUNCTION__,"");
	bSoftApOver = false; bGotSSID = false;
	app_mqtt_stop();
	if (app_ble_blufi_is_unlimit()) app_ble_blufi_force_end();

    wifi_config_t wifi_config;
    app_wifif_softap_prepare(&wifi_config);

	esp_wifi_set_mode(WIFI_MODE_AP);
	iWifiMode = APP_WIFI_SOFT_AP;
    esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);

    xTaskCreate(task_softap, "task_softap", 4096, NULL, 4, &xSoftApConfigHanle);
}

void app_wifif_softap_prepare(wifi_config_t* p_wifi_config) {
	uint8_t eth_mac[6];
	esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
	memset(p_wifi_config, 0, sizeof(wifi_config_t));
	snprintf((char*)p_wifi_config->ap.ssid, sizeof(p_wifi_config->ap.ssid), "VCONNEX-%02X%02X", eth_mac[4], eth_mac[5]);
	LREP_WARNING(__FUNCTION__, "%s", (char*)p_wifi_config->ap.ssid);
	p_wifi_config->ap.ssid_len = strlen((char*)p_wifi_config->ap.ssid);
	strncpy((char*)p_wifi_config->ap.password,  APP_WIFI_SOFTAP_PASS, sizeof(p_wifi_config->ap.password));
	p_wifi_config->ap.max_connection = 4;
	p_wifi_config->ap.beacon_interval = 100;
	p_wifi_config->ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
	if (strlen(APP_WIFI_SOFTAP_PASS) == 0) {
		p_wifi_config->ap.authmode = WIFI_AUTH_OPEN;
	}
}

static void task_softap(void* parm) {
	softApCount = 0;
	LREP_WARNING(__FUNCTION__,"START");
	for(;;) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		softApCount++;
		if(bSoftApOver || softApCount > 120) {
			break;
		}
	}
	LREP(__FUNCTION__, "softap over");
	bSoftApOver = true;
	esp_wifi_set_mode(WIFI_MODE_STA);
	iWifiMode = APP_WIFI_STATION;

	if (app_ble_blufi_is_unlimit() && app_mqtt_check_cloud_setting() == false) { LREP(__FUNCTION__, "Blufi enable"); g_bRequestBluFi = true;}
	else {LREP(__FUNCTION__, "mqtt enable"); app_mqtt_start();}
	gpio_set_led_normal();
	TASK_DELETE_SAFE(xSoftApConfigHanle);
}

void app_wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	static bool firsttime = true;
	if (event_base == WIFI_EVENT) {
		if (event_id == WIFI_EVENT_AP_STACONNECTED) {
			wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
			LREP(__FUNCTION__, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
		} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
			wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
			LREP(__FUNCTION__, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
		}
		else if (event_id == WIFI_EVENT_STA_START) {
			if (firsttime) {
				LREP(__FUNCTION__, "Set host name");
				tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, APP_WIFI_STA_HOST_NAME);
				firsttime = false;
			}
			esp_wifi_connect();
		}
		else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
			if(g_otaRunning) {
				esp_restart();
			}
			LREP(__FUNCTION__, "retry to connect to the AP");
			esp_wifi_connect();
			g_espGotIp = false;
			LREP_ERROR(__FUNCTION__,"WIFI_EVENT_STA_DISCONNECTED");
		}
		else if (event_id == WIFI_EVENT_AP_START) {
			LREP_WARNING(__FUNCTION__, "WIFI_EVENT_AP_START");
		}
		else if (event_id == WIFI_EVENT_AP_STOP) {
			LREP_ERROR(__FUNCTION__,"WIFI_EVENT_AP_STOP");
		}
	}
	else if(event_base == IP_EVENT) {
		if (event_id == IP_EVENT_STA_GOT_IP) {
			ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
			memset(strBroadCastIp, 0 , sizeof(strBroadCastIp));
			memset(strStationIp, 0, sizeof(strStationIp));
			app_wifi_ip_t gatewayIp;
			app_wifi_ip_t stationIp;
			stationIp.u32Ip = event->ip_info.ip.addr;
			gatewayIp.u32Ip = event->ip_info.gw.addr;
			sprintf(strStationIp, "%u:%u:%u:%u", stationIp.pu8Ip[0], stationIp.pu8Ip[1], stationIp.pu8Ip[2], stationIp.pu8Ip[3]);
			sprintf(strBroadCastIp, "%u:%u:%u:%u", gatewayIp.pu8Ip[0], gatewayIp.pu8Ip[1], gatewayIp.pu8Ip[2], 255);
			LREP_WARNING(__FUNCTION__,"got ip:%s", strStationIp);
			LREP_WARNING(__FUNCTION__, "broadcast ip %s", strBroadCastIp);
			iAppWifiStaRetryNum = 0;
			g_espGotIp = true;
			if (!g_blufiOver) app_ble_send_wifi_conn_report();
			else app_cmd_handle_GetDeviceID(NULL, CMD_TYPE_UDP);
		}
	}
}

const char* app_wifi_get_mac_str() {
	if(pu8Mac[0] == 0) {
		ESP_ERROR_CHECK(esp_efuse_mac_get_default(pu8Mac));
		LREP_WARNING(__FUNCTION__,"Mac address %02X:%02X:%02X:%02X:%02X:%02X", pu8Mac[0], pu8Mac[1], pu8Mac[2], pu8Mac[3], pu8Mac[4], pu8Mac[5]);
		sprintf(strMac,"%02X%02X%02X%02X%02X%02X",pu8Mac[0], pu8Mac[1], pu8Mac[2], pu8Mac[3], pu8Mac[4], pu8Mac[5]);
	}
	return strMac;
}

const char* app_wifi_get_broadcast_ip() {
	if(strlen(strBroadCastIp) == 0) return "255:255:255:255";
	else return strBroadCastIp;
}

bool app_wifi_udp_ready() {
	if(iWifiMode == APP_WIFI_STATION) {
		return g_espGotIp;
	}
	else if (iWifiMode == APP_WIFI_SOFT_AP || iWifiMode == APP_WIFI_SOFT_AP_STATION) {
		return true;
	}
	return false;
}

void app_wifi_set_station(const char* ssid, const char* pass) {
	wifi_config_t wifi_config;
	memset(&wifi_config, 0, sizeof(wifi_config));
	strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
	strncpy((char*)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password) -1);
	wifi_config.sta.bssid_set = false;
	esp_wifi_disconnect();
	esp_wifi_set_mode(WIFI_MODE_APSTA);
	iWifiMode = APP_WIFI_SOFT_AP_STATION;
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_connect();
	bGotSSID = true;
}

void app_wifi_refresh() {
	if (esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg) == ESP_OK) {
		if (strlen((char*)wifi_cfg.sta.ssid) == 0) return;
	}
	esp_wifi_stop();
	esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &app_wifi_event_handler);
	esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &app_wifi_event_handler);
	esp_wifi_deinit();
	esp_netif_destroy(g_esp_sta_netif);
	esp_netif_destroy(g_esp_ap_netif);
	esp_event_loop_delete_default();
	esp_netif_deinit();
	app_wifi_init();
}
