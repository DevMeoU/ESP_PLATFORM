/*
 * app_ble_mesh.h
 *
 *  Created on: Feb 22, 2021
 *      Author: laian
 */

#ifndef __APP_BLE_MESH_H
#define __APP_BLE_MESH_H

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_ble_mesh_defs.h"
#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_networking_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_config_model_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_ble_mesh_local_data_operation_api.h"
#include "esp_ble_mesh_lighting_model_api.h"

#include "app_ble.h"

typedef enum {
	APP_BLE_MESH_ELEMENT_ROOT,
	APP_BLE_MESH_ELEMENT_EXTEND_1,
	APP_BLE_MESH_ELEMENT_EXTEND_2,
	APP_BLE_MESH_ELEMENT_EXTEND_3
} app_ble_mesh_element_t;

void app_ble_mesh_init();
bool app_ble_mesh_provisioned();
bool app_ble_mesh_config_end();
void app_ble_mesh_prov_begin();

#endif /* __APP_BLE_MESH_H */
