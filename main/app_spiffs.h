/*
 * app_spiffs.h
 *
 *  Created on: Mar 4, 2020
 *      Author: laian
 */

#ifndef __APP_SPIFFS_H
#define __APP_SPIFFS_H

#include <string.h>
#include <stdint.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "config.h"

void app_spiffs_init();

esp_err_t app_spiffs_read_file(const char* fileName, char* buffer, int maximumLen);
esp_err_t app_spiffs_write_file(const char* fileName, const char* buffer);

esp_err_t app_spiffs_delete_file(const char* fileName);

bool app_spiffs_check_file_exist(const char* fileName);

#endif /* __APP_SPIFFS_H */
