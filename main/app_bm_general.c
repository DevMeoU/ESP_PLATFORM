/*
 * app_bm_general.c
 *
 *  Created on: Apr 5, 2021
 *      Author: laian
 */

#include "app_bm_general.h"
#include "config.h"
#include "app_gpio.h"

void BM_GEN_SRV(esp_ble_mesh_generic_server_cb_event_t event, esp_ble_mesh_generic_server_cb_param_t *param) {
	switch(event) {
	case ESP_BLE_MESH_GENERIC_SERVER_RECV_GET_MSG_EVT:
		LREP(__FUNCTION__, "Generic GET");
		switch (param->ctx.recv_op) {
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET: bm_onoff_get(param); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_MANUFACTURER_PROPERTY_GET: bm_man_prop_get(param); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_ADMIN_PROPERTY_GET: bm_admin_prop_get(param); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_GET: bm_user_prop_get(param); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_CLIENT_PROPERTIES_GET: bm_client_prop_get(param); break;
		default:
			LREP_WARNING(__FUNCTION__, "Opcode unhandled 0x%02X", param->ctx.recv_op);
			break;
		}
		break;
	case ESP_BLE_MESH_GENERIC_SERVER_RECV_SET_MSG_EVT:
		LREP(__FUNCTION__, "Generic Set");
		switch (param->ctx.recv_op) {
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET: bm_onoff_set(param, 1); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK: bm_onoff_set(param, 0); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_MANUFACTURER_PROPERTY_SET: bm_man_prop_set(param, 1); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_MANUFACTURER_PROPERTY_SET_UNACK: bm_man_prop_set(param, 0); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_ADMIN_PROPERTY_SET: bm_admin_prop_set(param, 1); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_ADMIN_PROPERTY_SET_UNACK: bm_admin_prop_set(param, 0); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET: bm_user_prop_set(param, 1); break;
		case ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_SET_UNACK: bm_user_prop_set(param, 0); break;
		default:
			LREP_WARNING(__FUNCTION__, "Opcode unhandled 0x%04X", param->ctx.recv_op);
			break;
		}
		break;
	default:
		LREP_WARNING(__FUNCTION__, "Event unhandled %d", event);
		break;
	}
}

void BM_GEN_CLI(esp_ble_mesh_generic_client_cb_event_t event, esp_ble_mesh_generic_client_cb_param_t *param) {
	LREP("GEN_CLIENT", "event %d, error code is %d, opcode is 0x%x",  event, param->error_code, param->params->opcode);
	switch(event) {
	default:
		LREP_WARNING(__FUNCTION__, "event unhandled %d", event);
		break;
	}
}

void bm_onoff_get(esp_ble_mesh_generic_server_cb_param_t *param) {
	esp_ble_mesh_gen_onoff_srv_t * srv = param->model->user_data;
	LREP(__FUNCTION__, "onoff 0x%02x", srv->state.onoff);
	esp_ble_mesh_server_model_send_msg(param->model, &param->ctx, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);
}

static uint8_t server_data_send[64];
static uint16_t server_data_send_length;

void bm_man_prop_get(esp_ble_mesh_generic_server_cb_param_t *param) {
	esp_ble_mesh_generic_server_recv_get_msg_t * get = &param->value.get;
	LREP(__FUNCTION__,"property id 0x%04X" ,get->manu_property.property_id);
	esp_ble_mesh_gen_manu_prop_srv_t * srv = param->model->user_data;
	esp_ble_mesh_generic_property_t* pProp = NULL;
#if 0
	if (srv == NULL) {
		LREP_ERROR(__FUNCTION__, "srv null");
		return;
	}
	if (srv->properties == NULL) {
		LREP_ERROR(__FUNCTION__, "properties null");
		return;
	}
	if (srv->properties->val == NULL) {
		LREP_ERROR(__FUNCTION__, "properties val null");
		return;
	}
#endif
	for (int i = 0; i < srv->property_count; ++i) {
		if (get->manu_property.property_id == srv->properties[i].id)
			pProp = srv->properties + i;
	}

	if (pProp == NULL) {
		LREP_WARNING(__FUNCTION__, "property id not found");
		return;
	}

	memset(server_data_send, 0, sizeof(server_data_send));
	server_data_send_length = 3 + pProp->val->len;
	if (sizeof(server_data_send) < server_data_send_length) {
		LREP_WARNING(__FUNCTION__, "property data length too big");
		return;
	}

	server_data_send[0] = pProp->id & 0xFF;
	server_data_send[1] = pProp->id >> 8;
	server_data_send[2] = pProp->user_access;
	for (int i = 0; i<pProp->val->len; ++i) server_data_send[i+3] = pProp->val->data[i];

	esp_ble_mesh_server_model_send_msg (
			param->model, &param->ctx,
			ESP_BLE_MESH_MODEL_OP_GEN_MANUFACTURER_PROPERTY_STATUS,
			server_data_send_length,
			server_data_send);
}

void bm_admin_prop_get(esp_ble_mesh_generic_server_cb_param_t *param) {
	esp_ble_mesh_generic_server_recv_get_msg_t * get = &param->value.get;
	LREP(__FUNCTION__, "property id 0x%04X", get->admin_property.property_id);
	esp_ble_mesh_gen_admin_prop_srv_t * srv = param->model->user_data;
	esp_ble_mesh_generic_property_t* pProp = NULL;
	for (int i = 0; i < srv->property_count; ++i) {
		if (get->admin_property.property_id == srv->properties[i].id)
			pProp = srv->properties + i;
	}

	if (pProp == NULL) {
		LREP_WARNING(__FUNCTION__, "property id not found");
		return;
	}

	memset(server_data_send, 0, sizeof(server_data_send));
	server_data_send_length = 3 + pProp->val->len;
	if (sizeof(server_data_send) < server_data_send_length) {
		LREP_WARNING(__FUNCTION__, "property data length too big");
		return;
	}

	server_data_send[0] = pProp->id & 0xFF;
	server_data_send[1] = pProp->id >> 8;
	server_data_send[2] = pProp->user_access;
	for (int i = 0; i<pProp->val->len; ++i) server_data_send[i+3] = pProp->val->data[i];

	esp_ble_mesh_server_model_send_msg (
			param->model, &param->ctx,
			ESP_BLE_MESH_MODEL_OP_GEN_ADMIN_PROPERTY_STATUS,
			server_data_send_length,
			server_data_send);
}

void bm_user_prop_get(esp_ble_mesh_generic_server_cb_param_t *param) {
	esp_ble_mesh_generic_server_recv_get_msg_t * get = &param->value.get;
	LREP(__FUNCTION__, "property id 0x%04X", get->user_property.property_id);
	esp_ble_mesh_gen_user_prop_srv_t * srv = param->model->user_data;
	esp_ble_mesh_generic_property_t* pProp = NULL;
	for (int i = 0; i < srv->property_count; ++i) {
		if (get->user_property.property_id == srv->properties[i].id)
			pProp = srv->properties + i;
	}

	if (pProp == NULL) {
		LREP_WARNING(__FUNCTION__, "property id not found");
		return;
	}

	memset(server_data_send, 0, sizeof(server_data_send));
	server_data_send_length = 3 + pProp->val->len;
	if (sizeof(server_data_send) < server_data_send_length) {
		LREP_WARNING(__FUNCTION__, "property data length too big");
		return;
	}

	server_data_send[0] = pProp->id & 0xFF;
	server_data_send[1] = pProp->id >> 8;
	server_data_send[2] = pProp->user_access;
	for (int i = 0; i<pProp->val->len; ++i) server_data_send[i+3] = pProp->val->data[i];

	esp_ble_mesh_server_model_send_msg (
			param->model, &param->ctx,
			ESP_BLE_MESH_MODEL_OP_GEN_USER_PROPERTY_STATUS,
			server_data_send_length,
			server_data_send);
}

void bm_client_prop_get(esp_ble_mesh_generic_server_cb_param_t *param) {
	esp_ble_mesh_generic_server_recv_get_msg_t * get = &param->value.get;
	LREP(__FUNCTION__, "property id 0x%04X", get->client_properties.property_id);

	server_data_send_length = 10;
	for (int i = 0; i < server_data_send_length; ++i) {
		server_data_send[i] = i;
	}

	esp_ble_mesh_server_model_send_msg (
			param->model, &param->ctx,
			ESP_BLE_MESH_MODEL_OP_GEN_CLIENT_PROPERTIES_STATUS,
			server_data_send_length,
			server_data_send);
}

void bm_onoff_set(esp_ble_mesh_generic_server_cb_param_t *param, int ack) {
	LREP(__FUNCTION__, "General OnOff Set: onoff 0x%02x, tid 0x%02x", param->value.set.onoff.onoff, param->value.set.onoff.tid);
	esp_ble_mesh_gen_onoff_srv_t * srv = param->model->user_data;
	srv->state.onoff = param->value.set.onoff.onoff;

	esp_ble_mesh_elem_t * p_Element = param->model->element;
#ifdef DEVICE_TYPE
	buzzer();
	if (p_Element == esp_ble_mesh_find_element(esp_ble_mesh_get_primary_element_address())) g_ChannelControl[1].iStatusReq = param->value.set.onoff.onoff;
	else if (p_Element == esp_ble_mesh_find_element(esp_ble_mesh_get_primary_element_address() + 1)) g_ChannelControl[2].iStatusReq = param->value.set.onoff.onoff;
	else if (p_Element == esp_ble_mesh_find_element(esp_ble_mesh_get_primary_element_address() + 2)) g_ChannelControl[3].iStatusReq = param->value.set.onoff.onoff;
	else LREP_ERROR(__FUNCTION__, "Element not found");
#endif
	if (ack)
		esp_ble_mesh_server_model_send_msg(param->model, &param->ctx, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff);

	if (param->model->pub != NULL && param->model->pub->publish_addr)
		esp_ble_mesh_model_publish(param->model, ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(srv->state.onoff), &srv->state.onoff, ROLE_NODE);
}

void bm_man_prop_set(esp_ble_mesh_generic_server_cb_param_t *param, int ack) {
	esp_ble_mesh_server_recv_gen_manufacturer_property_set_t* pProp = &param->value.set.manu_property;
	LREP(__FUNCTION__, "Property Id 0x%04X, User Access 0x%02X", pProp->property_id, pProp->user_access);
}

void bm_user_prop_set(esp_ble_mesh_generic_server_cb_param_t *param, int ack) {
	esp_ble_mesh_server_recv_gen_user_property_set_t* pProp = &param->value.set.user_property;
	LREP(__FUNCTION__, "Property Id 0x%04X", pProp->property_id);
	LREP(__FUNCTION__, "Len %d", pProp->property_value->len);
	for (int i = 0; i < pProp->property_value->len; ++i) {
		LREP_RAW("%02X ", pProp->property_value->data[i]);
	} LREP_RAW("\r\n");
}

void bm_admin_prop_set(esp_ble_mesh_generic_server_cb_param_t *param, int ack) {
	esp_ble_mesh_server_recv_gen_admin_property_set_t* pProp = &param->value.set.admin_property;
	LREP(__FUNCTION__, "Property Id 0x%04X, user access 0x%02X", pProp->property_id, pProp->user_access);
	LREP(__FUNCTION__, "Len %d", pProp->property_value->len);
	for (int i = 0; i < pProp->property_value->len; ++i) {
		LREP_RAW("%02X ", pProp->property_value->data[i]);
	} LREP_RAW("\r\n");
}

