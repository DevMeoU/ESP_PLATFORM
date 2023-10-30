/*
 * app_cmd_mgr.h
 *
 *  Created on: Sep 21, 2020
 *      Author: laian
 */

#ifndef __APP_CMD_MGR_H
#define __APP_CMD_MGR_H
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include <string.h>
#include "mbedtls/sha256.h"

#include "app_common.h"
#include "config.h"

typedef enum __COMMAND_TYPE {
	CMD_TYPE_UDP = 0,
	CMD_TYPE_MQTT = 1,
	CMD_TYPE_MQTT_EPLINK = 2,
	CMD_TYPE_BLUFI = 3,
} cmd_type_t;

#define CMD_RESPONSE_MQTT				0x01
#define CMD_REPORT_MQTT					0x02
#define CMD_UDP_BROADCAST				0x04
#define CMD_UDP_UNICAST					0x08
#define CMD_RESPONSE_BLUFI				0x10

/*Command chung*/
#define CMD_GET_DEVICE_INFO   		"CmdGetDeviceInfo"
#define CMD_GET_DEVICE_ID        	"CmdGetDeviceID"
#define CMD_GET_DEVICE_STATUS       "CmdGetStatus"
#define CMD_RESET					"CmdReset"

/*Command them, xoa thiet bi*/
#define CMD_ADD_DEVICE        		"CmdAddDevice"
#define CMD_DELETE_DEVICE     		"CmdDeleteDevice"
#define CMD_SET_DEVICE_CONFIG		"CmdSetDeviceConfig"
#define CMD_GET_DEVICE_CONFIG   	"CmdGetDeviceConfig"
#define CMD_SET_WIFI_INFO			"CmdSetWifiInfo"
#define CMD_EXIT_CONFIGURATION		"CmdExitConfiguration"

/*Command dieu khien, cai dat, lay trang thai thiet bi*/
#define CMD_SET_EXTRA_CONFIG		"CmdSetExtraConfig"
#define CMD_GET_EXTRA_CONFIG		"CmdGetExtraConfig"
#define CMD_GET_DEVICE        		"CmdGetData"
#define CMD_SET_DEVICE        		"CmdSetData"

/*Commmand them, sua, xoa, kiem tra lich*/
#define CMD_ADD_SCHEDULE	  		"CmdAddSchedule"
#define CMD_DELETE_SCHEDULE	  		"CmdDeleteSchedule"
#define CMD_UPDATE_SCHEDULE	  		"CmdUpdateSchedule"
#define CMD_LIST_SCHEDULE			"CmdScheduleList"
#define CMD_GET_SCHEDULE			"CmdGetSchedule"

/*Command OTA*/
#define CMD_START_OTA         		"CmdStartOta"
#define CMD_SET_FORCE_OTA			"CmdSetForceOta"

/*Command Endpoint*/
#define CMD_SET_END_POINT_CONFIG 	"CmdSetEndpointConfig"
#define CMD_GET_END_POINT_CONFIG 	"CmdGetEndpointConfig"
#define CMD_DELETE_END_POINT_CONFIG "CmdDeleteEndpointConfig"
#define CMD_END_POINT_LINK_SET_DATA  "CmdEndpointSetData"

/*report*/
#define REPORT_FORCE_OTA			"ReportForceOTA"

#define SWITCH_1					"switch_1"
#define SWITCH_2					"switch_2"
#define SWITCH_3					"switch_3"
#define SWITCH_4					"switch_4"

#ifdef DEVICE_TYPE
#define CURTAIN_OPEN				"curtain_open"
#define CURTAIN_CLOSE				"curtain_close"
#define CURTAIN_STOP				"curtain_stop"
#define OPEN_LEVEL					"open_level"
#endif

#define BE_SHARED_KEY 	"29a49d844c009abb32bcb5094b327382fe260a2dgth68mbncbjkebfjk9849328jnfkj"

esp_err_t   app_cmd_handle_buffer(const char* msg, int msgLen, cmd_type_t msgType);
esp_err_t	app_cmd_handle(const char* msg, cmd_type_t msgType);

esp_err_t   app_cmd_handle_GetDeviceInfo(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_GetDeviceID(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_GetStatus(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t 	app_cmd_handle_Reset(const cJSON* pJsValue, cmd_type_t msgType);

esp_err_t   app_cmd_handle_DeleteDevice(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_SetDeviceConfig(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_GetDeviceConfig(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_SetWifiInfo(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t 	app_cmd_handle_ExitConfiguration(const cJSON* pJsValue, cmd_type_t msgType);

esp_err_t 	app_cmd_handle_SetExtraConfig(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t 	app_cmd_handle_GetExtraConfig(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_GetData(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_SetData(const cJSON* pJsValue, cmd_type_t msgType);

esp_err_t   app_cmd_handle_AddSchedule(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_DeleteSchedule(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_UpdateSchedule(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_ScheduleList(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_GetSchedule(const cJSON* pJsValue, cmd_type_t msgType);

esp_err_t   app_cmd_handle_StartOta(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_SetForceOta(const cJSON* pJsValue, cmd_type_t msgType);

esp_err_t   app_cmd_handle_SetEndpointConfig(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_GetEndpointConfig(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_DeleteEndpointConfig(const cJSON* pJsValue, cmd_type_t msgType);
esp_err_t   app_cmd_handle_EndpointSetData(const cJSON* pJsValue, cmd_type_t msgType);

esp_err_t 	app_cmd_value_check_devT(const cJSON* pJsValue);
esp_err_t 	app_cmd_value_check_devExtAddr(const cJSON* pJsValue);

const char*	app_cmd_get_deviceId(void);

void 		app_cmd_response_GetData(void);
void 		app_cmd_GetData(void);
void 		app_cmd_GetDataResend(void);

void		app_cmd_SetData(const cJSON *pJsDevV);

void        app_cmd_report_force_ota(int skdId, int err);

void		app_cmd_schedule_reponse(const char* cmd, int id, int err);
void 		app_cmd_basic_response(const char* cmd, int err);

void 		app_cmd_send_EndpointSetData(int endpoint, const char* origId);

void		app_cmd_send(const char* data_string, uint8_t response);

void 		app_cmd_send_check_force_ota(void);

void		app_cmd_sha256(const char* pStrInput, char* pStrOutput);

void 		app_cmd_led_SetExtraConfig(const cJSON* pJsValue);
void		app_cmd_channel_SetExtraConfig(const cJSON* pJsValue, int channel);
void 		app_cmd_curtain_SetExtraConfig(const cJSON* pJsValue);
void 		app_cmd_night_SetExtraConfig(const cJSON* pJsValue);

void 		app_cmd_build_response_ExtraConfig(cJSON *root);

#define json_check_object(__JSON) 		(__JSON != NULL && cJSON_IsObject(__JSON))
#define json_check_array(__JSON) 		(__JSON != NULL && cJSON_IsArray(__JSON))
#define json_check_string(__JSON)		(__JSON != NULL && cJSON_IsString(__JSON))
#define json_check_number(__JSON)		(__JSON != NULL && cJSON_IsNumber(__JSON))

#define json_cmp_str(__JSON_STR, __STR) strcmp(__JSON_STR->valuestring, __STR) == 0

#define PARAM_INT_VALUE_ADD(__JS_ARRAY, __PARAM, __VALUE) \
	do{\
		cJSON* pX = cJSON_CreateObject();\
		cJSON_AddStringToObject(pX, "param", __PARAM);\
		cJSON_AddNumberToObject(pX, "value", __VALUE);\
		cJSON_AddItemToArray(__JS_ARRAY, pX);\
	}while(0)

#endif /* __APP_CMD_MGR_H */
