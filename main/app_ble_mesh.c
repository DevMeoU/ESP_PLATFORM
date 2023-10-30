/*
 * app_ble_mesh.c
 *
 *  Created on: Feb 22, 2021
 *      Author: laian
 */
#include "esp_wifi.h"

#include "app_ble_mesh.h"
#include "app_bm_general.h"
#include "app_gpio.h"
#include "config.h"

#define CID_ESP 0x02E5

static void BM_PROV(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param);
static void BM_CONF_SRV(esp_ble_mesh_cfg_server_cb_event_t event, esp_ble_mesh_cfg_server_cb_param_t *param);
static void BM_LIGHT_SRV(esp_ble_mesh_lighting_server_cb_event_t event, esp_ble_mesh_lighting_server_cb_param_t *param);

static uint8_t dev_uuid[16] = {0xdd, 0xdd};

static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_ENABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
#if defined(CONFIG_BLE_MESH_FRIEND)
    .friend_state = ESP_BLE_MESH_FRIEND_ENABLED,
#else
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
#endif
#if defined(CONFIG_BLE_MESH_GATT_PROXY_SERVER)
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
#else
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_NOT_SUPPORTED,
#endif
    .default_ttl = 7,
    /* 3 transmissions with 20ms interval */
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
};

ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_goos_1, 2 + 3, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_goos_2, 2 + 3, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_goos_3, 2 + 3, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_goos_4, 2 + 3, ROLE_NODE);

ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_gooc_1, 2 + 1, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_gooc_2, 2 + 1, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_gooc_3, 2 + 1, ROLE_NODE);
ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_gooc_4, 2 + 1, ROLE_NODE);

static esp_ble_mesh_gen_onoff_srv_t goos_model_data_1;
static esp_ble_mesh_gen_onoff_srv_t goos_model_data_2;
static esp_ble_mesh_gen_onoff_srv_t goos_model_data_3;
static esp_ble_mesh_gen_onoff_srv_t goos_model_data_4;

static esp_ble_mesh_client_t gooc_data_1;
static esp_ble_mesh_client_t gooc_data_2;
static esp_ble_mesh_client_t gooc_data_3;
static esp_ble_mesh_client_t gooc_data_4;

//ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_lhsl_1, 20, ROLE_NODE);
//static esp_ble_mesh_light_hsl_state_t lhlsl_state_1;
//static esp_ble_mesh_light_hsl_srv_t lhsl_data_1 = { .state = &lhlsl_state_1, };

ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_gmps_1, 40, ROLE_NODE);
uint8_t gmps_data_buff_1[2] = {0x07, 0x25};
uint8_t gmps_data_buff_2[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t gmps_data_buff_3[3] = {0xAA, 0xBB, 0xCC};
uint8_t gmps_data_buff_4[20] = {0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x3B, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B};
struct net_buf_simple gmps_val_1 = {.data = gmps_data_buff_1, .size = sizeof(gmps_data_buff_1), .len = sizeof(gmps_data_buff_1)};
struct net_buf_simple gmps_val_2 = {.data = gmps_data_buff_2, .size = sizeof(gmps_data_buff_2), .len = sizeof(gmps_data_buff_2)};
struct net_buf_simple gmps_val_3 = {.data = gmps_data_buff_3, .size = sizeof(gmps_data_buff_3), .len = sizeof(gmps_data_buff_3)};
struct net_buf_simple gmps_val_4 = {.data = gmps_data_buff_4, .size = sizeof(gmps_data_buff_4), .len = sizeof(gmps_data_buff_4)};
static esp_ble_mesh_generic_property_t gmps_prop_table[] = {
	{.id = 1, .val = &gmps_val_1},
	{.id = 2, .val = &gmps_val_2},
	{.id = 3, .val = &gmps_val_3},
	{.id = 4, .val = &gmps_val_4}
};
static esp_ble_mesh_gen_manu_prop_srv_t gmps_data_1 = {.properties = gmps_prop_table, .property_count = 4};

ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_gaps_1, 40, ROLE_NODE);
uint8_t gaps_data_buff_1[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t gaps_data_buff_2[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
uint8_t gaps_data_buff_3[15] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x20};
uint8_t gaps_data_buff_4[20] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25};
struct net_buf_simple gaps_val_1 = {.data = gaps_data_buff_1, .size = sizeof(gaps_data_buff_1), .len = sizeof(gaps_data_buff_1)};
struct net_buf_simple gaps_val_2 = {.data = gaps_data_buff_2, .size = sizeof(gaps_data_buff_2), .len = sizeof(gaps_data_buff_2)};
struct net_buf_simple gaps_val_3 = {.data = gaps_data_buff_3, .size = sizeof(gaps_data_buff_3), .len = sizeof(gaps_data_buff_3)};
struct net_buf_simple gaps_val_4 = {.data = gaps_data_buff_4, .size = sizeof(gaps_data_buff_4), .len = sizeof(gaps_data_buff_4)};
static esp_ble_mesh_generic_property_t gaps_prop_table[] = {
	{.id = 1, .val = &gaps_val_1},
	{.id = 2, .val = &gaps_val_2},
	{.id = 3, .val = &gaps_val_3},
	{.id = 4, .val = &gaps_val_4}
};
static esp_ble_mesh_gen_admin_prop_srv_t gaps_data_1 = {.properties = gaps_prop_table, .property_count = 4};

ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_gups_1, 40, ROLE_NODE);
uint8_t gups_data_buff_1[5] = {0x01, 0x02, 0x03, 0x04, 0x05};
uint8_t gups_data_buff_2[10] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
uint8_t gups_data_buff_3[15] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x20};
uint8_t gups_data_buff_4[20] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25};
struct net_buf_simple gups_val_1 = {.data = gups_data_buff_1, .size = sizeof(gups_data_buff_1), .len = sizeof(gups_data_buff_1)};
struct net_buf_simple gups_val_2 = {.data = gups_data_buff_2, .size = sizeof(gups_data_buff_2), .len = sizeof(gups_data_buff_2)};
struct net_buf_simple gups_val_3 = {.data = gups_data_buff_3, .size = sizeof(gups_data_buff_3), .len = sizeof(gups_data_buff_3)};
struct net_buf_simple gups_val_4 = {.data = gups_data_buff_4, .size = sizeof(gups_data_buff_4), .len = sizeof(gups_data_buff_4)};
esp_ble_mesh_generic_property_t gups_prop_table[] = {
	{.id = 1, .val = &gups_val_1},
	{.id = 2, .val = &gups_val_2},
	{.id = 3, .val = &gups_val_3},
	{.id = 4, .val = &gups_val_4}
};
static esp_ble_mesh_gen_user_prop_srv_t gups_data_1 = {.properties = gups_prop_table, .property_count = 4};

ESP_BLE_MESH_MODEL_PUB_DEFINE(pub_gcps_1, 20, ROLE_NODE);
static uint16_t gcps_prop[20] = {0x2507};
static esp_ble_mesh_gen_client_prop_srv_t gcps_data_1 = {.id_count = 20, .property_ids = gcps_prop};

static esp_ble_mesh_model_t root_model[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
	ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&pub_goos_1, &goos_model_data_1),
//	ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&pub_gooc_1, &gooc_data_1),
//	ESP_BLE_MESH_MODEL_LIGHT_HSL_SRV(&pub_lhsl_1, &lhsl_data_1),
//	ESP_BLE_MESH_MODEL_GEN_USER_PROP_SRV(&pub_gups_1, &gups_data_1),
//	ESP_BLE_MESH_MODEL_GEN_ADMIN_PROP_SRV(&pub_gaps_1, &gaps_data_1),
//	ESP_BLE_MESH_MODEL_GEN_MANUFACTURER_PROP_SRV(&pub_gmps_1, &gmps_data_1),
//	ESP_BLE_MESH_MODEL_GEN_CLIENT_PROP_SRV(&pub_gcps_1, &gcps_data_1),
};

static esp_ble_mesh_model_t extend_model_1[] = {
	ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&pub_goos_2, &goos_model_data_2),
//	ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&pub_gooc_2, &gooc_data_2),
};

static esp_ble_mesh_model_t extend_model_2[] = {
	ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&pub_goos_3, &goos_model_data_3),
//	ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&pub_gooc_3, &gooc_data_3),
};

static esp_ble_mesh_model_t extend_model_3[] = {
	ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&pub_goos_4, &goos_model_data_4),
//	ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&pub_gooc_4, &gooc_data_4),
};

static esp_ble_mesh_elem_t mesh_elements[] = {
#ifdef DEVICE_TYPE
	ESP_BLE_MESH_ELEMENT(0, root_model, ESP_BLE_MESH_MODEL_NONE),
	ESP_BLE_MESH_ELEMENT(0, extend_model_1, ESP_BLE_MESH_MODEL_NONE),
	ESP_BLE_MESH_ELEMENT(0, extend_model_2, ESP_BLE_MESH_MODEL_NONE),
//	ESP_BLE_MESH_ELEMENT(0, extend_model_3, ESP_BLE_MESH_MODEL_NONE),
#endif
};

static esp_ble_mesh_comp_t mesh_comp = {
    .cid = CID_ESP,
    .elements = mesh_elements,
    .element_count = ARRAY_SIZE(mesh_elements),
};

static esp_ble_mesh_prov_t mesh_prov = {
    .uuid = dev_uuid,
    .output_size = 0,
    .output_actions = 0,
};

bool bNodeProvisioning = false;

void app_ble_mesh_init() {
	esp_err_t ret;

	app_bluedroid_init();

	const uint8_t* pBtAddr = esp_bt_dev_get_address();
	if (pBtAddr == NULL) {LREP_ERROR(__FUNCTION__, "bluetooth stack is not enabled"); exit(-1);}

	LREP_WARNING(__FUNCTION__, "bluetooth device address %02X:%02X:%02X:%02X:%02X:%02X", *pBtAddr, *(pBtAddr+1), *(pBtAddr+2), *(pBtAddr+3), *(pBtAddr+4), *(pBtAddr+5));
	memcpy(dev_uuid + 2, pBtAddr, BD_ADDR_LEN);

	esp_ble_mesh_register_prov_callback(BM_PROV);
	esp_ble_mesh_register_config_server_callback(BM_CONF_SRV);
	esp_ble_mesh_register_generic_server_callback(BM_GEN_SRV);
	esp_ble_mesh_register_generic_client_callback(BM_GEN_CLI);

	ret = esp_ble_mesh_init(&mesh_prov, &mesh_comp);
	APP_ESP_ERROR_RETURN(ret, __FUNCTION__, "Initializing mesh failed (err %d)", ret);

	uint8_t eth_mac[6];
	esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
	char device_name[20] = {0};
	snprintf(device_name, sizeof(device_name),"VCONNEX_MESH_%02X%02X", eth_mac[4], eth_mac[5]);
	esp_ble_mesh_set_unprovisioned_device_name(device_name);
}

static void BM_PROV(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param) {
	LREP("PROV_CB","Event %d", event);
	switch (event) {
	case ESP_BLE_MESH_PROV_REGISTER_COMP_EVT: /* 0 !< Initialize BLE Mesh provisioning capabilities and internal data information completion event */
		break;
	case ESP_BLE_MESH_NODE_SET_UNPROV_DEV_NAME_COMP_EVT: /* 1 !< Set the unprovisioned device name completion event */
		break;
	case ESP_BLE_MESH_NODE_PROV_ENABLE_COMP_EVT: /* 2 !< Enable node provisioning functionality completion event */
		break;
	case ESP_BLE_MESH_NODE_PROV_DISABLE_COMP_EVT: /* 3 !< Disable node provisioning functionality completion event */
		break;
	case ESP_BLE_MESH_NODE_PROV_LINK_OPEN_EVT: /* 4 !< Establish a BLE Mesh link event */
		break;
	case ESP_BLE_MESH_NODE_PROV_LINK_CLOSE_EVT: /* 5 !< Close a BLE Mesh link event */
		break;
	case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:
		LREP(__FUNCTION__, "Provisioning done");
		LREP(__FUNCTION__, "net_idx 0x%04X , primary addr 0x%04X, flags 0x%02X, iv_index %d",
			param->node_prov_complete.net_idx, param->node_prov_complete.addr, param->node_prov_complete.flags, param->node_prov_complete.iv_index);
		uint8_t* pKey = param->node_prov_complete.net_key;
		LREP(__FUNCTION__, "net_key: 0x %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X",
			pKey[0], pKey[1], pKey[2], pKey[3],	pKey[4], pKey[5], pKey[6], pKey[7], pKey[8], pKey[9], pKey[10], pKey[11], pKey[12], pKey[13], pKey[14], pKey[15]);
		gpio_set_led_normal();
		break;
	case ESP_BLE_MESH_NODE_PROV_RESET_EVT:
		LREP(__FUNCTION__, "Provisioning reset");
		esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT);
		bNodeProvisioning = true;
		break;
	default:
		LREP_WARNING(__FUNCTION__, "Event unhandled %d", event);
		break;
	}
}

static void BM_CONF_SRV(esp_ble_mesh_cfg_server_cb_event_t event, esp_ble_mesh_cfg_server_cb_param_t *param) {
	if (event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
		LREP(__FUNCTION__, "Model: model_id 0x%02X ", param->model->model_id);

		LREP(__FUNCTION__, "Context: recv_op 0x%02X, remote addr 0x%04X, recv_dst 0x%04X",
				param->ctx.recv_op, param->ctx.addr, param->ctx.recv_dst);

		switch (param->ctx.recv_op) {
		case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
			LREP(__FUNCTION__, "App Key Add, net_idx 0x%04X, app_idx 0x%04X",
				param->value.state_change.appkey_add.net_idx,
				param->value.state_change.appkey_add.app_idx);
			uint8_t* pKey = param->value.state_change.appkey_add.app_key;
			LREP(__FUNCTION__, "0x %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X",
				pKey[0], pKey[1], pKey[2], pKey[3], pKey[4], pKey[5], pKey[6], pKey[7], pKey[8], pKey[9], pKey[10], pKey[11], pKey[12], pKey[13], pKey[14], pKey[15]);
			break;
		case ESP_BLE_MESH_MODEL_OP_APP_KEY_DELETE:
			LREP(__FUNCTION__, "App Key Delete, net_idx 0x%04X, app_idx 0x%04X",
				param->value.state_change.appkey_delete.net_idx, param->value.state_change.appkey_delete.app_idx);
			break;
		case ESP_BLE_MESH_MODEL_OP_APP_KEY_UPDATE:
			LREP(__FUNCTION__, "App Key Update, net_idx 0x%04X, app_idx 0x%04X",
				param->value.state_change.appkey_update.net_idx, param->value.state_change.appkey_update.app_idx);
			uint8_t* pKeyUpdate = param->value.state_change.appkey_update.app_key;
			LREP(__FUNCTION__, "0x %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X",
				pKeyUpdate[0], pKeyUpdate[1], pKeyUpdate[2], pKeyUpdate[3], pKeyUpdate[4], pKeyUpdate[5], pKeyUpdate[6], pKeyUpdate[7],
				pKeyUpdate[8], pKeyUpdate[9], pKeyUpdate[10], pKeyUpdate[11], pKeyUpdate[12], pKeyUpdate[13], pKeyUpdate[14], pKeyUpdate[15]);
			break;
		case ESP_BLE_MESH_MODEL_OP_MODEL_PUB_SET:
			LREP(__FUNCTION__, "Publication Set");
			LREP(__FUNCTION__, "Element Addr 0x%04X, Pub_Addr 0x%04X, App Key Idx %d",
					param->value.state_change.mod_pub_set.element_addr,
					param->value.state_change.mod_pub_set.pub_addr,
					param->value.state_change.mod_pub_set.app_idx);
			LREP(__FUNCTION__, "cred_flag %s, ttl %d, period %d, retransmit %d, company id 0x%04X, model id 0x%04X",
					param->value.state_change.mod_pub_set.cred_flag ? "YES" : "NO",
					param->value.state_change.mod_pub_set.pub_ttl,
					param->value.state_change.mod_pub_set.pub_period,
					param->value.state_change.mod_pub_set.pub_retransmit,
					param->value.state_change.mod_pub_set.company_id,
					param->value.state_change.mod_pub_set.model_id);
			break;
		case ESP_BLE_MESH_MODEL_OP_MODEL_APP_BIND:
			LREP(__FUNCTION__, "Bind App key");
			LREP(__FUNCTION__, "elem_addr 0x%04x, app_idx 0x%04x, cid 0x%04x, mod_id 0x%04x",
				param->value.state_change.mod_app_bind.element_addr,
				param->value.state_change.mod_app_bind.app_idx,
				param->value.state_change.mod_app_bind.company_id,
				param->value.state_change.mod_app_bind.model_id);
			break;
		case ESP_BLE_MESH_MODEL_OP_MODEL_SUB_ADD:
			LREP(__FUNCTION__, "Subscriber Add");
			LREP(__FUNCTION__, "Element Addr 0x%04X, Sub_addr 0x%04X, Model 0x%04X, company 0x%04X",
					param->value.state_change.mod_sub_add.element_addr,
					param->value.state_change.mod_sub_add.sub_addr,
					param->value.state_change.mod_sub_add.model_id,
					param->value.state_change.mod_sub_add.company_id);
			break;
		default:
			LREP_WARNING(__FUNCTION__,"Opcode unhandled 0x%02X", param->ctx.recv_op);
			break;
		}
	}
	else {
		LREP_WARNING(__FUNCTION__, "Event unhandled %d", event);
	}
}

bool app_ble_mesh_provisioned() {
	return esp_ble_mesh_node_is_provisioned();
}

bool app_ble_mesh_config_end() {
	if ( bNodeProvisioning && esp_ble_mesh_node_prov_disable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT) == ESP_OK )  {
		bNodeProvisioning = false;
		return true;
	}
	return false;
}

void app_ble_mesh_prov_begin() {
	if (esp_ble_mesh_node_is_provisioned()) esp_ble_mesh_node_local_reset();
	else esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT), bNodeProvisioning = true;
}

