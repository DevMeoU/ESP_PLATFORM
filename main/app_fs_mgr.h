/*
 * app_fs_mgr.h
 *
 *  Created on: Aug 28, 2020
 *      Author: laian
 */

#ifndef __APP_FS_MGR_H
#define __APP_FS_MGR_H

#include "app_spiffs.h"
#include "app_mqtt.h"
#include "app_ota.h"
#include "app_gpio.h"

#define APP_DEVICE_BROKER_STORAGE_FILE 		"/spiffs/broker.txt"

#define APP_FILESYSTEM_LINE_FORMAT			"%s\r\n"

void app_filesystem_read_cloud_info(sCloudInfo_t* pCloudInfo);
void app_filesystem_write_cloud_info(const sCloudInfo_t* pCloudInfo);
void app_filesystem_delete_cloud_info(void);

#define APP_FORCE_OTA_FILE					"/spiffs/force_ota"

esp_err_t app_filesystem_read_force_ota_info(sForceOta_t* pForceOtaInfo);
void app_filesystem_write_force_ota_info(const sForceOta_t* pForceOtaInfo);

#define APP_GPIO_CONFIG_FILE				"/spiffs/app_gpio_config"

void app_filesystem_read_config(sSwRgbConfig_t* pConfig);
void app_filesystem_write_config(const sSwRgbConfig_t* pConfig);
void app_filesystem_delete_config(void);

int app_filesytem_read_status(int channel);
void app_filesytem_write_status(int channel, int status);

#endif /* __APP_FS_MGR_H */
