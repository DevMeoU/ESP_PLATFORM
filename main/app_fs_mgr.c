/*
 * app_fs_mgr.c
 *
 *  Created on: Aug 28, 2020
 *      Author: laian
 */

#include "app_fs_mgr.h"
#include "stdlib.h"

#define APP_FS_WRITE_LINE(__BUF, __STR, __FILE) \
	do {\
		memset(__BUF, 0, sizeof(__BUF));\
		sprintf(__BUF, "%s\r\n", (__STR != NULL) ? __STR : "");\
		fputs(__BUF, __FILE);\
	} while(0)

#define APP_FS_WRITE_LINE_F(__BUF, __F, __FILE) \
	do { \
		memset(__BUF, 0, sizeof(__BUF));\
		sprintf(__BUF, "%.2f\r\n", __F);\
		fputs(__BUF, __FILE);\
	} while(0)

#define APP_FS_WRITE_LINE_I(__BUF, __I, __FILE) \
	do{\
		memset(__BUF, 0, sizeof(__BUF));\
		sprintf(__BUF, "%d\r\n", __I);\
		fputs(__BUF, __FILE);\
	}while(0)

#define APP_FS_WRITE_LINE_LLI(__BUF, __LLI, __FILE) \
	do{\
		memset(__BUF, 0, sizeof(__BUF));\
		sprintf(__BUF, "%lld\r\n", __LLI);\
		fputs(__BUF, __FILE);\
	}while(0)

void app_filesystem_read_cloud_info(sCloudInfo_t* pCloudInfo) {
	if (app_spiffs_check_file_exist(APP_DEVICE_BROKER_STORAGE_FILE)) {
		char buf[256]={0};
		FILE * fPtr = fopen(APP_DEVICE_BROKER_STORAGE_FILE, "r");
		if (fPtr!=NULL) {
			int countline =0;
			while (fgets(buf, sizeof(buf), fPtr) != NULL) {
				countline++;
				switch(countline) {
				case 1:
					memset(pCloudInfo->pBrokerUrl, 0, 100);
					memcpy(pCloudInfo->pBrokerUrl, buf, strlen(buf) - 2);
					LREP_WARNING(__FUNCTION__, "broker Url %s", pCloudInfo->pBrokerUrl);
					break;
				case 2:
					memset(pCloudInfo->pBrokerUsr, 0, 100);
					memcpy(pCloudInfo->pBrokerUsr, buf, strlen(buf) - 2);
					break;
				case 3:
					memset(pCloudInfo->pBrokerPass, 0, 100);
					memcpy(pCloudInfo->pBrokerPass, buf, strlen(buf) - 2);
					break;
				case 4:
					memset(pCloudInfo->pTopicSub, 0,  MQTT_MAXIMUM_TOPIC_LENGTH);
					memcpy(pCloudInfo->pTopicSub, buf, strlen(buf) - 2);
					break;
				case 5:
					memset(pCloudInfo->pTopicPub, 0, MQTT_MAXIMUM_TOPIC_LENGTH);
					memcpy(pCloudInfo->pTopicPub, buf, strlen(buf) - 2);
					break;
				case 6:
					memset(pCloudInfo->pTopicAllert, 0, MQTT_MAXIMUM_TOPIC_LENGTH);
					memcpy(pCloudInfo->pTopicAllert, buf, strlen(buf) - 2);
					break;
				case 7:
					memset(pCloudInfo->pForceOtaUrl, 0 , 100);
					memcpy(pCloudInfo->pForceOtaUrl, buf, strlen(buf) - 2);
					break;
				case 8:
					memset(pCloudInfo->pBeSharedKey, 0, 100);
					memcpy(pCloudInfo->pBeSharedKey, buf, strlen(buf) - 2);
					break;
				default:
					break;
				}
				memset(buf, 0, 256);
			}
		}
		else LREP_ERROR(__FUNCTION__, "fopen ERROR");
		fclose(fPtr);
	}
	else  {
		LREP_WARNING(__FUNCTION__, "cloud info file is not exist");
	}
}


void app_filesystem_write_cloud_info(const sCloudInfo_t* pCloudInfo) {
	if (app_spiffs_check_file_exist(APP_DEVICE_BROKER_STORAGE_FILE)) {
		LREP_WARNING(__FUNCTION__, "Delete old cloud info file");
		if (app_spiffs_delete_file(APP_DEVICE_BROKER_STORAGE_FILE) != ESP_OK) {
			LREP_ERROR(__FUNCTION__, "Cannot delete old cloud info file");
			return;
		}
	}
	char buf[256]={0};

	FILE * fPtr = fopen(APP_DEVICE_BROKER_STORAGE_FILE, "ab");
	if (fPtr != NULL) {
		APP_FS_WRITE_LINE(buf, pCloudInfo->pBrokerUrl, fPtr);
		APP_FS_WRITE_LINE(buf, pCloudInfo->pBrokerUsr, fPtr);
		APP_FS_WRITE_LINE(buf, pCloudInfo->pBrokerPass, fPtr);
		APP_FS_WRITE_LINE(buf, pCloudInfo->pTopicSub, fPtr);
		APP_FS_WRITE_LINE(buf, pCloudInfo->pTopicPub, fPtr);
		APP_FS_WRITE_LINE(buf, pCloudInfo->pTopicAllert, fPtr);
		APP_FS_WRITE_LINE(buf, pCloudInfo->pForceOtaUrl, fPtr);
		APP_FS_WRITE_LINE(buf, pCloudInfo->pBeSharedKey, fPtr);
	}
	else LREP_ERROR(__FUNCTION__, "fopen ERROR");
	fclose(fPtr);

}

void app_filesystem_delete_cloud_info(void) {
	if (app_spiffs_check_file_exist(APP_DEVICE_BROKER_STORAGE_FILE)) {
		LREP_WARNING(__FUNCTION__, "Delete old cloud info file");
		if (app_spiffs_delete_file(APP_DEVICE_BROKER_STORAGE_FILE) != ESP_OK) {
			LREP_ERROR(__FUNCTION__, "Cannot delete old cloud info file");
			return;
		}
	}
}

esp_err_t app_filesystem_read_force_ota_info(sForceOta_t* pForceOtaInfo) {
	if (app_spiffs_check_file_exist(APP_FORCE_OTA_FILE)) {
		char buf[256] = {0};
		FILE * fPtr = fopen(APP_FORCE_OTA_FILE, "r");
		if (fPtr != NULL) {
			int countline =0;
			while (fgets(buf, sizeof(buf), fPtr) != NULL) {
				countline++;
#if 0
				LREP(__FUNCTION__, "%s", buf);
#endif
				switch(countline) {
				case 1:
					memset(pForceOtaInfo->strUrl, 0, 128);
					memcpy(pForceOtaInfo->strUrl, buf, strlen(buf) - 2);
					break;
				case 2:
					memset(pForceOtaInfo->strVersion, 0, 64);
					memcpy(pForceOtaInfo->strVersion, buf, strlen(buf) - 2);
					break;
				case 3:
					pForceOtaInfo->forceOtaSkd.u64TimeStart = atoll(buf);
					break;
				case 4:
					pForceOtaInfo->forceOtaSkd.u64TimeEnd = atoll(buf);
					break;
				case 5:
					pForceOtaInfo->forceOtaSkd.fTimezone = atof(buf);
					break;
				case 6:
					pForceOtaInfo->forceOtaSkd.iSkdId = atoi(buf);
					break;
				case 7:
					pForceOtaInfo->bUpdated = (atoi(buf) == 1) ? true : false;
					break;
				case 8:
					pForceOtaInfo->forceOtaSkd.u64DateStart = atoll(buf);
					break;
				case 9:
					pForceOtaInfo->forceOtaSkd.u64DateEnd = atoll(buf);
					break;
				case 10:
					pForceOtaInfo->bAuto = (atoi(buf) == 1) ? true : false;
					break;
				default:
					break;
				}
				memset(buf, 0, 256);
			}
		}
		else LREP_ERROR(__FUNCTION__, "fopen error");
		fclose(fPtr);
		return ESP_OK;
	}
	else {
		LREP_WARNING(__FUNCTION__,"force ota file not found");
		return ESP_FAIL;
	}
}

void app_filesystem_write_force_ota_info(const sForceOta_t* pForceOtaInfo) {
	if (app_spiffs_check_file_exist(APP_FORCE_OTA_FILE)) {
		LREP_WARNING(__FUNCTION__, "Delete old force ota info file");
		if (app_spiffs_delete_file(APP_FORCE_OTA_FILE) != ESP_OK) {
			LREP_ERROR(__FUNCTION__, "Cannot delete old cloud info file");
			return;
		}
	}

	char buf[256] = {0};
	FILE * fPtr = fopen(APP_FORCE_OTA_FILE, "ab");
	if (fPtr != NULL) {
		APP_FS_WRITE_LINE(buf, pForceOtaInfo->strUrl, fPtr);
		APP_FS_WRITE_LINE(buf, pForceOtaInfo->strVersion, fPtr);
		APP_FS_WRITE_LINE_LLI(buf, pForceOtaInfo->forceOtaSkd.u64TimeStart, fPtr);
		APP_FS_WRITE_LINE_LLI(buf, pForceOtaInfo->forceOtaSkd.u64TimeEnd, fPtr);
		APP_FS_WRITE_LINE_F(buf, pForceOtaInfo->forceOtaSkd.fTimezone, fPtr);
		APP_FS_WRITE_LINE_I(buf, pForceOtaInfo->forceOtaSkd.iSkdId, fPtr);
		APP_FS_WRITE_LINE_I(buf, pForceOtaInfo->bUpdated ? 1 : 0, fPtr);
		APP_FS_WRITE_LINE_LLI(buf, pForceOtaInfo->forceOtaSkd.u64DateStart, fPtr);
		APP_FS_WRITE_LINE_LLI(buf, pForceOtaInfo->forceOtaSkd.u64DateEnd, fPtr);
		APP_FS_WRITE_LINE_I(buf, pForceOtaInfo->bAuto ? 1: 0, fPtr);
	}
	else LREP_ERROR(__FUNCTION__, "fopen ERROR");
	fclose (fPtr);
}

void app_filesystem_read_config(sSwRgbConfig_t* pConfig) {
	if (app_spiffs_check_file_exist(APP_GPIO_CONFIG_FILE)) {
		char buf[256] = {0};
		FILE * fPtr = fopen(APP_GPIO_CONFIG_FILE, "r");

		if (fPtr != NULL) {
			int countline = 0;
			while (fgets(buf, sizeof(buf), fPtr) != NULL) {
				countline++;
#if 0
				LREP(__FUNCTION__, "%s", buf);
#endif
				switch(countline) {
				case 1: pConfig->iBuzzerEnb = atoi(buf); break;
				case 2: pConfig->iLedEnb = atoi(buf); break;
				case 3: pConfig->u32RgbOn = atoi(buf); break;
				case 4: pConfig->u32RgbOff = atoi(buf); break;
				case 5: pConfig->channel_1_mode = atoi(buf); break;
				case 6: pConfig->channel_2_mode = atoi(buf); break;
				case 7: pConfig->channel_3_mode = atoi(buf); break;
				case 8: pConfig->channel_4_mode = atoi(buf); break;
				case 9: pConfig->channel_1_countdown = atoi(buf); break;
				case 10: pConfig->channel_2_countdown = atoi(buf); break;
				case 11: pConfig->channel_3_countdown = atoi(buf); break;
				case 12: pConfig->channel_4_countdown = atoi(buf); break;
				case 13: pConfig->curtain_cycle = atoi(buf); break;
				case 14: pConfig->curtain_2_cycle = atoi(buf); break;
				case 15: pConfig->channel_1_control_mode = atoi(buf); break;
				case 16: pConfig->channel_2_control_mode = atoi(buf); break;
				case 17: pConfig->channel_3_control_mode = atoi(buf); break;
				case 18: pConfig->channel_4_control_mode = atoi(buf); break;
				case 19: pConfig->channel_1_led_off = atoi(buf); break;
				case 20: pConfig->channel_1_rgb_on = atoi(buf); break;
				case 21: pConfig->channel_1_rgb_off = atoi(buf); break;
				case 22: pConfig->channel_2_led_off = atoi(buf); break;
				case 23: pConfig->channel_2_rgb_on = atoi(buf); break;
				case 24: pConfig->channel_2_rgb_off = atoi(buf); break;
				case 25: pConfig->channel_3_led_off = atoi(buf); break;
				case 26: pConfig->channel_3_rgb_on = atoi(buf); break;
				case 27: pConfig->channel_3_rgb_off = atoi(buf); break;
				case 28: pConfig->channel_4_led_off = atoi(buf); break;
				case 29: pConfig->channel_4_rgb_on = atoi(buf); break;
				case 30: pConfig->channel_4_rgb_off = atoi(buf); break;
				case 31: pConfig->iNightmodeEnb = atoi(buf); break;
				case 32: pConfig->u32NightBegin = atoi(buf); break;
				case 33: pConfig->u32NightEnd = atoi(buf); break;
				case 34: pConfig->iNightTimezone = atoi(buf); break;
				case 35: pConfig->channel_1_lightness = atoi(buf); break;
				case 36: pConfig->channel_2_lightness = atoi(buf); break;
				case 37: pConfig->channel_3_lightness = atoi(buf); break;
				case 38: pConfig->channel_4_lightness = atoi(buf); break;
				case 39: pConfig->led_lighness = atoi(buf); break;
				case 40: pConfig->saveStt = atoi(buf); break;
				default:
					break;
				}
				memset(buf, 0, 256);
			}
		}
		else LREP_ERROR(__FUNCTION__, "fopen error");
		fclose(fPtr);
	}
	else {
		LREP_WARNING(__FUNCTION__, "gpio config file not found");
	}
}

void app_filesystem_write_config(const sSwRgbConfig_t* pConfig) {
	if (app_spiffs_check_file_exist(APP_GPIO_CONFIG_FILE)) {
		LREP_WARNING(__FUNCTION__, "Delete old gpio config file");
		if (app_spiffs_delete_file(APP_GPIO_CONFIG_FILE) != ESP_OK) {
			LREP_ERROR(__FUNCTION__, "Cannot delete gpio config file");
			return;
		}
	}

	char buf[256] = {0};
	FILE * fPtr = fopen(APP_GPIO_CONFIG_FILE, "ab");
	if (fPtr != NULL) {
		APP_FS_WRITE_LINE_I(buf, pConfig->iBuzzerEnb, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->iLedEnb, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->u32RgbOn, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->u32RgbOff, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_1_mode, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_2_mode, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_3_mode, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_4_mode, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_1_countdown, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_2_countdown, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_3_countdown, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_4_countdown, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->curtain_cycle, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->curtain_2_cycle, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_1_control_mode, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_2_control_mode, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_3_control_mode, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_4_control_mode, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_1_led_off, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_1_rgb_on, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_1_rgb_off, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_2_led_off, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_2_rgb_on, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_2_rgb_off, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_3_led_off, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_3_rgb_on, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_3_rgb_off, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_4_led_off, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_4_rgb_on, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_4_rgb_off, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->iNightmodeEnb, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->u32NightBegin, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->u32NightEnd, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->iNightTimezone, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_1_lightness, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_2_lightness, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_3_lightness, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->channel_4_lightness, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->led_lighness, fPtr);
		APP_FS_WRITE_LINE_I(buf, pConfig->saveStt, fPtr);
	}
	else LREP_ERROR(__FUNCTION__, "fopen ERROR");
	fclose (fPtr);
}

void app_filesystem_delete_config(void) {
	if (app_spiffs_check_file_exist(APP_GPIO_CONFIG_FILE)) {
		LREP_WARNING(__FUNCTION__, "Delete old gpio config file");
		if (app_spiffs_delete_file(APP_GPIO_CONFIG_FILE) != ESP_OK) {
			LREP_ERROR(__FUNCTION__, "Cannot delete gpio config file");
			return;
		}
	}
}

int app_filesytem_read_status(int channel) {
	nvs_handle_t nvs_handle;
	int err = nvs_open("relay_stt", NVS_READWRITE, &nvs_handle);
	if (err != ESP_OK) {
		LREP_ERROR(__FUNCTION__, "nvs open error %d", err);
		return 0;
	}
	char key[10] = {0};
	int value = 0;
	sprintf(key,"Relay_%d", channel);
	err = nvs_get_i32(nvs_handle, key, &value);
	if (err != ESP_OK) LREP_ERROR(__FUNCTION__, "nvs get error %d", err);
//	ESP_ERR_NVS_NOT_FOUND
	nvs_close(nvs_handle);
	return value;
}

void app_filesytem_write_status(int channel, int status) {
	nvs_handle_t nvs_handle;
	int err = nvs_open("relay_stt", NVS_READWRITE, &nvs_handle);
	if (err != ESP_OK) {
		LREP_ERROR(__FUNCTION__, "nvs open error %d", err);
		return;
	}
	char key[10] = {0};
	sprintf(key, "Relay_%d", channel);
	err = nvs_set_i32(nvs_handle, key, status);
	if (err != ESP_OK) LREP_ERROR(__FUNCTION__, "nvs set error %d", err);

	nvs_commit(nvs_handle);
	nvs_close(nvs_handle);
}

