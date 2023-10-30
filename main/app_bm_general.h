/*
 * app_bm_general.h
 *
 *  Created on: Apr 5, 2021
 *      Author: laian
 */

#ifndef __APP_BM_GENERAL_H
#define __APP_BM_GENERAL_H
#include "app_ble_mesh.h"

void BM_GEN_SRV(esp_ble_mesh_generic_server_cb_event_t event, esp_ble_mesh_generic_server_cb_param_t *param);
void BM_GEN_CLI(esp_ble_mesh_generic_client_cb_event_t event, esp_ble_mesh_generic_client_cb_param_t *param);

void bm_onoff_get(esp_ble_mesh_generic_server_cb_param_t *param);
void bm_man_prop_get(esp_ble_mesh_generic_server_cb_param_t *param);
void bm_admin_prop_get(esp_ble_mesh_generic_server_cb_param_t *param);
void bm_user_prop_get(esp_ble_mesh_generic_server_cb_param_t *param);
void bm_client_prop_get(esp_ble_mesh_generic_server_cb_param_t *param);

void bm_onoff_set(esp_ble_mesh_generic_server_cb_param_t *param, int ack);
void bm_man_prop_set(esp_ble_mesh_generic_server_cb_param_t *param, int ack);
void bm_user_prop_set(esp_ble_mesh_generic_server_cb_param_t *param, int ack);
void bm_admin_prop_set(esp_ble_mesh_generic_server_cb_param_t *param, int ack);

#endif /* __APP_BM_GENERAL_H */
