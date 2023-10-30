/*
 * app_ota.h
 *
 *  Created on: Mar 10, 2020
 *      Author: laian
 */

#ifndef __APP_OTA_H
#define __APP_OTA_H

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
//#include "app_control_json.h"
#include "config.h"
#include "app_spiffs.h"
#include "app_cmd_mgr.h"

extern bool g_otaRunning;

typedef struct {
	uint64_t u64TimeStart;
	uint64_t u64TimeEnd;
	uint64_t u64DateStart;
	uint64_t u64DateEnd;
	float fTimezone;
	int iSkdId;
} sForceOtaSkd_t;

typedef struct {
	/*parameters from input*/
	char strUrl[128];
	char strVersion[64];
	sForceOtaSkd_t forceOtaSkd;
//	bool bManual;
	bool bAuto;
	/*parameters for handle in program*/
	bool  bUpdated;
} sForceOta_t;

typedef enum {
	OTA_TYPE_NORMAL,
	OTA_TYPE_FORCE,
} eOTA_t;

extern sForceOta_t   g_forceOta;

void app_ota_running_partition_check();
bool app_ota_diagnostic();

esp_err_t app_ota_set_server_url(const char* severUrl);
esp_err_t app_ota_set_update_version(const char* version);
esp_err_t app_ota_start();

const char* app_ota_get_software_version();

void app_ota_task_fatal_err();

void app_force_ota_init(void);
bool app_force_ota_condition_check(uint64_t timestamp);
void app_force_ota_process(void);
void app_force_ota_start(void);

#endif /* __APP_OTA_H */
