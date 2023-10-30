/*
 * app_ota.c
 *
 *  Created on: Mar 10, 2020
 *      Author: laian
 */
#include "app_ota.h"
#include "app_fs_mgr.h"
#include "app_time_server.h"
#include "app_mqtt.h"

#define BUFFSIZE 10000//1024

static TaskHandle_t xOtaHandle = NULL;

static char* ota_write_data = NULL;
static char strServerUrl[128] = {0};
static char strCurrentVer[64] = {0};
static char strUpdateVer[64] = {0};
bool g_otaRunning  = false;

static void task_ota(void *pvParameter);
void app_ota_task_fatal_err();
void app_ota_http_cleanup(esp_http_client_handle_t client);

sForceOta_t   g_forceOta;
static bool   bForceOtaRequire = false;

eOTA_t     otaType = OTA_TYPE_NORMAL;

void app_ota_running_partition_check() {
	esp_partition_t partition;

	partition = *esp_ota_get_running_partition();
	ESP_LOGI(__FUNCTION__, "running partition address %x", partition.address);
	ESP_LOGI(__FUNCTION__, "running partition size %x", partition.size);
	ESP_LOGI(__FUNCTION__, "running partition label %s", partition.label);
	ESP_LOGI(__FUNCTION__, "running partition type %s", partition.type ==  ESP_PARTITION_TYPE_DATA ? "DATA" : "APP");
	ESP_LOGI(__FUNCTION__, "running partition sub type %d",partition.subtype);

	esp_ota_img_states_t ota_state;

	if(esp_ota_get_state_partition(&partition, &ota_state) == ESP_OK) {
		if(ota_state == ESP_OTA_IMG_NEW) {
			LREP(__FUNCTION__, "ota_state ESP_OTA_IMG_NEW");
		} else if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
			LREP(__FUNCTION__, "ota_state ESP_OTA_IMG_PENDING_VERIFY");
			if(app_ota_diagnostic()){
				LREP(__FUNCTION__, "Diagnostics completed successfully! Continuing execution ...");
				esp_ota_mark_app_valid_cancel_rollback();
			} else {
				LREP_ERROR(__FUNCTION__, "Diagnostics failed! Start rollback to the previous version ...");
				esp_ota_mark_app_invalid_rollback_and_reboot();
			}
		} else if (ota_state == ESP_OTA_IMG_VALID) {
			LREP(__FUNCTION__, "ota_state ESP_OTA_IMG_VALID");
		} else if (ota_state == ESP_OTA_IMG_INVALID) {
			LREP(__FUNCTION__, "ota_state ESP_OTA_IMG_INVALID");
		} else if (ota_state == ESP_OTA_IMG_ABORTED) {
			LREP(__FUNCTION__, "ota_state ESP_OTA_IMG_ABORTED");
		} else if (ota_state == ESP_OTA_IMG_UNDEFINED) {
			LREP(__FUNCTION__, "ota_state ESP_OTA_IMG_UNDEFINED");
		}
	}
}

bool app_ota_diagnostic() {
	//todo: condition active ota partition in ESP_OTA_IMG_PENDING_VERIFY
	return true;
}

esp_err_t app_ota_start() {
	if(g_otaRunning) return ESP_FAIL;
	otaType = OTA_TYPE_NORMAL;
	app_mqtt_disconnect();

	xTaskCreate(&task_ota, "task_ota", 8192, NULL, 5, &xOtaHandle);
	g_otaRunning = true;
	return ESP_OK;
}

void task_ota(void *pvParameter) {
	LREP(__FUNCTION__, "Starting OTA");
	LREP_WARNING(__FUNCTION__, "Free memory: %d bytes", esp_get_free_heap_size());

	ota_write_data = (char*) malloc(BUFFSIZE + 1);
	if (ota_write_data == NULL) {
		LREP_ERROR(__FUNCTION__, "buffer malloc failed");
		app_ota_task_fatal_err();
	}

	esp_err_t err;
	esp_ota_handle_t update_handle = 0 ;
	int binary_file_length = 0;
	bool header_checked = false;

	const esp_partition_t *running = esp_ota_get_running_partition();
	LREP(__FUNCTION__, "Running partition type %d subtype %d (offset 0x%08x)",running->type, running->subtype, running->address);

	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
	LREP(__FUNCTION__, "Writing to partition subtype %d at offset 0x%x", update_partition->subtype, update_partition->address);

	err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
	if (err != ESP_OK) {
		LREP_ERROR(__FUNCTION__, "esp_ota_begin failed (%s)", esp_err_to_name(err));
		app_ota_task_fatal_err();
	}
	LREP(__FUNCTION__, "esp_ota_begin succeeded");

	esp_http_client_config_t config = {
			.url = (otaType == OTA_TYPE_NORMAL) ? strServerUrl : g_forceOta.strUrl,
	};
	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == NULL) {
		LREP_ERROR(__FUNCTION__, "Failed to initialise HTTP connection");
		app_ota_task_fatal_err();
	}

    err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        LREP_ERROR(__FUNCTION__, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        app_ota_task_fatal_err();
    }

    int ret = esp_http_client_fetch_headers(client);
    if(ret == -1) {
    	LREP_ERROR(__FUNCTION__, "esp_http_client_fetch_headers ERROR");
    }
    else if(ret == 0) {
    	LREP_ERROR(__FUNCTION__, "stream doesn't contain content-length header");
    }

	for(;;) {
		int data_read = esp_http_client_read(client, ota_write_data, BUFFSIZE);
		if (data_read < 0) {
			LREP_ERROR(__FUNCTION__, "Error: data read error");
			app_ota_http_cleanup(client);
			app_ota_task_fatal_err();
		}
		else if (data_read == 0) {
			if (esp_http_client_is_complete_data_received(client) == true) {
				LREP(__FUNCTION__, "Connection closed");
				break;
			}
			else {
				LREP_ERROR(__FUNCTION__, "Error: data read error zero, %d", errno);
				app_ota_http_cleanup(client);
				app_ota_task_fatal_err();
			}
		}
		else {
			if(!header_checked) {
				esp_app_desc_t new_app_info;
				if(data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
					memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
					LREP(__FUNCTION__, "New firmware version: %s", new_app_info.version);
					if (strcmp(PROJECT_NAME,new_app_info.project_name)!=0) {
						LREP_ERROR(__FUNCTION__, "Project Name Not Found, new app project name \"%s\"", new_app_info.project_name);
						app_ota_http_cleanup(client);
						app_ota_task_fatal_err();
					}
					esp_app_desc_t running_app_info;
					if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
						LREP(__FUNCTION__, "Running firmware version: %s", running_app_info.version);
					}

					char* pUpdateVer = (otaType  == OTA_TYPE_NORMAL) ? strUpdateVer : g_forceOta.strVersion;
					if (strlen(pUpdateVer) > 0 && memcmp(new_app_info.version, pUpdateVer, sizeof(new_app_info.version)) != 0) {
						LREP_WARNING(__FUNCTION__, "app update version is not valid \"%s\" \"%s\"", new_app_info.version, pUpdateVer);
						app_ota_http_cleanup(client);
						app_ota_task_fatal_err();
					}
					header_checked = true;
				} else {
					LREP_ERROR(__FUNCTION__, "received package is not fit len");
					app_ota_http_cleanup(client);
					app_ota_task_fatal_err();
				}

			}
			err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
			if (err != ESP_OK) {
				app_ota_http_cleanup(client);
				app_ota_task_fatal_err();
			}
			binary_file_length += data_read;
			LREP(__FUNCTION__, "readed %d", data_read);
		}
	}

	app_ota_http_cleanup(client);

	LREP(__FUNCTION__, "Total Write binary data length: %d", binary_file_length);
    if (esp_http_client_is_complete_data_received(client) != true) {
        LREP_ERROR(__FUNCTION__, "Error in receiving complete file");
        app_ota_task_fatal_err();
    }

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            LREP_ERROR(__FUNCTION__, "Image validation failed, image is corrupted");
        }
        LREP_ERROR(__FUNCTION__, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        app_ota_task_fatal_err();
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        LREP_ERROR(__FUNCTION__, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        app_ota_task_fatal_err();
    }
    LREP(__FUNCTION__, "Prepare to restart system!");

    app_mqtt_reconnect(60);
    if (otaType == OTA_TYPE_NORMAL) app_cmd_basic_response(CMD_START_OTA, ERROR_CODE_OK);
    else {
    	app_cmd_report_force_ota(g_forceOta.forceOtaSkd.iSkdId, ERROR_CODE_OK);
    	g_forceOta.bUpdated = true;
    	app_filesystem_write_force_ota_info(&g_forceOta);
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
    esp_restart();
    return ;
}

void app_ota_task_fatal_err() {
	LREP_ERROR(__FUNCTION__, "Exiting task due to fatal error...");
	g_otaRunning = false;
	if (ota_write_data != NULL) {
		free(ota_write_data);
		ota_write_data = NULL;
	}
	app_mqtt_reconnect(60);

	if (otaType == OTA_TYPE_NORMAL) app_cmd_basic_response(CMD_START_OTA, ERROR_CODE_UNDEFINED);
	else app_cmd_report_force_ota(g_forceOta.forceOtaSkd.iSkdId, ERROR_CODE_UNDEFINED);
	TASK_DELETE_SAFE(xOtaHandle);
}

void app_ota_http_cleanup(esp_http_client_handle_t client) {
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

esp_err_t app_ota_set_server_url(const char* severUrl) {
	if(strlen(severUrl) > (sizeof(strServerUrl) -1)) {
		LREP_WARNING(__FUNCTION__, "sever url oversized");
		return ESP_FAIL;
	}
	strncpy(strServerUrl, severUrl, sizeof(strServerUrl) -1);
	return ESP_OK;
}

esp_err_t app_ota_set_update_version(const char* version) {
	if(strlen(version) > (sizeof(strUpdateVer) -1)) {
		LREP_WARNING(__FUNCTION__, "update version oversized");
		return ESP_FAIL;
	}
	strncpy(strUpdateVer, version, sizeof(strUpdateVer) -1);
	return ESP_OK;
}

const char* app_ota_get_software_version() {
	if(strlen(strCurrentVer) == 0) {
		const esp_partition_t *running = esp_ota_get_running_partition();
		esp_app_desc_t running_app_info;
		if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
			LREP(__FUNCTION__, "Running firmware version: %s", running_app_info.version);
			strncpy(strCurrentVer, running_app_info.version, sizeof(strCurrentVer) -1);
		}
	}
	return strCurrentVer;
}

static struct tm tmForceOtaStart;
static struct tm tmForceOtaEnd;
static struct tm tmForceOtaDateStart;
static struct tm tmForceOtaDateEnd;
static bool bForceOtaPassMidNight;

void app_force_ota_init(void) {
	if (app_filesystem_read_force_ota_info(&g_forceOta) == ESP_OK) {
		LREP(__FUNCTION__, "Force ota info");
		LREP(__FUNCTION__, "Url:\"%s\"", g_forceOta.strUrl);
		LREP(__FUNCTION__, "version:\"%s\"", g_forceOta.strVersion);
		LREP(__FUNCTION__, "time start %lld", g_forceOta.forceOtaSkd.u64TimeStart);
		LREP(__FUNCTION__, "time end %lld", g_forceOta.forceOtaSkd.u64TimeEnd);
		LREP(__FUNCTION__, "date start %lld", g_forceOta.forceOtaSkd.u64DateStart);
		LREP(__FUNCTION__, "date end %lld", g_forceOta.forceOtaSkd.u64DateEnd);
		LREP(__FUNCTION__, "timezone  %.02f", g_forceOta.forceOtaSkd.fTimezone);
		LREP(__FUNCTION__, "skd id %d", g_forceOta.forceOtaSkd.iSkdId);
		LREP(__FUNCTION__, "Updated %s", g_forceOta.bUpdated ? "YES" : "NO");
		LREP(__FUNCTION__, "AUTO %s", g_forceOta.bAuto ? "YES" : "NO");
		bForceOtaRequire = true;
	} else bForceOtaRequire = false;

	if (bForceOtaRequire) {
		tmForceOtaStart = *app_time_server_get_timetm(g_forceOta.forceOtaSkd.u64TimeStart, g_forceOta.forceOtaSkd.fTimezone);
		tmForceOtaEnd = *app_time_server_get_timetm(g_forceOta.forceOtaSkd.u64TimeEnd, g_forceOta.forceOtaSkd.fTimezone);
		tmForceOtaDateStart = *app_time_server_get_timetm(g_forceOta.forceOtaSkd.u64DateStart, g_forceOta.forceOtaSkd.fTimezone);
		tmForceOtaDateEnd = *app_time_server_get_timetm(g_forceOta.forceOtaSkd.u64DateEnd, g_forceOta.forceOtaSkd.fTimezone);

		if (tmForceOtaStart.tm_hour < tmForceOtaEnd.tm_hour) bForceOtaPassMidNight = false;
		else if (tmForceOtaStart.tm_hour == tmForceOtaEnd.tm_hour && tmForceOtaStart.tm_min < tmForceOtaEnd.tm_min) bForceOtaPassMidNight = false;
		else bForceOtaPassMidNight = true;

		LREP(__FUNCTION__, "Time start %02d:%02d", tmForceOtaStart.tm_hour, tmForceOtaStart.tm_min);
		LREP(__FUNCTION__, "Time end %02d:%02d", tmForceOtaEnd.tm_hour, tmForceOtaEnd.tm_min);
		LREP(__FUNCTION__, "Date start %02d/%02d/%04d %02d:%02d:%02d", tmForceOtaDateStart.tm_mday, tmForceOtaDateStart.tm_mon+1, tmForceOtaDateStart.tm_year + 1900, tmForceOtaDateStart.tm_hour, tmForceOtaDateStart.tm_min, tmForceOtaDateStart.tm_sec);
		LREP(__FUNCTION__, "Date end %02d/%02d/%04d %02d:%02d:%02d", tmForceOtaDateEnd.tm_mday, tmForceOtaDateEnd.tm_mon+1, tmForceOtaDateEnd.tm_year + 1900, tmForceOtaDateEnd.tm_hour, tmForceOtaDateEnd.tm_min, tmForceOtaDateEnd.tm_sec);
	}
}

bool app_force_ota_condition_check(uint64_t timestamp) {
	if (!bForceOtaRequire || g_forceOta.bAuto ||g_forceOta.bUpdated ) return false;

	if (g_forceOta.forceOtaSkd.u64DateStart > timestamp || timestamp > g_forceOta.forceOtaSkd.u64DateEnd) {
		LREP_WARNING("Force OTA", "date condition not match");
		return false;
	}

	struct tm* pTm = app_time_server_get_timetm(timestamp, g_forceOta.forceOtaSkd.fTimezone);

	if (bForceOtaPassMidNight) {
		if (pTm->tm_hour > tmForceOtaStart.tm_hour || pTm->tm_hour < tmForceOtaEnd.tm_hour) return true;
		else if (pTm->tm_hour == tmForceOtaStart.tm_hour && pTm->tm_min >= tmForceOtaStart.tm_min) return true;
		else if (pTm->tm_hour == tmForceOtaEnd.tm_hour && pTm->tm_min <= tmForceOtaEnd.tm_min) return true;
		else return false;
	}
	else {
		if (pTm->tm_hour > tmForceOtaStart.tm_hour && pTm->tm_hour < tmForceOtaEnd.tm_hour) return true;
		else if (pTm->tm_hour == tmForceOtaStart.tm_hour && pTm->tm_hour < tmForceOtaEnd.tm_hour) {
			return (pTm->tm_min >= tmForceOtaStart.tm_min) ? true : false;
		}
		else if (pTm->tm_hour > tmForceOtaStart.tm_hour && pTm->tm_hour == tmForceOtaEnd.tm_hour) {
			return (pTm->tm_min <= tmForceOtaEnd.tm_min) ? true : false;
		}
		else if (pTm->tm_hour == tmForceOtaStart.tm_hour && pTm->tm_hour == tmForceOtaEnd.tm_hour) {
			return (pTm->tm_min >= tmForceOtaStart.tm_min && pTm->tm_min <= tmForceOtaEnd.tm_min) ? true : false;
		}
		else return false;
	}
}

void app_force_ota_process(void) {
	if(g_otaRunning) return;
	if (app_force_ota_condition_check(app_time_server_get_timestamp_now())) {
		app_force_ota_start();
	}
}

void app_force_ota_start(void) {
	LREP(__FUNCTION__, "Force ota start");
	if (strcmp(g_forceOta.strVersion, app_ota_get_software_version()) == 0) {
		app_cmd_report_force_ota(g_forceOta.forceOtaSkd.iSkdId, ERROR_CODE_OK);
		g_forceOta.bUpdated = true;
		app_filesystem_write_force_ota_info(&g_forceOta);
	}
	else {
		otaType = OTA_TYPE_FORCE;
		app_cmd_report_force_ota(g_forceOta.forceOtaSkd.iSkdId, ERROR_CODE_FORCE_OTA_START);
		app_mqtt_disconnect();
		xTaskCreate(&task_ota, "task_ota", 8192, NULL, 5, &xOtaHandle);
		g_otaRunning = true;
	}
}

