/*
 * app_ble.c
 *
 *  Created on: Feb 22, 2021
 *      Author: laian
 */
#include "esp_wifi.h"

#include "app_ble.h"
#include "app_wifi_mode.h"
#include "app_cmd_mgr.h"
#include "app_mqtt.h"
#include "config.h"

#define BLUFI_DEVICE_NAME            "BLUFI_DEVICE"
static uint8_t example_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
};

//static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
static esp_ble_adv_data_t blufi_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 16,
    .p_service_uuid = example_service_uuid128,
    .flag = 0x6,
};

static esp_ble_adv_params_t blufi_adv_params = {
    .adv_int_min        = 0x100,
    .adv_int_max        = 0x100,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    //.peer_addr            =
    //.peer_addr_type       =
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static wifi_config_t sta_config;

static uint8_t blufi_server_if = 0;
static uint16_t blufi_conn_id = 0;

bool g_blufiConnected = false;
bool g_blufiOver = true;

static void app_blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param);
static void app_blufi_negotiate(uint8_t *data, int len, uint8_t **output_data, int *output_len, bool *need_free);

static esp_blufi_callbacks_t blufi_callbacks = {
    .event_cb = app_blufi_event_callback,
    .negotiate_data_handler = app_blufi_negotiate,
    .encrypt_func = NULL,
    .decrypt_func = NULL,
    .checksum_func = NULL,
};

static TaskHandle_t xBlufiTaskHandle = NULL;
static void task_blufi(void* pvParameters);

static void app_ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
    	LREP(__FUNCTION__, "BLE_ADV_DATA_SET_COMPLETE_EVT");
        break;
    default:
        break;
    }
}

void  app_blufi_init() {
	esp_err_t ret;
	ret = esp_bluedroid_init();
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "init bluedroid failed: %s", esp_err_to_name(ret));

	ret = esp_bluedroid_enable();
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "enable bluedroid failed: %s", esp_err_to_name(ret));

	const uint8_t* pBtAddr = esp_bt_dev_get_address();
	if (pBtAddr == NULL) {
		LREP_ERROR(__FUNCTION__, "bluetooth stack is not enabled");
		return;
	}
	else {
		LREP_WARNING(__FUNCTION__, "bluetooth device address %02X:%02X:%02X:%02X:%02X:%02X", *pBtAddr, *(pBtAddr+1), *(pBtAddr+2), *(pBtAddr+3), *(pBtAddr+4), *(pBtAddr+5));
	}

	ret = esp_ble_gap_register_callback(app_ble_gap_event_handler);
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "gap register failed: %s", esp_err_to_name(ret));

	ret = esp_blufi_register_callbacks(&blufi_callbacks);
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "blufi register failed: %s", esp_err_to_name(ret));

	ret = esp_blufi_profile_init();
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "blufi profi init failed: %s", esp_err_to_name(ret));
}

void  app_blufi_deinit() {
	esp_err_t ret;
	ret = esp_blufi_profile_deinit();
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "blufi profi deinit failed: %s", esp_err_to_name(ret));
	ret = esp_bluedroid_disable();
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "disable bluedroid failed: %s", esp_err_to_name(ret));
	delay_ms(200); //this delay is mandatory (unknow reason)
	ret = esp_bluedroid_deinit();
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "deinit bludroid failed %s", esp_err_to_name(ret));
}

void  app_bluedroid_init() {
	esp_err_t ret;

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	APP_ESP_ERROR_RETURN(ret , __FUNCTION__, "initialize bt controller failed: %s", esp_err_to_name(ret));

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "enable bt controller failed: %s", esp_err_to_name(ret));

	ret = esp_bluedroid_init();
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "init bluedroid failed: %s", esp_err_to_name(ret));

	ret = esp_bluedroid_enable();
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "enable bluedroid failed: %s", esp_err_to_name(ret));
}

void  app_bt_controller_init() {
	esp_err_t ret;

	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init(&bt_cfg);
	APP_ESP_ERROR_RETURN(ret , __FUNCTION__, "initialize bt controller failed: %s", esp_err_to_name(ret));

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "enable bt controller failed: %s", esp_err_to_name(ret));
}

static bool bBlufiRequestEnd = false;
static bool bBlufiUnlimited = false;

void  app_ble_blufi_begin() {
	static bool firstrun = true;
	if (firstrun) {
		app_bt_controller_init();
		firstrun = false;
	}

	if (g_blufiOver == false) app_ble_blufi_force_end();//{ LREP_ERROR(__FUNCTION__, "Blufi is not over"); return;}

	app_mqtt_stop();

	app_blufi_init();
	if (esp_ble_gap_start_advertising(&blufi_adv_params) != ESP_OK) {
		LREP_WARNING(__FUNCTION__, "Blufi error");
		esp_restart();
	}
	g_blufiOver = false;
	bBlufiRequestEnd = false;
	bBlufiUnlimited = false;

	xTaskCreate(&task_blufi,"task_blufi",4096, NULL, 3, &xBlufiTaskHandle);
}

bool  app_ble_blufi_end() {
	if (bBlufiUnlimited) return false;
	else if (app_mqtt_check_cloud_setting() == false) {
		app_ble_blufi_unlimited();
		gpio_set_led_normal();
		return true;
	}
	else if (g_blufiOver == false) {
		g_blufiOver = true;
		esp_ble_gap_stop_advertising();
		if(g_blufiConnected) esp_blufi_close(blufi_server_if, blufi_conn_id);
		TASK_DELETE_SAFE(xBlufiTaskHandle);
		app_blufi_deinit();

		app_mqtt_start();
		gpio_set_led_normal();
		return true;
	}
	return false;
}

bool app_ble_blufi_force_end() {
	if (g_blufiOver == false) {
		g_blufiOver = true;
		esp_ble_gap_stop_advertising();
		if(g_blufiConnected) esp_blufi_close(blufi_server_if, blufi_conn_id);
		TASK_DELETE_SAFE(xBlufiTaskHandle);
		app_blufi_deinit();
		return true;
	}
	return false;
}

void  app_ble_blufi_request_end() {
	bBlufiRequestEnd = true;
}

void  app_ble_blufi_unlimited() {
	bBlufiUnlimited = true;
}

bool app_ble_blufi_is_unlimit() {
	return bBlufiUnlimited;
}

static void app_blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param) {
//	LREP(__FUNCTION__, "event %d", event);
	switch (event) {
	case ESP_BLUFI_EVENT_INIT_FINISH:
		LREP(__FUNCTION__, "BLUFI init finish");
		uint8_t eth_mac[6];
		esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
		char ssid_with_mac[20];
		snprintf(ssid_with_mac, sizeof(ssid_with_mac), "VCONNEX-%02X%02X", eth_mac[4], eth_mac[5]);
		esp_ble_gap_set_device_name(ssid_with_mac);
		esp_ble_gap_config_adv_data(&blufi_adv_data);
		break;
	case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
		LREP(__FUNCTION__, "BLUFI Set WIFI opmode %d", param->wifi_mode.op_mode);
		if ( param->wifi_mode.op_mode == WIFI_MODE_STA) {
			ESP_ERROR_CHECK( esp_wifi_set_mode(param->wifi_mode.op_mode) );
		}
		else LREP_WARNING(__FUNCTION__, "Only support STA mode");
		break;
	case ESP_BLUFI_EVENT_BLE_CONNECT:
		LREP(__FUNCTION__, "BLUFI ble connect");
		gpio_set_blufi_connected();
		blufi_server_if = param->connect.server_if;
		blufi_conn_id = param->connect.conn_id;
		esp_ble_gap_stop_advertising();
		g_blufiConnected = true;
		break;
	case ESP_BLUFI_EVENT_BLE_DISCONNECT:
		LREP(__FUNCTION__,"BLUFI ble disconnect");
		if (bBlufiUnlimited) gpio_set_led_normal();
		else gpio_set_led_auto();
		g_blufiConnected = false;
		if (g_blufiOver == false) esp_ble_gap_start_advertising(&blufi_adv_params);
		break;
	case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
		LREP(__FUNCTION__, "BLUFI requset wifi connect to AP");
		esp_wifi_disconnect();
		esp_wifi_connect();
		break;
	case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
		LREP(__FUNCTION__, "BLUFI request Wifi Status");
		wifi_mode_t mode;
		esp_wifi_get_mode(&mode);
		if (g_espGotIp) {
			esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, NULL);
		}
		else {
			esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
		}
		break;
	case ESP_BLUFI_EVENT_RECV_STA_BSSID:
		memcpy(sta_config.sta.bssid, param->sta_bssid.bssid, 6);
		sta_config.sta.bssid_set = 1;
		esp_wifi_set_config(WIFI_IF_STA, &sta_config);
		LREP(__FUNCTION__,"Recv STA BSSID %s", sta_config.sta.ssid);
		break;
	case ESP_BLUFI_EVENT_RECV_STA_SSID:
		strncpy((char *)sta_config.sta.ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
		sta_config.sta.ssid[param->sta_ssid.ssid_len] = '\0';
		esp_wifi_set_config(WIFI_IF_STA, &sta_config);
		LREP(__FUNCTION__, "Recv STA SSID %s", sta_config.sta.ssid);
		break;
	case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
		strncpy((char *)sta_config.sta.password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
		sta_config.sta.password[param->sta_passwd.passwd_len] = '\0';
		esp_wifi_set_config(WIFI_IF_STA, &sta_config);
		LREP(__FUNCTION__, "Recv STA PASSWORD %s", sta_config.sta.password);
		break;
	case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
		LREP(__FUNCTION__, "blufi close a gatt connection");
		esp_blufi_close(blufi_server_if, blufi_conn_id);
		break;
	case ESP_BLUFI_EVENT_REPORT_ERROR:
		LREP_ERROR(__FUNCTION__, "");
		if(g_blufiConnected) esp_blufi_close(blufi_server_if, blufi_conn_id);
		if (g_blufiOver == false) {
			esp_ble_gap_start_advertising(&blufi_adv_params);
			gpio_set_led_auto();
		}
		else gpio_set_led_normal();
		break;
	case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
		LREP(__FUNCTION__, "Recv Custom Data %d", param->custom_data.data_len);
		app_ble_blufi_handle_custom_msg(param->custom_data.data, param->custom_data.data_len);
		break;
	default:
		LREP_WARNING(__FUNCTION__,"event unhandled %d", event);
		break;
	}
}

static void app_blufi_negotiate(uint8_t *data, int len, uint8_t **output_data, int *output_len, bool *need_free) {
	LREP(__FUNCTION__,"type %d", data[0]);
}

void  app_ble_send_wifi_conn_report(void) {
	if (!g_blufiConnected) return;
	wifi_mode_t mode;
	esp_wifi_get_mode(&mode);
	if (g_espGotIp) {
		esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, 0, NULL);
		app_cmd_handle_GetDeviceID(NULL, CMD_TYPE_BLUFI);
	}
	else {
		esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, 0, NULL);
	}
}

void  app_ble_blufi_send_custom_msg(const uint8_t* msg, int msg_len) {
	if (!g_blufiConnected) return;
	ESP_ERROR_CHECK(esp_blufi_send_custom_data(msg, msg_len));
}

void  app_ble_blufi_handle_custom_msg(const uint8_t* msg, int msg_len) {
	static char msg_buff[MAX_MESSAGE_LENGTH] = {0};
	memset(msg_buff, 0, MAX_MESSAGE_LENGTH);
	memcpy(msg_buff, msg, (msg_len > MAX_MESSAGE_LENGTH - 1) ? (MAX_MESSAGE_LENGTH - 1) : msg_len);
	LREP(__FUNCTION__, "%s", msg_buff);
	app_cmd_handle_buffer(msg_buff, msg_len, CMD_TYPE_BLUFI);
}

static void task_blufi(void* pvParameters) {
	int blufi_count = 0;
	while(1) {
		delay_ms(1000);
		if (bBlufiRequestEnd) break;
		if (bBlufiUnlimited) continue;
		if (blufi_count++ > 90) break;
	}
	g_blufiOver = true;
	esp_ble_gap_stop_advertising();
	if(g_blufiConnected) esp_blufi_close(blufi_server_if, blufi_conn_id);
	app_blufi_deinit();

	app_mqtt_start();
	gpio_set_led_normal();

	TASK_DELETE_SAFE(xBlufiTaskHandle);
}
