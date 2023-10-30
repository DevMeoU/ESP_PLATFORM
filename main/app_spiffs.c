/*
 * app_spiffs.c
 *
 *  Created on: Mar 5, 2020
 *      Author: laian
 */

#include "app_spiffs.h"

static esp_vfs_spiffs_conf_t conf = {
  .base_path = "/spiffs",
  .partition_label = "storage",
  .max_files = 10,
  .format_if_mount_failed = true
};

void app_spiffs_init() {
	LREP(__FUNCTION__, "Initializing SPIFFS");
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            LREP_ERROR(__FUNCTION__, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            LREP_ERROR(__FUNCTION__, "Failed to find SPIFFS partition");
        } else {
            LREP_ERROR(__FUNCTION__, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }
    else {
    	LREP(__FUNCTION__,"Initialize SPIFFS OK");
    }
    ESP_ERROR_CHECK(ret);
    if(esp_spiffs_mounted("storage") == ESP_OK) {
    	LREP(__FUNCTION__,"So far so good");
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        LREP_ERROR(__FUNCTION__, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
    	LREP(__FUNCTION__, "Partition size: total: %d, used: %d", total, used);
    }
}

esp_err_t app_spiffs_read_file(const char* fileName, char* buffer, int maximumLen) {
	FILE* fptr = fopen(fileName, "r");
	if (fptr == NULL) {
		LREP_ERROR(__FUNCTION__, "Failed to open file for reading");
		return ESP_FAIL;
	}
	int index = 0;
	int c = fgetc(fptr);
	while(c != EOF) {
		buffer[index] = (char)c;
		c = fgetc(fptr);
		if(index++ >= maximumLen) break;
	}
	fclose(fptr);
	return ESP_OK;
}

esp_err_t app_spiffs_write_file(const char* fileName, const char* buffer) {
	FILE * fPtr = fopen(fileName, "w");
	if (fPtr == NULL) {
		LREP_ERROR(__FUNCTION__, "Failed to open file for reading");
		return ESP_FAIL;
	}

	fputs(buffer,fPtr);
	fclose(fPtr);
	return ESP_OK;
}

esp_err_t app_spiffs_delete_file(const char* fileName) {
	if(remove(fileName) == 0)
		return ESP_OK;
	return ESP_FAIL;
}

bool app_spiffs_check_file_exist(const char* fileName) {
	struct stat st;
    if (stat(fileName, &st) == 0) {
    	return true;
    }
	return false;
}



