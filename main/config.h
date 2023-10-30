/*
 * config.h
 *
 *  Created on: Dec 13, 2019
 *      Author: Admin
 */

#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "time.h"


#define	ON	1
#define	OFF	0

/**/
#define DEVICE_TYPE       			3017
#define PROJECT_NAME      			"SSW_RGB3"

/*gpio config*/
#define BUZZER_PIN					GPIO_NUM_12

#define RELAY_1_PIN		            GPIO_NUM_26
#define RELAY_2_PIN		            GPIO_NUM_25
#define RELAY_3_PIN		            GPIO_NUM_16
#define RELAY_4_PIN		            GPIO_NUM_4

#define MODE_PIN					GPIO_NUM_15

#define	GPIO_PIN_SEL				((1ULL << BUZZER_PIN) | (1ULL << RELAY_1_PIN) | (1ULL << RELAY_2_PIN) | (1ULL << RELAY_3_PIN)| (1ULL << RELAY_4_PIN) )

#define	RELAY_ON					1
#define	RELAY_OFF					0

#define	BUZZER_ON					gpio_set_level(BUZZER_PIN, 1)
#define	BUZZER_OFF					gpio_set_level(BUZZER_PIN, 0)


#define	RELAY_1_ON					gpio_set_level(RELAY_1_PIN, RELAY_ON)
#define	RELAY_1_OFF					gpio_set_level(RELAY_1_PIN, RELAY_OFF)

#define	RELAY_2_ON					gpio_set_level(RELAY_2_PIN, RELAY_ON)
#define	RELAY_2_OFF					gpio_set_level(RELAY_2_PIN, RELAY_OFF)

#define	RELAY_3_ON					gpio_set_level(RELAY_3_PIN, RELAY_ON)
#define	RELAY_3_OFF					gpio_set_level(RELAY_3_PIN, RELAY_OFF)

#define	RELAY_4_ON					gpio_set_level(RELAY_4_PIN, RELAY_ON)
#define	RELAY_4_OFF					gpio_set_level(RELAY_4_PIN, RELAY_OFF)


#define BATTERYPERCENT            	100

/*ma loi*/
#define ERROR_CODE_OK					50000

#define ERROR_CODE_UNDEFINED			50004
#define ERROR_CODE_INVALID_PARAM		50005
#define ERROR_CODE_SCHEDULE_ID_EXISTED 	50006
#define ERROR_CODE_SCHEDULE_FULL		50007
#define ERROR_CODE_SCHEDULE_NOT_INIT	50008
#define ERROR_CODE_JSON_FORMAT			50009
#define ERROR_CODE_SCHEDULE_NOT_EXISTED	50010

#define ERROR_CODE_FORCE_OTA_START		50011

#define ERROR_CODE_SET_CONFIG_IN_PROCESS	50100
#define ERROR_CODE_SET_CONFIG_THRESHOLD		50101

#define DEBUG

#ifdef DEBUG
#define LREP(tag, format, ...) 				ESP_LOGI(tag, format, ##__VA_ARGS__)
#define LREP_WARNING(tag, format, ...)		ESP_LOGW(tag, format, ##__VA_ARGS__)
#define LREP_ERROR(tag, format, ...)		ESP_LOGE(tag, format, ##__VA_ARGS__)
#define LREP_RAW(format, ...)				printf(format, ##__VA_ARGS__)
#else
#define LREP(tag, format, ...)				{}
#define LREP_WARNING(tag, format, ...)		{}
#define LREP_ERROR(tag, format, ...)		{}
#define LREP_RAW(format, ...)				{}
#endif

extern bool g_bRequestGetData;
extern bool g_bRequestWifiConfig;
extern bool g_bRequestOta;
extern bool g_bRequestForceOtaAuto;
extern uint64_t g_MainTime;

extern bool g_bRequestSmartconfig;
extern bool g_bRequestSoftap;
extern bool g_bRequestBluFi;
extern bool g_bRequestExitConfig;
extern bool g_bRequestBleMeshProv;

extern bool g_bRun_MainTask;
extern bool g_bRun_ButtonTask;
extern bool g_bRun_RelayTask;
extern bool g_bRun_BuzzerTask;
extern bool g_bRun_GpioTask;
extern bool g_bRun_MqttTask;
extern bool g_bRun_UdpTask;
extern bool g_bRun_SkdTask;

#define ENCRYPTED		0
#define ENCRYPTED_INPUT_HEX_DIGIT		1

#define MAX_MESSAGE_LENGTH	2048

//#define BLE_MESH_ENABLE

//#define WIFI_FIXED
#ifdef WIFI_FIXED
#define WIFI_FIXED_SSID			"VCONNEX"
#define WIFI_FIXED_PASS			"0913456789"
#endif

#define TASK_DELETE_SAFE(__HANDLE)  if(__HANDLE) {TaskHandle_t __HANDLE_X = __HANDLE; __HANDLE = NULL; vTaskDelete(__HANDLE_X); }

#endif /* MAIN_CONFIG_H_ */
