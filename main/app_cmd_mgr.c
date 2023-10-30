/*
 * app_cmd_mgr.c
 *
 *  Created on: Sep 21, 2020
 *      Author: laian
 */

#include "app_cmd_mgr.h"

#include "app_wifi_mode.h"
#include "app_schedule.h"
#include "app_mqtt.h"
#include "app_udp.h"
#include "app_time_server.h"
#include "app_ota.h"
#include "app_endpoint_link.h"
#include "app_fs_mgr.h"
#include "app_ble.h"

#include "esp_tls.h"
#include "esp_http_client.h"

#if ENCRYPTED
#include "app_sercure_mgr.h"

static char TxMessageBuff[MAX_MESSAGE_LENGTH] = {0};
static int TxMessageLen = 0;

static char RxMessageBuff[MAX_MESSAGE_LENGTH] = {0};
static int RxMessageLen = 0;
#endif

bool g_bSendSuccess = true;

static esp_err_t _http_event_handler(esp_http_client_event_t *evt);

esp_err_t	app_cmd_handle(const char* msg, cmd_type_t msgType) {
//	LREP_WARNING(__FUNCTION__, "Handle %s msg", (msgType == CMD_TYPE_UDP) ? "UDP" : "MQTT");
	esp_err_t ret = ESP_FAIL;
	cJSON *root = cJSON_Parse(msg);
	if (!json_check_object(root)) {
		LREP_WARNING(__FUNCTION__, "root is null, check json");
		goto DELETE;
	}
	LREP_WARNING(__FUNCTION__, "MSG: \"%s\"", msg);
	cJSON *pJsName = cJSON_GetObjectItem(root, "name");
	if (!json_check_string(pJsName)) {
		LREP_WARNING(__FUNCTION__, "json error: name");
		goto DELETE;
	}
	cJSON *pJsValue = cJSON_GetObjectItem(root, "value");
	if (!json_check_object(pJsValue)) {
		LREP_WARNING(__FUNCTION__, "json error: value object");
		goto DELETE;
	}

	if (json_cmp_str(pJsName, CMD_GET_DEVICE_INFO)) 				ret = app_cmd_handle_GetDeviceInfo(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_GET_DEVICE_ID)) 				ret = app_cmd_handle_GetDeviceID(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_GET_DEVICE_STATUS)) 			ret = app_cmd_handle_GetStatus(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_RESET))						ret = app_cmd_handle_Reset(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_DELETE_DEVICE)) 				ret = app_cmd_handle_DeleteDevice(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_SET_DEVICE_CONFIG)) 			ret = app_cmd_handle_SetDeviceConfig(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_GET_DEVICE_CONFIG)) 			ret = app_cmd_handle_GetDeviceConfig(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_SET_WIFI_INFO)) 				ret = app_cmd_handle_SetWifiInfo(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_EXIT_CONFIGURATION))			ret = app_cmd_handle_ExitConfiguration(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_SET_EXTRA_CONFIG)) 			ret = app_cmd_handle_SetExtraConfig(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_GET_EXTRA_CONFIG)) 			ret = app_cmd_handle_GetExtraConfig(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_GET_DEVICE)) 				ret = app_cmd_handle_GetData(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_SET_DEVICE)) 				ret = app_cmd_handle_SetData(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_ADD_SCHEDULE)) 				ret = app_cmd_handle_AddSchedule(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_DELETE_SCHEDULE)) 			ret = app_cmd_handle_DeleteSchedule(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_UPDATE_SCHEDULE)) 			ret = app_cmd_handle_UpdateSchedule(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_LIST_SCHEDULE)) 				ret = app_cmd_handle_ScheduleList(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_GET_SCHEDULE)) 				ret = app_cmd_handle_GetSchedule(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_SET_END_POINT_CONFIG)) 		ret = app_cmd_handle_SetEndpointConfig(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_GET_END_POINT_CONFIG)) 		ret = app_cmd_handle_GetEndpointConfig(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_DELETE_END_POINT_CONFIG)) 	ret = app_cmd_handle_DeleteEndpointConfig(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_END_POINT_LINK_SET_DATA)) 	ret = app_cmd_handle_EndpointSetData(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_START_OTA)) 					ret = app_cmd_handle_StartOta(pJsValue, msgType);
	else if (json_cmp_str(pJsName, CMD_SET_FORCE_OTA))				ret = app_cmd_handle_SetForceOta(pJsValue, msgType);
	else LREP_WARNING(__FUNCTION__, "Command \"%s\" not supported", pJsName->valuestring);

	DELETE: cJSON_Delete(root);
	return ret;
}

esp_err_t   app_cmd_handle_buffer(const char* msg, int msgLen, cmd_type_t msgType) {
	esp_err_t ret = ESP_FAIL;
	ret  = app_cmd_handle(msg, msgType);
	return ret;
}

#define CMD_HANDLE_CHECK_DEV_T(__JS_VALUE) \
	if (app_cmd_value_check_devT(__JS_VALUE) != ESP_OK) {\
		LREP_WARNING(__FUNCTION__, "devT invalid"); return ESP_FAIL;\
	}

#define CMD_HANDLE_CHECK_EXT_ADDR(__JS_VALUE) \
	if (app_cmd_value_check_devExtAddr(__JS_VALUE) != ESP_OK) {\
		LREP_WARNING(__FUNCTION__, "devExtAddr invalid"); return ESP_FAIL;\
	}

esp_err_t   app_cmd_handle_GetDeviceInfo(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_DEVICE_INFO);
	cJSON_AddStringToObject(root, "devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timestamp", app_time_server_get_timestamp_now());

	cJSON *pDevV = cJSON_AddArrayToObject(root, "devV");
	cJSON *pValue = cJSON_CreateObject();
	cJSON_AddStringToObject(pValue, "param", "software_version");
	cJSON_AddStringToObject(pValue, "value", app_ota_get_software_version());
	cJSON_AddItemToArray(pDevV, pValue);

	char *data_string = cJSON_PrintUnformatted(root);
	app_udp_client_send_broadcast(data_string);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	cJSON_Delete(root);
	free(data_string);

	return ESP_OK;
}

esp_err_t   app_cmd_handle_GetDeviceID(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT && msgType != CMD_TYPE_UDP && msgType != CMD_TYPE_BLUFI) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT, UDP or BLUFI message");
		return ESP_FAIL;
	}

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_DEVICE_ID);
	cJSON_AddNumberToObject(root, "devT",DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr",app_wifi_get_mac_str());

	const uint8_t* pBtAddr = esp_bt_dev_get_address();
	if (pBtAddr != NULL) {
		char bleMac[20] = {0};
		sprintf(bleMac, "%02X%02X%02X%02X%02X%02X", *pBtAddr, *(pBtAddr + 1), *(pBtAddr + 2), *(pBtAddr + 3), *(pBtAddr + 4), *(pBtAddr + 5));
		cJSON_AddStringToObject(root, "devBleMac", bleMac);
	}

	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());

	char *data_string = cJSON_PrintUnformatted(root);

	if (msgType == CMD_TYPE_BLUFI) 		app_cmd_send(data_string, CMD_RESPONSE_BLUFI);
	else if (msgType == CMD_TYPE_UDP)   app_cmd_send(data_string, CMD_UDP_UNICAST);
	else app_cmd_send(data_string, CMD_UDP_UNICAST|CMD_RESPONSE_MQTT);

	free(data_string);
	cJSON_Delete(root);
	return ESP_OK;
}

esp_err_t   app_cmd_handle_GetStatus(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_DEVICE_STATUS);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_wifi_get_mac_str());

	char *data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	free(data_string);
	cJSON_Delete(root);

	return ESP_OK;
}

esp_err_t 	app_cmd_handle_Reset(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType == CMD_TYPE_BLUFI) {
		LREP_WARNING(__FUNCTION__, "Reset from Blufi Cmd");
		esp_restart();
	}
	else if (msgType == CMD_TYPE_MQTT || msgType == CMD_TYPE_UDP) {
		CMD_HANDLE_CHECK_DEV_T(pJsValue);
		CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);
		LREP_WARNING(__FUNCTION__, "Reset from Mqtt or UDP");
		esp_restart();
	}
	return ESP_OK;
}

esp_err_t   app_cmd_handle_DeleteDevice(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType == CMD_TYPE_MQTT) {
		CMD_HANDLE_CHECK_DEV_T(pJsValue);
		CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

		LREP_WARNING(__FUNCTION__, "delete device ");
		app_skd_delete_all();
		app_endpoint_link_delete_all();
		app_mqtt_delete_cloud_info();
		app_filesystem_delete_config();

		cJSON *root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, "name", CMD_DELETE_DEVICE);
		cJSON_AddNumberToObject(root, "devT",DEVICE_TYPE);
		cJSON_AddStringToObject(root, "devExtAddr", app_wifi_get_mac_str());
		cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
		char *data_string = cJSON_PrintUnformatted(root);

		app_cmd_send(data_string, CMD_RESPONSE_MQTT);

		free(data_string);
		cJSON_Delete(root);
		exit(0);
	}
	else if (msgType == CMD_TYPE_BLUFI) {
		LREP_WARNING(__FUNCTION__, "delete device");
		app_skd_delete_all();
		app_endpoint_link_delete_all();
		app_mqtt_delete_cloud_info();
		app_filesystem_delete_config();
		esp_wifi_restore();

		esp_restart();
	}
	else if (msgType == CMD_TYPE_UDP) {
		CMD_HANDLE_CHECK_DEV_T(pJsValue);
		CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

		LREP_WARNING(__FUNCTION__, "delete device ");
		app_skd_delete_all();
		app_endpoint_link_delete_all();
		app_mqtt_delete_cloud_info();
		app_filesystem_delete_config();
		esp_wifi_restore();

		esp_restart();
	}
	return ESP_OK;
}

esp_err_t   app_cmd_handle_SetDeviceConfig(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_UDP && msgType != CMD_TYPE_BLUFI) {
		LREP_WARNING(__FUNCTION__, "Only support UDP && BLUFI message");
		return ESP_FAIL;
	}
	if ( app_cmd_value_check_devExtAddr(pJsValue) != ESP_OK) {
		LREP_WARNING(__FUNCTION__, "devExtAddr invalid");
		return ESP_FAIL;
	}

	cJSON *pbroker = cJSON_GetObjectItem(pJsValue, "broker");
	cJSON *puser = cJSON_GetObjectItem(pJsValue, "username");
	cJSON *ppass = cJSON_GetObjectItem(pJsValue, "password");
	cJSON *pmqttsub = cJSON_GetObjectItem(pJsValue, "mqttsub");
	cJSON *pmqttpub = cJSON_GetObjectItem(pJsValue, "mqttpub");
	cJSON *pmqttalert = cJSON_GetObjectItem(pJsValue, "mqttalert");
	cJSON *pforceOtaUrl = cJSON_GetObjectItem(pJsValue, "forceOtaUrl");
	cJSON *pbeSharedKey = cJSON_GetObjectItem(pJsValue, "beSharedKey");

	sCloudInfo_t CloudInfo = {
		.pBrokerUrl = json_check_string(pbroker) ? pbroker->valuestring : NULL,
		.pBrokerUsr = json_check_string(puser) ? puser->valuestring : NULL,
		.pBrokerPass = json_check_string(ppass) ? ppass->valuestring : NULL,
		.pTopicSub = json_check_string(pmqttsub) ? pmqttsub->valuestring : NULL,
		.pTopicPub = json_check_string(pmqttpub) ? pmqttpub->valuestring : NULL,
		.pTopicAllert = json_check_string(pmqttalert) ? pmqttalert->valuestring : NULL,
		.pForceOtaUrl = json_check_string(pforceOtaUrl) ? pforceOtaUrl->valuestring : NULL,
		.pBeSharedKey = json_check_string(pbeSharedKey) ? pbeSharedKey->valuestring : NULL,
	};

	if (!g_blufiOver) {
		cJSON *root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, "name", CMD_SET_DEVICE_CONFIG);
		cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
		cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
		cJSON_AddNumberToObject(root, "errorCode", ERROR_CODE_OK);
		cJSON_AddStringToObject(root, "software_version", app_ota_get_software_version());
		cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
		char *data_string = cJSON_PrintUnformatted(root);
		app_cmd_send(data_string, CMD_RESPONSE_BLUFI);
		buzzer();
		free(data_string);
		cJSON_Delete(root);

		app_skd_delete_all();
		app_endpoint_link_delete_all();
		app_filesystem_write_cloud_info(&CloudInfo);

		gpio_set_original_config(&g_Config);
		gpio_set_channel_config();
		gpio_set_led_config();
		gpio_set_night_config();

		app_ble_blufi_request_end();
	}
	else if (!bSoftApOver) {
		cJSON *root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, "name", CMD_SET_DEVICE_CONFIG);
		cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
		cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
		cJSON_AddNumberToObject(root, "errorCode", ERROR_CODE_OK);
		cJSON_AddStringToObject(root, "software_version", app_ota_get_software_version());
		cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
		char *data_string = cJSON_PrintUnformatted(root);

		buzzer();
		app_cmd_send(data_string, CMD_UDP_UNICAST);
		delay_ms(100);
		app_cmd_send(data_string, CMD_UDP_UNICAST);

		free(data_string);
		cJSON_Delete(root);

		app_skd_delete_all();
		app_endpoint_link_delete_all();
		app_filesystem_write_cloud_info(&CloudInfo);

		gpio_set_original_config(&g_Config);
		gpio_set_channel_config();
		gpio_set_led_config();
		gpio_set_night_config();

		app_wifi_config_mode_end();
	}
	else {
		bool new_config = false;
		if ((json_check_string(pbroker) && strcmp(g_CloudInfo.pBrokerUrl, pbroker->valuestring) != 0) ||
				(json_check_string(puser) && strcmp(g_CloudInfo.pBrokerUsr, puser->valuestring) != 0) ||
				(json_check_string(ppass) && strcmp(g_CloudInfo.pBrokerPass, ppass->valuestring) != 0) ||
				(json_check_string(pmqttsub) && strcmp(g_CloudInfo.pTopicSub, pmqttsub->valuestring) != 0) ||
				(json_check_string(pmqttpub) && strcmp(g_CloudInfo.pTopicPub, pmqttpub->valuestring) != 0) ||
				(json_check_string(pmqttalert) && strcmp(g_CloudInfo.pTopicAllert, pmqttalert->valuestring) != 0) ||
				(json_check_string(pforceOtaUrl) && strcmp(g_CloudInfo.pForceOtaUrl, pforceOtaUrl->valuestring) != 0) ||
				(json_check_string(pbeSharedKey) && strcmp(g_CloudInfo.pBeSharedKey, pbeSharedKey->valuestring) != 0)
		) new_config = true;

		if (new_config) {
			char *data_string;
			cJSON *root = NULL;
			root = cJSON_CreateObject();
			cJSON_AddStringToObject(root, "name", CMD_SET_DEVICE_CONFIG);
			cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
			cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
			cJSON_AddNumberToObject(root, "errorCode", ERROR_CODE_SET_CONFIG_IN_PROCESS);

			data_string = cJSON_PrintUnformatted(root);
			app_cmd_send(data_string, CMD_UDP_UNICAST);
			free(data_string);
			cJSON_Delete(root);
		}
		else {
			char *data_string;
			cJSON *root = NULL;
			root = cJSON_CreateObject();
			cJSON_AddStringToObject(root, "name", CMD_SET_DEVICE_CONFIG);
			cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
			if (json_check_string(pbroker)) 		cJSON_AddStringToObject(root, "broker",pbroker->valuestring);
			if (json_check_string(puser)) 			cJSON_AddStringToObject(root, "username",puser->valuestring);
			if (json_check_string(ppass)) 			cJSON_AddStringToObject(root, "password",ppass->valuestring);
			if (json_check_string(pmqttsub)) 		cJSON_AddStringToObject(root, "mqttsub",pmqttsub->valuestring);
			if (json_check_string(pmqttpub)) 		cJSON_AddStringToObject(root, "mqttpub",pmqttpub->valuestring);
			if (json_check_string(pmqttalert)) 		cJSON_AddStringToObject(root, "mqttalert",pmqttalert->valuestring);
			if (json_check_string(pforceOtaUrl))	cJSON_AddStringToObject(root, "forceOtaUrl", pforceOtaUrl->valuestring);
			if (json_check_string(pbeSharedKey))	cJSON_AddStringToObject(root, "beSharedKey", pbeSharedKey->valuestring);
			cJSON_AddStringToObject(root, "software_version", app_ota_get_software_version());
			cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
			data_string = cJSON_PrintUnformatted(root);
			app_cmd_send(data_string, CMD_UDP_UNICAST);
			free(data_string);
			cJSON_Delete(root);
		}
	}
	return ESP_OK;
}

esp_err_t   app_cmd_handle_GetDeviceConfig(const cJSON* pJsValue, cmd_type_t msgType) {
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_DEVICE_CONFIG);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
	cJSON_AddStringToObject(root, "broker",g_CloudInfo.pBrokerUrl);
	cJSON_AddStringToObject(root, "username",g_CloudInfo.pBrokerUsr);
	cJSON_AddStringToObject(root, "password",g_CloudInfo.pBrokerPass);
	cJSON_AddStringToObject(root, "mqttsub",g_CloudInfo.pTopicSub);
	cJSON_AddStringToObject(root, "mqttpub",g_CloudInfo.pTopicPub);
	cJSON_AddStringToObject(root, "mqttalert",g_CloudInfo.pTopicAllert);
	cJSON_AddStringToObject(root, "software_version", app_ota_get_software_version());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
	char *data_string = cJSON_PrintUnformatted(root);
	if (msgType == CMD_TYPE_MQTT) app_cmd_send(data_string, CMD_RESPONSE_MQTT);//app_mqtt_response(data_string);
	else if (msgType == CMD_TYPE_UDP) app_cmd_send(data_string, CMD_UDP_UNICAST);//app_udp_client_send_unicast(data_string);
	free(data_string);
	cJSON_Delete(root);
	return ESP_OK;
}

esp_err_t   app_cmd_handle_SetWifiInfo(const cJSON* pJsValue, cmd_type_t msgType) {
	if ( msgType != CMD_TYPE_UDP) {
		LREP_WARNING(__FUNCTION__, "Only support UDP message");
		return ESP_FAIL;
	}

	cJSON *pSSID = cJSON_GetObjectItem(pJsValue, "ssid");
	cJSON *pPass = cJSON_GetObjectItem(pJsValue, "password");
	if (!json_check_string(pSSID) || !json_check_string(pPass)) {
		LREP_WARNING(__FUNCTION__,"Invalid wifi info");
		return ESP_FAIL;
	}

	LREP_WARNING(__FUNCTION__, "SSID :\"%s\"", pSSID->valuestring);
	LREP_WARNING(__FUNCTION__, "PASS :\"%s\"", pPass->valuestring);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_SET_WIFI_INFO);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
	cJSON_AddNumberToObject(root, "errorCode", ERROR_CODE_OK);
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
	char *data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_UDP_UNICAST);
	free(data_string);
	cJSON_Delete(root);

	app_wifi_set_station(pSSID->valuestring, pPass->valuestring);
	return ESP_OK;
}

esp_err_t 	app_cmd_handle_ExitConfiguration(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_BLUFI && msgType != CMD_TYPE_UDP) {
		LREP_WARNING(__FUNCTION__, "Only support BLUFI and UDP");
		return ESP_FAIL;
	}

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_EXIT_CONFIGURATION);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
	cJSON_AddNumberToObject(root, "errorCode", ERROR_CODE_OK);
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
	char *data_string = cJSON_PrintUnformatted(root);
	if (msgType == CMD_TYPE_UDP)
		app_cmd_send(data_string, CMD_UDP_UNICAST);
	else if (msgType == CMD_TYPE_BLUFI)
		app_cmd_send(data_string, CMD_RESPONSE_BLUFI);
	free(data_string);
	cJSON_Delete(root);
	if (!g_blufiOver) {
		LREP(__FUNCTION__, "blufi end");
		app_ble_blufi_request_end();
	}
	else if (!bSoftApOver) {
		LREP(__FUNCTION__, "Softap End");
		app_wifi_config_mode_end();
	}

	return ESP_OK;
}

esp_err_t 	app_cmd_handle_SetExtraConfig(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON* pReset = cJSON_GetObjectItem(pJsValue, "resetAll");
	if (json_check_number(pReset)) {
		LREP_WARNING(__FUNCTION__, "Reset config %d", pReset->valueint);
		gpio_set_led_color(pReset->valueint, 1, COLOR_ON_DEFAULT);
		gpio_set_led_color(pReset->valueint, 0, COLOR_OFF_DEFAULT);
		gpio_set_led_lightness(pReset->valueint, LIGHTNESS_DEFAULT);
	}
	else {
		cJSON* pBuzzer = cJSON_GetObjectItem(pJsValue, "buzzerEnb");
		if (json_check_number(pBuzzer)) {
			LREP_WARNING(__FUNCTION__, "set Buzzer %s", (pBuzzer->valueint == 1) ? "ENABLE" : "DISABLE");
			if (g_Config.iBuzzerEnb != pBuzzer->valueint) {
				g_Config.iBuzzerEnb = (pBuzzer->valueint == 1) ? 1 : 0;
			}
		}
		app_cmd_led_SetExtraConfig(pJsValue);
		app_cmd_channel_SetExtraConfig(pJsValue, SW_CHANNEL_1);
		app_cmd_channel_SetExtraConfig(pJsValue, SW_CHANNEL_2);
		app_cmd_channel_SetExtraConfig(pJsValue, SW_CHANNEL_3);
		app_cmd_channel_SetExtraConfig(pJsValue, SW_CHANNEL_4);
		app_cmd_curtain_SetExtraConfig(pJsValue);
		app_cmd_night_SetExtraConfig(pJsValue);

		cJSON* pSaveStt = cJSON_GetObjectItem(pJsValue, "saveStt");
		if (json_check_number(pSaveStt)) {
			LREP_WARNING(__FUNCTION__, "set Save Stt %s", (pSaveStt->valueint == 1) ? "ENABLE" : "DISABLE");
			g_Config.saveStt = (pSaveStt->valueint == 1) ? 1 : 0;
		}
	}

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_SET_EXTRA_CONFIG);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
	app_cmd_build_response_ExtraConfig(root);
	char* data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	free(data_string);
	cJSON_Delete(root);

	return ESP_OK;
}



esp_err_t 	app_cmd_handle_GetExtraConfig(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_EXTRA_CONFIG);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
	app_cmd_build_response_ExtraConfig(root);
	char* data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	free(data_string);
	cJSON_Delete(root);

	return ESP_OK;
}

esp_err_t   app_cmd_handle_GetData(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT && msgType != CMD_TYPE_UDP) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT or UDP message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	g_bRequestGetData = true;
	return ESP_OK;
}

esp_err_t   app_cmd_handle_SetData(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT && msgType != CMD_TYPE_UDP) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT or UDP message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON* pdevV = cJSON_GetObjectItem(pJsValue, "devV");
#if 0
	if (!json_check_object(pdevV)) {
		LREP_WARNING(__FUNCTION__, "devV Error");
		return ESP_FAIL;
	}

	cJSON* pParam = cJSON_GetObjectItem(pdevV, "param");
	cJSON* pValue = cJSON_GetObjectItem(pdevV, "value");
	if (!json_check_string(pParam) || !json_check_number(pValue)) {
		LREP_WARNING(__FUNCTION__, "param, value invalid");
		return ESP_FAIL;
	}
#ifdef DEVICE_TYPE
	if (json_cmp_str(pParam, SWITCH_1)) {
		if (app_remote_change_channel_status(SW_CHANNEL_1, pValue->valueint)) buzzer();
	}
	else if (json_cmp_str(pParam, SWITCH_2)) {
		if (app_remote_change_channel_status(SW_CHANNEL_2, pValue->valueint)) buzzer();
	}
	else if (json_cmp_str(pParam, SWITCH_3)) {
		if (app_remote_change_channel_status(SW_CHANNEL_3, pValue->valueint)) buzzer();
	}
	else if (json_cmp_str(pParam, "all")) {
		app_remote_change_channel_status(SW_CHANNEL_1, pValue->valueint);
		app_remote_change_channel_status(SW_CHANNEL_2, pValue->valueint);
		app_remote_change_channel_status(SW_CHANNEL_3, pValue->valueint);
		buzzer();
	}
#endif
#else
	if (json_check_object(pdevV)) {
		app_cmd_SetData(pdevV);
	}
	else if (json_check_array(pdevV)) {
		for (int i=0; i<cJSON_GetArraySize(pdevV); ++i) {
			cJSON *pDevV_Item = cJSON_GetArrayItem(pdevV, i);
			app_cmd_SetData(pDevV_Item);
		}
	}
	else {
		LREP_WARNING(__FUNCTION__, "devV Error");
		return ESP_FAIL;
	}
#endif
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_SET_DEVICE);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "errorCode", ERROR_CODE_OK);
	char *data_string = cJSON_PrintUnformatted(root);
	if (msgType == CMD_TYPE_MQTT) app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	else if (msgType == CMD_TYPE_UDP) app_cmd_send(data_string, CMD_UDP_UNICAST);
	free(data_string);
	cJSON_Delete(root);

	g_bRequestGetData = true;
	return ESP_OK;
}

esp_err_t   app_cmd_handle_AddSchedule(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
#if 0
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);
#endif
	int id = -1;
	char* data_string = cJSON_PrintUnformatted(pJsValue);
	int err = app_skd_add_return_id(data_string, &id);
	free(data_string);

	app_cmd_schedule_reponse(CMD_ADD_SCHEDULE, id, err);
	return ESP_OK;
}

esp_err_t   app_cmd_handle_DeleteSchedule(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
#if 0
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);
#endif
	cJSON* pId = cJSON_GetObjectItem(pJsValue, "id");
	if (!json_check_number(pId)) {
		LREP_WARNING(__FUNCTION__, "id invalid");
		return ESP_FAIL;
	}
	int err = ERROR_CODE_UNDEFINED;
	if (pId->valueint == -1) {
		err = app_skd_delete_all();
	}
	else {
		err = app_skd_delete(pId->valueint);
	}
	app_cmd_schedule_reponse(CMD_DELETE_SCHEDULE, pId->valueint, err);
	return ESP_OK;
}

esp_err_t   app_cmd_handle_UpdateSchedule(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
#if 0
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);
#endif
	int id = -1;
	char* data_string = cJSON_PrintUnformatted(pJsValue);
	int err = app_skd_update_return_id(data_string, &id);
	free(data_string);

	app_cmd_schedule_reponse(CMD_UPDATE_SCHEDULE, id, err);
	return ESP_OK;
}

esp_err_t   app_cmd_handle_ScheduleList(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);
	int idList[APP_SKD_MAXIMUM] = {-1};
	int skdCount = app_skd_list(idList);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_LIST_SCHEDULE);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
	cJSON *pDevV = cJSON_AddArrayToObject(root, "devV");
	PARAM_INT_VALUE_ADD(pDevV, "max_skd", APP_SKD_MAXIMUM);
	PARAM_INT_VALUE_ADD(pDevV, "skd_count", skdCount);
	for(int i =0; i< skdCount; i++) PARAM_INT_VALUE_ADD(pDevV, "id" , idList[i]);
	char *data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	free(data_string);
	cJSON_Delete(root);
	return ESP_OK;
}

esp_err_t   app_cmd_handle_GetSchedule(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON* pId = cJSON_GetObjectItem(pJsValue, "id");
	if  (!json_check_number(pId)) {
		LREP_WARNING(__FUNCTION__,"json id not ok");
		return ESP_FAIL;
	}

	const sSkd_t* pSkd = app_skd_get(pId->valueint);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_SCHEDULE);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
	cJSON_AddNumberToObject(root, "id", pId->valueint);
	if (pSkd == NULL) {
		cJSON_AddNumberToObject(root, "errorCode", ERROR_CODE_SCHEDULE_NOT_EXISTED);
	}
	else {
		cJSON* pDevV = cJSON_AddObjectToObject(root,"devV");
		app_skd_build_json_value(pDevV, pSkd);
	}

	char *data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	free(data_string);
	cJSON_Delete(root);
	return ESP_OK;
}

esp_err_t   app_cmd_handle_StartOta(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);
	cJSON* pVer = cJSON_GetObjectItem(pJsValue, "version");
	cJSON* pUrl = cJSON_GetObjectItem(pJsValue, "url");

	if (!json_check_string(pVer) || !json_check_string(pUrl)) {
		LREP_WARNING(__FUNCTION__, "version or url invalid");
		app_cmd_basic_response(CMD_START_OTA, ERROR_CODE_UNDEFINED);
		return ESP_FAIL;
	}

	if ( app_ota_set_update_version(pVer->valuestring) != ESP_OK ||
			app_ota_set_server_url(pUrl->valuestring) != ESP_OK) {
		app_cmd_basic_response(CMD_START_OTA, ERROR_CODE_UNDEFINED);
		return ESP_FAIL;
	}

	g_bRequestOta = true;

	return ESP_OK;
}

esp_err_t   app_cmd_handle_SetForceOta(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON *pVer = cJSON_GetObjectItem(pJsValue, "version");
	cJSON *pUrl = cJSON_GetObjectItem(pJsValue, "url");
	cJSON *pTimeStart = cJSON_GetObjectItem(pJsValue, "timeStart");
	cJSON *pTimeEnd = cJSON_GetObjectItem(pJsValue, "timeEnd");
	cJSON *pTimezone = cJSON_GetObjectItem(pJsValue, "timezone");
	cJSON *pSkdId = cJSON_GetObjectItem(pJsValue, "scheduleId");
	cJSON *pDateStart = cJSON_GetObjectItem(pJsValue, "dateStart");
	cJSON *pDateEnd = cJSON_GetObjectItem(pJsValue, "dateEnd");
	cJSON *pType = cJSON_GetObjectItem(pJsValue,"state");
	int errcode = ERROR_CODE_OK;

	if (!json_check_string(pVer) || !json_check_string(pUrl)) {
		LREP_WARNING(__FUNCTION__, "version or url invalid");
		errcode = ERROR_CODE_UNDEFINED;
	}

	if (!json_check_number(pTimeStart) || !json_check_number(pTimeEnd) || !json_check_number(pTimezone) ||
			!json_check_number(pSkdId) || !json_check_number(pDateStart) || !json_check_number(pDateEnd) || !json_check_string(pType)) {
		LREP_WARNING(__FUNCTION__, "force ota schedule invalid");
		errcode = ERROR_CODE_UNDEFINED;
	}

	if (errcode == ERROR_CODE_OK) {
		g_forceOta.forceOtaSkd.u64TimeStart = pTimeStart->valueint;
		g_forceOta.forceOtaSkd.u64TimeEnd = pTimeEnd->valueint;
		g_forceOta.forceOtaSkd.u64DateStart = pDateStart->valueint;
		g_forceOta.forceOtaSkd.u64DateEnd = pDateEnd->valueint;
		g_forceOta.forceOtaSkd.fTimezone = pTimezone->valuedouble;
		g_forceOta.forceOtaSkd.iSkdId = pSkdId->valueint;
		memset(g_forceOta.strUrl, 0, sizeof(g_forceOta.strUrl));
		strcpy(g_forceOta.strUrl, pUrl->valuestring);
		memset(g_forceOta.strVersion, 0, sizeof(g_forceOta.strVersion));
		strcpy(g_forceOta.strVersion, pVer->valuestring);
		g_forceOta.bAuto = (strcmp(pType->valuestring, "AUTO") == 0) ? true : false;
		g_forceOta.bUpdated = false;
		app_filesystem_write_force_ota_info(&g_forceOta);
		app_force_ota_init();

		cJSON *root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, "name", CMD_SET_FORCE_OTA);
		cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
		cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
		cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
		cJSON_AddNumberToObject(root, "errorCode", errcode);
		cJSON_AddNumberToObject(root, "scheduleId", pSkdId->valueint);
		char *data_string = cJSON_PrintUnformatted(root);
		app_cmd_send(data_string, CMD_REPORT_MQTT);
		free(data_string);
		cJSON_Delete(root);

		if (g_forceOta.bAuto) g_bRequestForceOtaAuto = true;
	}
	else {
		cJSON *root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, "name", CMD_SET_FORCE_OTA);
		cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
		cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
		cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
		cJSON_AddNumberToObject(root, "errorCode", errcode);
		char *data_string = cJSON_PrintUnformatted(root);
		app_cmd_send(data_string, CMD_REPORT_MQTT);
		free(data_string);
		cJSON_Delete(root);
	}
	return ESP_OK;
}

esp_err_t   app_cmd_handle_SetEndpointConfig(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	LREP(__FUNCTION__,"go to set endpoint config");

	cJSON *pEndpoint = cJSON_GetObjectItem(pJsValue, "endpoint");
	if (json_check_number(pEndpoint)) {
		LREP(__FUNCTION__," end point config %d",pEndpoint->valueint);
	}

	char *endpointconfig_str = cJSON_PrintUnformatted(pJsValue);
	int ret = app_endpoint_link_add(endpointconfig_str);
	free(endpointconfig_str);

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root,"name", CMD_SET_END_POINT_CONFIG);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root,"devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timestamp", app_time_server_get_timestamp_now());
	cJSON_AddNumberToObject(root, "errorCode", ret);
	char *data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);

	cJSON_Delete(root);
	free(data_string);
	return ESP_OK;
}

esp_err_t   app_cmd_handle_GetEndpointConfig(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON *root   = cJSON_CreateObject();
	cJSON *devVs  = NULL;
	cJSON_AddStringToObject(root, "name", CMD_GET_END_POINT_CONFIG);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddNumberToObject(root, "batteryPercent", BATTERYPERCENT);
	cJSON_AddStringToObject(root, "devExtAddr",app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
	devVs = cJSON_AddArrayToObject(root, "devV");
#ifdef DEVICE_TYPE
	for (int i = 1; i <= 3; i++)
#else
	for (int i = 1; i < MAX_ENDPOINT_LINK; i++)
#endif
	{
		cJSON* pDevV = cJSON_CreateObject();
		app_endpoint_link_build_json_value(pDevV, i);
		cJSON_AddItemToArray(devVs, pDevV);
	}

	char* data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);

	cJSON_Delete(root);
	free(data_string);

	return ESP_OK;
}

esp_err_t   app_cmd_handle_DeleteEndpointConfig(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT message");
		return ESP_FAIL;
	}
	CMD_HANDLE_CHECK_DEV_T(pJsValue);
	CMD_HANDLE_CHECK_EXT_ADDR(pJsValue);

	cJSON* pEndpoint = cJSON_GetObjectItem(pJsValue, "endpoint");
	if (!json_check_number(pEndpoint)) {
		LREP_WARNING(__FUNCTION__," end point invalid");
	}

	int ret = ERROR_CODE_OK;

	if (pEndpoint->valueint == 0) ret = app_endpoint_link_delete_all();
	else ret = app_endpoint_link_delete(pEndpoint->valueint);


	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root,"name", CMD_DELETE_END_POINT_CONFIG);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root,"devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "endpoint", pEndpoint->valueint);
	cJSON_AddNumberToObject(root, "timestamp", app_time_server_get_timestamp_now());
	cJSON_AddNumberToObject(root, "errorCode", ret);
	char *data_string = cJSON_PrintUnformatted(root);

	app_cmd_send(data_string, CMD_RESPONSE_MQTT);

	cJSON_Delete(root);
	free(data_string);

	return ESP_OK;
}

esp_err_t   app_cmd_handle_EndpointSetData(const cJSON* pJsValue, cmd_type_t msgType) {
	if (msgType != CMD_TYPE_MQTT_EPLINK && msgType != CMD_TYPE_UDP) {
		LREP_WARNING(__FUNCTION__, "Only support MQTT via EPLINK or UDP message");
		return ESP_FAIL;
	}

	cJSON* pdevT  = cJSON_GetObjectItem(pJsValue, "devT");
	cJSON* pdesId = cJSON_GetObjectItem(pJsValue, "desDevId");
	cJSON* pdesEp = cJSON_GetObjectItem(pJsValue, "desEndpoint");
	cJSON* psrcId = cJSON_GetObjectItem(pJsValue, "srcDevId");
	cJSON* psrcEp = cJSON_GetObjectItem(pJsValue, "srcEndpoint");
	cJSON* pVal   = cJSON_GetObjectItem(pJsValue, "value");
	cJSON* porigId = cJSON_GetObjectItem(pJsValue, "origId");

	if (!json_check_number(pdevT) || !json_check_string(pdesId) || !json_check_number(pdesEp) ||
			!json_check_string(psrcId) || !json_check_number(psrcEp) || !json_check_number(pVal)) {
		LREP_WARNING(__FUNCTION__, "Lack essential values");
		return ESP_FAIL;
	}
//	LREP(__FUNCTION__," srcDevId_param %s",psrcId->valuestring);
//	LREP(__FUNCTION__," srcEndpoint_param %d",psrcEp->valueint);
//	LREP(__FUNCTION__," desDevId_param %s",pdesId->valuestring);
//	LREP(__FUNCTION__," desEndpoint_param %d",pdesEp->valueint);


	if (strcmp(pdesId->valuestring, app_wifi_get_mac_str()) != 0) {
		LREP_WARNING(__FUNCTION__,"Invalid device id");
		return ESP_FAIL;
	}
	if (pdesEp->valueint==0 || pdesEp->valueint >= MAX_ENDPOINT_LINK) {
		LREP_WARNING(__FUNCTION__, "des endpoint invalid");
		return ESP_FAIL;
	}
	if (strlen(g_endpoint_link_arr[pdesEp->valueint].chsrcDevId) == 0 || g_endpoint_link_arr[pdesEp->valueint].iSrcEndpoint == 0) {
		LREP_WARNING(__FUNCTION__, "src for endpoint %d is not set", pdesEp->valueint);
		return ESP_FAIL;
	}
	if (strcmp(psrcId->valuestring, g_endpoint_link_arr[pdesEp->valueint].chsrcDevId) != 0 ||
			psrcEp->valueint != g_endpoint_link_arr[pdesEp->valueint].iSrcEndpoint) {
		LREP_WARNING(__FUNCTION__,"src endpoint invalid");
		return ESP_FAIL;
	}

#ifdef DEVICE_TYPE
	if (json_check_string(porigId)) {
		LREP(__FUNCTION__," original id %s", porigId->valuestring);
		if (strcmp(porigId->valuestring, app_wifi_get_mac_str()) == 0) {
			LREP_WARNING(__FUNCTION__,"echo message will not be handled");
			return ESP_OK;
		}
		if (pdesEp->valueint == 1) {
			app_remote_eplink_change_channel_status(SW_CHANNEL_1, pVal->valueint);
			app_cmd_send_EndpointSetData(1, porigId->valuestring);
		}
		else if (pdesEp->valueint == 2) {
			app_remote_eplink_change_channel_status(SW_CHANNEL_2, pVal->valueint);
			app_cmd_send_EndpointSetData(2, porigId->valuestring);
		}
		else if (pdesEp->valueint == 3) {
			app_remote_eplink_change_channel_status(SW_CHANNEL_3, pVal->valueint);
			app_cmd_send_EndpointSetData(3, porigId->valuestring);
		}
	}
	else {
		if (pdesEp->valueint == 1)	app_remote_eplink_change_channel_status(SW_CHANNEL_1, pVal->valueint);
		else if (pdesEp->valueint == 2) app_remote_eplink_change_channel_status(SW_CHANNEL_2, pVal->valueint);
		else if (pdesEp->valueint == 3) app_remote_eplink_change_channel_status(SW_CHANNEL_3, pVal->valueint);
	}
#endif
	return ESP_OK;
}

esp_err_t 	app_cmd_value_check_devT(const cJSON* pJsValue) {
	cJSON* pdevT = cJSON_GetObjectItem(pJsValue, "devT");
	if (!json_check_number(pdevT)) return ESP_FAIL;
	if (pdevT->valueint != DEVICE_TYPE) return ESP_FAIL;
	return ESP_OK;
}

esp_err_t 	app_cmd_value_check_devExtAddr(const cJSON* pJsValue) {
	cJSON* pExtAddr = cJSON_GetObjectItem(pJsValue, "devExtAddr");
	if (!json_check_string(pExtAddr)) return ESP_FAIL;
	if (strcmp(pExtAddr->valuestring, app_wifi_get_mac_str()) != 0) return ESP_FAIL;
	return ESP_OK;
}

const char*	app_cmd_get_deviceId(void) {
	return app_wifi_get_mac_str();
}

void 		app_cmd_response_GetData(void) {
	char *data_string;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_DEVICE);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddNumberToObject(root, "batteryPercent", BATTERYPERCENT);
	cJSON_AddStringToObject(root, "devExtAddr",app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());

	cJSON* pDevV = cJSON_AddArrayToObject(root, "devV");

#ifdef DEVICE_TYPE
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_1, g_ChannelControl[1].iStatus);
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_2, g_ChannelControl[2].iStatus);
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_3, g_ChannelControl[3].iStatus);
#endif

	data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT | CMD_UDP_BROADCAST);

	free(data_string);
	cJSON_Delete(root);
}

void 		app_cmd_GetData(void) {
	char *data_string;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_DEVICE);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddNumberToObject(root, "batteryPercent", BATTERYPERCENT);
	cJSON_AddStringToObject(root, "devExtAddr",app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());

	cJSON* pDevV = cJSON_AddArrayToObject(root, "devV");

#ifdef DEVICE_TYPE
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_1, g_ChannelControl[1].iStatus);
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_2, g_ChannelControl[2].iStatus);
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_3, g_ChannelControl[3].iStatus);
#endif

	data_string = cJSON_PrintUnformatted(root);
	LREP(__FUNCTION__, "\"%s\"", data_string);
	if (!app_mqtt_response(data_string)) {
		g_bSendSuccess = false;
		LREP_WARNING(__FUNCTION__, "UDP broadcast");
		app_udp_client_send_broadcast(data_string);
		app_udp_client_send_unicast(data_string);
	}
	else g_bSendSuccess = true;

	free(data_string);
	cJSON_Delete(root);
}

void 		app_cmd_GetDataResend(void) {
	if (g_bSendSuccess) return;
	char *data_string;
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_GET_DEVICE);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddNumberToObject(root, "batteryPercent", BATTERYPERCENT);
	cJSON_AddStringToObject(root, "devExtAddr",app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());

	cJSON* pDevV = cJSON_AddArrayToObject(root, "devV");
#ifdef DEVICE_TYPE
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_1, g_ChannelControl[1].iStatus);
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_2, g_ChannelControl[2].iStatus);
	PARAM_INT_VALUE_ADD(pDevV, SWITCH_3, g_ChannelControl[3].iStatus);
#endif
	data_string = cJSON_PrintUnformatted(root);
	LREP(__FUNCTION__, "\"%s\"", data_string);
	if (app_mqtt_response(data_string)) g_bSendSuccess = true;

	free(data_string);
	cJSON_Delete(root);
}

void		app_cmd_SetData(const cJSON *pJsDevV) {
	if (!json_check_object(pJsDevV)) {
		LREP_WARNING(__FUNCTION__, "devV Error");
		return;
	}

	cJSON* pParam = cJSON_GetObjectItem(pJsDevV, "param");
	cJSON* pValue = cJSON_GetObjectItem(pJsDevV, "value");
	if (!json_check_string(pParam) || !json_check_number(pValue)) {
		LREP_WARNING(__FUNCTION__, "param, value invalid");
		return;
	}
#ifdef DEVICE_TYPE
	if (json_cmp_str(pParam, SWITCH_1)) {
		if (app_remote_change_channel_status(SW_CHANNEL_1, pValue->valueint)) buzzer();
	}
	else if (json_cmp_str(pParam, SWITCH_2)) {
		if (app_remote_change_channel_status(SW_CHANNEL_2, pValue->valueint)) buzzer();
	}
	else if (json_cmp_str(pParam, SWITCH_3)) {
		if (app_remote_change_channel_status(SW_CHANNEL_3, pValue->valueint)) buzzer();
	}
	else if (json_cmp_str(pParam, "all")) {
		app_remote_change_channel_status(SW_CHANNEL_1, pValue->valueint);
		app_remote_change_channel_status(SW_CHANNEL_2, pValue->valueint);
		app_remote_change_channel_status(SW_CHANNEL_3, pValue->valueint);
		buzzer();
	}
#endif
}

void  		app_cmd_report_force_ota(int skdId, int err) {
	char *data_string;
	cJSON *root = NULL;
	root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "name", REPORT_FORCE_OTA);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_cmd_get_deviceId());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());
	cJSON_AddNumberToObject(root, "scheduleId", skdId);
	cJSON_AddNumberToObject(root, "errorCode", err);

	data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_REPORT_MQTT);
	free(data_string);
	cJSON_Delete(root);
}

void		app_cmd_schedule_reponse(const char* cmd, int id, int err) {
	char *data_string;
	cJSON *root = NULL;
	root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "name", cmd);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());

	cJSON_AddNumberToObject(root, "id", id);
	cJSON_AddNumberToObject(root, "errorCode", err);

	data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	free(data_string);
	cJSON_Delete(root);
}

void 		app_cmd_basic_response(const char* cmd, int err) {
	char *data_string;
	cJSON *root = cJSON_CreateObject();

	cJSON_AddStringToObject(root, "name", cmd);
	cJSON_AddNumberToObject(root, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(root, "devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(root, "timeStamp", app_time_server_get_timestamp_now());

	cJSON_AddNumberToObject(root, "errorCode", err);

	data_string = cJSON_PrintUnformatted(root);
	app_cmd_send(data_string, CMD_RESPONSE_MQTT);
	free(data_string);
	cJSON_Delete(root);
}

void		app_cmd_send(const char* data_string, uint8_t response) {
//	LREP(__FUNCTION__, "MQTT Response %s, MQTT Report %s,  UDP Broadcasr %s, UDP Unicast %s, BLUFI %s\r\n",
//			(response & CMD_RESPONSE_MQTT) ? "YES" : "NO", (response & CMD_REPORT_MQTT) ? "YES" : "NO",
//					(response & CMD_UDP_BROADCAST) ? "YES" : "NO", (response & CMD_UDP_UNICAST) ? "YES" : "NO",
//							(response & CMD_RESPONSE_BLUFI)? "YES" : "NO");
//	LREP(__FUNCTION__,"Plain text: \"%s\"", data_string);
#if ENCRYPTED
	TxMessageLen = app_sercure_mgr_encrypt(data_string, TxMessageBuff, strlen(data_string));
	LREP(__FUNCTION__, "Cippher len %d" , TxMessageLen);
	if (response & CMD_RESPONSE_MQTT) 	app_mqtt_buffer_response(TxMessageBuff, TxMessageLen);
	if (response & CMD_REPORT_MQTT)		app_mqtt_buffer_report_be(TxMessageBuff, TxMessageLen);
	if (response & CMD_UDP_BROADCAST) 	app_udp_client_send_buffer_broadcast(TxMessageBuff, TxMessageLen);
	if (response & CMD_UDP_UNICAST)		app_udp_client_send_buffer_unicast(TxMessageBuff, TxMessageLen);
	if (response & CMD_RESPONSE_BLUFI)	app_ble_blufi_send_custom_msg(TxMessageBuff, TxMessageLen);
#else
	if (response & CMD_RESPONSE_MQTT) 	app_mqtt_response(data_string);
	if (response & CMD_REPORT_MQTT)		app_mqtt_report_be(data_string);
	if (response & CMD_UDP_BROADCAST) 	app_udp_client_send_broadcast(data_string);
	if (response & CMD_UDP_UNICAST)		app_udp_client_send_unicast(data_string);
	if (response & CMD_RESPONSE_BLUFI)  app_ble_blufi_send_custom_msg((const uint8_t*)data_string, strlen(data_string));
#endif
}

void 		app_cmd_send_EndpointSetData(int endpoint, const char* origId) {
	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "name", CMD_END_POINT_LINK_SET_DATA);
	cJSON *value;
	cJSON_AddItemToObject(root,"value",value = cJSON_CreateObject());
	cJSON_AddNumberToObject(value, "devT",DEVICE_TYPE);
	cJSON_AddStringToObject(value, "srcDevId", app_cmd_get_deviceId());
	cJSON_AddNumberToObject(value, "srcEndpoint", endpoint);
	cJSON_AddStringToObject(value, "desDevId", g_endpoint_link_arr[endpoint].chdesDevId);
	cJSON_AddNumberToObject(value, "desEndpoint", g_endpoint_link_arr[endpoint].iDesEndpoint);
#ifdef DEVICE_TYPE
	cJSON_AddNumberToObject(value, "value", (endpoint == 1) ? g_ChannelControl[1].iStatus : (endpoint == 2) ? g_ChannelControl[2].iStatus : g_ChannelControl[3].iStatus);
#endif
	cJSON_AddStringToObject(value, "origId", origId);
	cJSON_AddNumberToObject(value,"timestamp", app_time_server_get_timestamp_now());

	char* data_string = cJSON_PrintUnformatted(root);
	if (g_bMqttstatus)
		app_mqtt_endpoint_link_transmit(g_endpoint_link_arr[endpoint].chdesDevId,data_string);
	else
		app_udp_client_send_broadcast_endpoint_link(data_string);
	cJSON_Delete(root);
	free(data_string);
}

static char httpData[2048] = {0};
static int  httpDataLen = 0;

void 		app_cmd_send_check_force_ota(void) {
	char *data_string;
	cJSON *x_root = cJSON_CreateObject();

	cJSON_AddStringToObject(x_root, "deviceId", app_cmd_get_deviceId());
	uint64_t timestamp = app_time_server_get_timestamp_now();
	char  checksum[128] = {0};
	char  buffer[256] = {0};
	sprintf(buffer, "%s%lld%s", app_cmd_get_deviceId(), timestamp, g_CloudInfo.pBeSharedKey);
	app_cmd_sha256(buffer, checksum);
	cJSON_AddStringToObject(x_root, "checksum", checksum);
	cJSON_AddNumberToObject(x_root, "timestamp", timestamp);
	data_string = cJSON_PrintUnformatted(x_root);
	LREP_WARNING(__FUNCTION__, "%s", buffer);
	LREP_WARNING(__FUNCTION__, "%s", data_string);
	LREP_WARNING(__FUNCTION__, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	httpDataLen = 0;
	memset(httpData, 0, sizeof(httpData));

	esp_http_client_config_t config = {
		.url = g_CloudInfo.pForceOtaUrl,
//		.url = "https://smarthome-api2.vconnex.vn/smarthome-api/api/device/check-ota-folkupdate",
		.event_handler = _http_event_handler,
		.timeout_ms = 5000,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	esp_err_t err;
	esp_http_client_set_method(client, HTTP_METHOD_POST);
	esp_http_client_set_post_field(client, data_string, strlen(data_string));
	while (1) {
		err = esp_http_client_perform(client);
		if (err != ESP_ERR_HTTP_EAGAIN) {
			break;
		}
	}
	if (err == ESP_OK) {
		LREP(__FUNCTION__, "HTTPS Status = %d, content_length = %d",
				esp_http_client_get_status_code(client),
				esp_http_client_get_content_length(client));
	} else {
		LREP_ERROR(__FUNCTION__, "Error perform http request %s", esp_err_to_name(err));
	}
	esp_http_client_cleanup(client);
	free(data_string);
	cJSON_Delete(x_root);

	LREP_WARNING(__FUNCTION__, "%d %s", httpDataLen, httpData);
#if ENCRYPTED

#else
	bool bUpdate = false;
	bool bAuto = false;
	cJSON* root = cJSON_Parse(httpData);
	if (!json_check_object(root)) {
		LREP_WARNING(__FUNCTION__, "root is null, check json");
		goto DELETE;
	}

	cJSON* pCode = cJSON_GetObjectItem(root, "code");
	cJSON* pStatus = cJSON_GetObjectItem(root, "status");

	if (!json_check_number(pCode) || !json_check_string(pStatus)) {
		LREP_WARNING(__FUNCTION__, "json error: code & status");
		goto DELETE;
	}

	if (pCode->valueint != 1) {
		LREP_WARNING(__FUNCTION__, "response error %s", pStatus->valuestring);
		goto DELETE;
	}

	cJSON* pData = cJSON_GetObjectItem(root, "data");
	if (!json_check_object(pData)) {
		LREP_WARNING(__FUNCTION__, "json error: data");
		goto DELETE;
	}

	cJSON* pJsValue = cJSON_GetObjectItem(pData, "value");
	if (!json_check_object(pJsValue)) {
		LREP_WARNING(__FUNCTION__, "json error: value");
		goto DELETE;
	}

	if (app_cmd_value_check_devT(pJsValue) != ESP_OK || app_cmd_value_check_devExtAddr(pJsValue) != ESP_OK) {
		LREP_WARNING(__FUNCTION__, "devT or devExtAddr invalid");
		goto DELETE;
	}

	cJSON* pVer = cJSON_GetObjectItem(pJsValue, "version");
	cJSON* pUrl = cJSON_GetObjectItem(pJsValue, "url");
	cJSON* pTimeStart = cJSON_GetObjectItem(pJsValue, "timeStart");
	cJSON* pTimeEnd = cJSON_GetObjectItem(pJsValue, "timeEnd");
	cJSON* pTimezone = cJSON_GetObjectItem(pJsValue, "timezone");
	cJSON* pSkdId = cJSON_GetObjectItem(pJsValue, "scheduleId");
	cJSON* pDateStart = cJSON_GetObjectItem(pJsValue, "dateStart");
	cJSON* pDateEnd = cJSON_GetObjectItem(pJsValue, "dateEnd");
	cJSON* pType = cJSON_GetObjectItem(pJsValue,"state");

	if (!json_check_string(pVer) || !json_check_string(pUrl)) {
		LREP_WARNING(__FUNCTION__, "version or url invalid");
		goto DELETE;
	}

	if (!json_check_number(pTimeStart) || !json_check_number(pTimeEnd) || !json_check_number(pTimezone) ||
			!json_check_number(pSkdId) || !json_check_number(pDateStart) || !json_check_number(pDateEnd) || !json_check_string(pType)) {
		LREP_WARNING(__FUNCTION__, "force ota schedule invalid");
		goto DELETE;
	}

	bAuto = (strcmp(pType->valuestring, "AUTO") == 0) ? true : false;

	do {
		bUpdate = true;
		if (strcmp(pVer->valuestring, g_forceOta.strVersion) != 0) break;
		if (strcmp(pUrl->valuestring, g_forceOta.strUrl) != 0) break;

		if (pTimeStart->valueint != g_forceOta.forceOtaSkd.u64TimeStart) break;
		if (pTimeEnd->valueint != g_forceOta.forceOtaSkd.u64TimeEnd) break;
		if (pDateStart->valueint != g_forceOta.forceOtaSkd.u64DateStart) break;
		if (pDateEnd->valueint != g_forceOta.forceOtaSkd.u64DateEnd) break;
		if (pTimezone->valuedouble != g_forceOta.forceOtaSkd.fTimezone) break;
		if (pSkdId->valueint != g_forceOta.forceOtaSkd.iSkdId) break;
		if (bAuto != g_forceOta.bAuto) break;

		bUpdate = false;
	} while (0);

	if (!bUpdate) {
		LREP_WARNING(__FUNCTION__, "Force OTA schedule is the same");
		goto DELETE;
	}

	g_forceOta.forceOtaSkd.u64TimeStart = pTimeStart->valueint;
	g_forceOta.forceOtaSkd.u64TimeEnd = pTimeEnd->valueint;
	g_forceOta.forceOtaSkd.u64DateStart = pDateStart->valueint;
	g_forceOta.forceOtaSkd.u64DateEnd = pDateEnd->valueint;
	g_forceOta.forceOtaSkd.fTimezone = pTimezone->valuedouble;
	g_forceOta.forceOtaSkd.iSkdId = pSkdId->valueint;
	memset(g_forceOta.strUrl, 0, sizeof(g_forceOta.strUrl));
	strcpy(g_forceOta.strUrl, pUrl->valuestring);
	memset(g_forceOta.strVersion, 0, sizeof(g_forceOta.strVersion));
	strcpy(g_forceOta.strVersion, pVer->valuestring);
	g_forceOta.bAuto = bAuto;
	g_forceOta.bUpdated = false;
	app_filesystem_write_force_ota_info(&g_forceOta);
	app_force_ota_init();

	DELETE: cJSON_Delete(root);
#endif


}

void		app_cmd_sha256(const char* pStrInput, char* pStrOutput) {
	const unsigned char* pStr = (const unsigned char*)pStrInput;
	unsigned char output[32];
	mbedtls_sha256_context ctx;
	mbedtls_sha256_init(&ctx);
	mbedtls_sha256_starts(&ctx, 0);
	mbedtls_sha256_update(&ctx, pStr, strlen(pStrInput));
	mbedtls_sha256_finish(&ctx, output);
	mbedtls_sha256_free(&ctx);
	sprintf(pStrOutput,"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
			output[0], output[1], output[2], output[3], output[4], output[5], output[6], output[7],
			output[8], output[9], output[10], output[11], output[12], output[13], output[14], output[15],
			output[16], output[17], output[18], output[19], output[20], output[21], output[22], output[23],
			output[24], output[25], output[26], output[27], output[28], output[29], output[30], output[31]);
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
   switch(evt->event_id) {
		case HTTP_EVENT_ERROR:
			LREP_WARNING("http_client", "HTTP_EVENT_ERROR");
			break;
		case HTTP_EVENT_ON_CONNECTED:
			LREP_WARNING("http_client", "HTTP_EVENT_ON_CONNECTED");
			break;
		case HTTP_EVENT_HEADER_SENT:
			LREP_WARNING("http_client", "HTTP_EVENT_HEADER_SENT");
			break;
		case HTTP_EVENT_ON_HEADER:
			LREP("http_client", "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
			break;
		case HTTP_EVENT_ON_DATA:
			LREP("http_client", "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
			if (!esp_http_client_is_chunked_response(evt->client)) {
				// Write out data
				 LREP("http_client", "%.*s", evt->data_len, (char*)evt->data);
				 memcpy(httpData + httpDataLen, (char*)evt->data, evt->data_len);
				 httpDataLen += evt->data_len;
			}
			break;
		case HTTP_EVENT_ON_FINISH:
			ESP_LOGD("http_client", "HTTP_EVENT_ON_FINISH");
			break;
		case HTTP_EVENT_DISCONNECTED:
			LREP("http_client", "HTTP_EVENT_DISCONNECTED");
			int mbedtls_err = 0;
			esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
			if (err != 0) {
				LREP("http_client", "Last esp error code: 0x%x", err);
				LREP("http_client", "Last mbedtls failure: 0x%x", mbedtls_err);
			}
			break;
		default:
			break;
	}
	return ESP_OK;
}

void 		app_cmd_led_SetExtraConfig(const cJSON* pJsValue) {
	if (pJsValue == NULL) return;

	cJSON* pLedEnb = cJSON_GetObjectItem(pJsValue, "ledEnb");
	cJSON* pLedRgbOn = cJSON_GetObjectItem(pJsValue, "ledRgbOn");
	cJSON* pLedRgbOff = cJSON_GetObjectItem(pJsValue,"ledRgbOff");
	cJSON* pLedLightness = cJSON_GetObjectItem(pJsValue, "led_lightness");
	if (json_check_number(pLedEnb)) {
		LREP_WARNING(__FUNCTION__, "set Led %s", (pLedEnb->valueint == 1) ? "ENABLE" : "DISABLE");
		g_Config.iLedEnb = (pLedEnb->valueint == 1) ? 1 : 0;
	}
	if (json_check_number(pLedRgbOn)) {
		LREP_WARNING(__FUNCTION__, "set ledRGBOn %d", pLedRgbOn->valueint);
		gpio_set_led_color(0, 1, pLedRgbOn->valueint);
	}
	if (json_check_number(pLedRgbOff)) {
		LREP_WARNING(__FUNCTION__, "set ledRGBOff %d", pLedRgbOff->valueint);
		gpio_set_led_color(0, 0, pLedRgbOff->valueint);
	}
	if (json_check_number(pLedLightness) && pLedLightness->valueint >= 0 && pLedLightness->valueint <=100) {
		LREP_WARNING(__FUNCTION__, "set led lightness %d", pLedLightness->valueint);
		gpio_set_led_lightness(0, pLedLightness->valueint);
	}
}

void		app_cmd_channel_SetExtraConfig(const cJSON* pJsValue, int channel) {
	if (pJsValue == NULL) return;
	if (channel <= 0 || channel > SW_CHANNEL_4) return;
	char json_string[64];

	sprintf(json_string, "switch_%d_type", channel);
	cJSON *pSwType =  cJSON_GetObjectItem(pJsValue, json_string);
	if (json_check_number(pSwType)) {
		LREP_WARNING(__FUNCTION__, "Set Sw%d type %d", channel, pSwType->valueint);
		gpio_set_channel_type(channel, pSwType->valueint);
	}

	sprintf(json_string, "switch_%d_countdown", channel);
	cJSON *pSwCd = cJSON_GetObjectItem(pJsValue, json_string);
	if (json_check_number(pSwCd)) {
		LREP_WARNING(__FUNCTION__,"Set Sw%d count down %d seconds", channel, pSwCd->valueint);
		gpio_set_channel_countdown(channel, pSwCd->valueint);
	}

	sprintf(json_string, "switch_%d_control_mode", channel);
	cJSON *pSwCtrlMode = cJSON_GetObjectItem(pJsValue, json_string);
	if (json_check_number(pSwCtrlMode)) {
		LREP_WARNING(__FUNCTION__,"Set Sw%d Ctrl mode %d", channel, pSwCtrlMode->valueint);
		gpio_set_channel_ctrl_mode(channel, pSwCtrlMode->valueint);
	}

	sprintf(json_string, "switch_%d_led_off", channel);
	cJSON *pSwLedOff = cJSON_GetObjectItem(pJsValue, json_string);
	if (json_check_number(pSwLedOff)) {
		LREP_WARNING(__FUNCTION__,"Set Sw%d led off %d", channel, pSwLedOff->valueint);
		if (pSwLedOff->valueint) gpio_disable_led_channel(channel);
		else gpio_enable_led_channel(channel);
	}

	sprintf(json_string, "switch_%d_rgb_on", channel);
	cJSON *pSwRgbOn = cJSON_GetObjectItem(pJsValue, json_string);
	if (json_check_number(pSwRgbOn)) {
		LREP_WARNING(__FUNCTION__,"Set Sw%d Rgb On %d", channel, pSwRgbOn->valueint);
		gpio_set_led_color(channel, 1, pSwRgbOn->valueint);
	}

	sprintf(json_string, "switch_%d_rgb_off", channel);
	cJSON *pSwRgbOff = cJSON_GetObjectItem(pJsValue, json_string);
	if (json_check_number(pSwRgbOff)) {
		LREP_WARNING(__FUNCTION__,"Set Sw%d Rgb Off %d", channel, pSwRgbOff->valueint);
		gpio_set_led_color(channel, 0, pSwRgbOff->valueint);
	}

	sprintf(json_string, "switch_%d_lightness", channel);
	cJSON *pSwLightness = cJSON_GetObjectItem(pJsValue, json_string);
	if (json_check_number(pSwLightness) && pSwLightness->valueint >= 0 && pSwLightness->valueint <= 100) {
		LREP_WARNING(__FUNCTION__, "Set Sw%d Lightness %d", channel, pSwLightness->valueint);
		gpio_set_led_lightness(channel, pSwLightness->valueint);
	}
}

void 		app_cmd_curtain_SetExtraConfig(const cJSON* pJsValue) {
	cJSON* pCurtainCycle = cJSON_GetObjectItem(pJsValue, "curtain_cycle");
	if (json_check_number(pCurtainCycle)) {
		LREP_WARNING(__FUNCTION__, "Set curtain cycle %d", pCurtainCycle->valueint);
		if (g_Config.curtain_cycle != pCurtainCycle->valueint) {
			g_Config.curtain_cycle = pCurtainCycle->valueint;
		}
	}

	cJSON* pCurtainCycle2 = cJSON_GetObjectItem(pJsValue, "curtain_2_cycle");
	if (json_check_number(pCurtainCycle2)) {
		LREP_WARNING(__FUNCTION__, "Set curtain 2 cycle %d", pCurtainCycle2->valueint);
		if (g_Config.curtain_2_cycle != pCurtainCycle2->valueint) {
			g_Config.curtain_2_cycle = pCurtainCycle2->valueint;
		}
	}
}

void 		app_cmd_night_SetExtraConfig(const cJSON* pJsValue) {
	if (pJsValue == NULL) return;
	cJSON *pNightMode = cJSON_GetObjectItem(pJsValue, "nightModeEnb");
	cJSON *pNightBegin = cJSON_GetObjectItem(pJsValue, "nightBegin");
	cJSON *pNightEnd = cJSON_GetObjectItem(pJsValue, "nightEnd");
	cJSON *pNightTz = cJSON_GetObjectItem(pJsValue, "nightTz");
	if (json_check_number(pNightMode)) {
		LREP_WARNING(__FUNCTION__, "Set Night mode %d", pNightMode->valueint);
		if (pNightMode->valueint) gpio_enable_nightmode();
		else gpio_disable_nightmode();
	}
	if (json_check_number(pNightBegin)) {
		LREP_WARNING(__FUNCTION__, "Set Night begin %d", pNightBegin->valueint);
		gpio_set_night_begin(pNightBegin->valueint);
	}
	if (json_check_number(pNightEnd)) {
		LREP_WARNING(__FUNCTION__, "Set Night end %d", pNightEnd->valueint);
		gpio_set_night_end(pNightEnd->valueint);
	}
	if (json_check_number(pNightTz)) {
		LREP_WARNING(__FUNCTION__, "Set Night tz %d", pNightTz->valueint);
		gpio_set_night_timezone(pNightTz->valueint);
	}
}

void 		app_cmd_build_response_ExtraConfig(cJSON *root) {
	if (root == NULL) return;
#ifdef DEVICE_TYPE
	cJSON_AddNumberToObject(root, "buzzerEnb", g_Config.iBuzzerEnb);
	cJSON_AddNumberToObject(root, "ledEnb", g_Config.iLedEnb);
	cJSON_AddNumberToObject(root, "ledRgbOn", g_Config.u32RgbOn);
	cJSON_AddNumberToObject(root, "ledRbgOff", g_Config.u32RgbOff);
	cJSON_AddNumberToObject(root, "led_lightness", g_Config.led_lighness);
	cJSON_AddNumberToObject(root, "switch_1_type", g_Config.channel_1_mode);
	cJSON_AddNumberToObject(root, "switch_1_countdown", g_Config.channel_1_countdown);
	cJSON_AddNumberToObject(root, "switch_1_control_mode", g_Config.channel_1_control_mode);
	cJSON_AddNumberToObject(root, "switch_1_led_off", g_Config.channel_1_led_off);
	cJSON_AddNumberToObject(root, "switch_1_rgb_on", g_Config.channel_1_rgb_on);
	cJSON_AddNumberToObject(root, "switch_1_rgb_off", g_Config.channel_1_rgb_off);
	cJSON_AddNumberToObject(root, "switch_1_lightness", g_Config.channel_1_lightness);
	cJSON_AddNumberToObject(root, "switch_2_type", g_Config.channel_2_mode);
	cJSON_AddNumberToObject(root, "switch_2_countdown", g_Config.channel_2_countdown);
	cJSON_AddNumberToObject(root, "switch_2_control_mode", g_Config.channel_2_control_mode);
	cJSON_AddNumberToObject(root, "switch_2_led_off", g_Config.channel_2_led_off);
	cJSON_AddNumberToObject(root, "switch_2_rgb_on", g_Config.channel_2_rgb_on);
	cJSON_AddNumberToObject(root, "switch_2_rgb_off", g_Config.channel_2_rgb_off);
	cJSON_AddNumberToObject(root, "switch_2_lightness", g_Config.channel_2_lightness);
	cJSON_AddNumberToObject(root, "switch_3_type", g_Config.channel_3_mode);
	cJSON_AddNumberToObject(root, "switch_3_countdown", g_Config.channel_3_countdown);
	cJSON_AddNumberToObject(root, "switch_3_control_mode", g_Config.channel_3_control_mode);
	cJSON_AddNumberToObject(root, "switch_3_led_off", g_Config.channel_3_led_off);
	cJSON_AddNumberToObject(root, "switch_3_rgb_on", g_Config.channel_3_rgb_on);
	cJSON_AddNumberToObject(root, "switch_3_rgb_off", g_Config.channel_3_rgb_off);
	cJSON_AddNumberToObject(root, "switch_3_lightness", g_Config.channel_3_lightness);
	cJSON_AddNumberToObject(root, "nightModeEnb", g_Config.iNightmodeEnb);
	cJSON_AddNumberToObject(root, "nightBegin", g_Config.u32NightBegin);
	cJSON_AddNumberToObject(root, "nightEnd", g_Config.u32NightEnd);
//	cJSON_AddNumberToObject(root, "nightTz", g_Config.iNightTimezone);
	char strBuff[20] = {0};
	sprintf(strBuff, "%.2f", (float)g_Config.iNightTimezone);
	cJSON_AddRawToObject(root, "nightTz", strBuff);
	cJSON_AddNumberToObject(root, "saveStt", g_Config.saveStt);
#endif
}

