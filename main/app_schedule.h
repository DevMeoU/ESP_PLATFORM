/*
 * app_schedule.h
 *
 *  Created on: Mar 4, 2020
 *      Author: laian
 */

#ifndef __APP_SCHEDULE_H
#define __APP_SCHEDULE_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "cJSON.h"

#include "app_wifi_mode.h"
#include "config.h"
#include "app_spiffs.h"
#include "app_gpio.h"
#include "app_endpoint_link.h"
#define APP_SKD_MAXIMUM		10
#define APP_SKD_BUFFER_SIZE	2048
#define APP_SKD_STORAGE_FILE	"/spiffs/schedule_%02d.txt"

#define APP_SKD_MONDAY		0x01
#define APP_SKD_TUESDAY		0x02
#define APP_SKD_WEDNESDAY	0x04
#define APP_SKD_THURSDAY	0x08
#define APP_SKD_FRIDAY		0x10
#define APP_SKD_SATURDAY	0x20
#define APP_SKD_SUNDAY		0x40

typedef enum __CURTAIN_ACTION_TYPE{
	CURT_ACT_STOP,
	CURT_ACT_OPEN,
	CURT_ACT_CLOSE,
	CURT_ACT_OPEN_LEVEL,
} curtain_action_t;

typedef struct {
	/*parameters get from schedule input*/
	int iID;
	int iStatus;
	int iActive;
	float fTimeZone;
	int iSkdType;
	int iLogicType;
	int iConditionType;
	uint64_t u64ExcutionTime;
	uint8_t u8LoopDays;
	int iSwitch1;
	bool bSw1Action;
	int iSwitch2;
	bool bSw2Action;
	int iSwitch3;
	bool bSw3Action;
	int iSwitch4;
	bool bSw4Action;
	int iCurt1;
	bool bCurt1Action;
	int iCurt1Level;
	int iCurt2;
	bool bCurt2Action;
	int iCurt2Level;
	/*parameters for handle in program*/
	bool bExist;		/*is exist so it can be updated, deleted, run schedule or if not, it can be added*/
	int  iStack;		/*stack in flash storage*/
	bool bBusy;			/*is handling in schedule thread or in add, update, delete, etc ...*/
	bool bSkdRun;		/*ensure that scheduled only run once time (with 30s loop check)*/
	bool bChanged;		/*if Changed must be save in flash later*/
} sSkd_t;

void app_skd_init();

int app_skd_add_return_id(const char* strSchedule, int* pId);
int app_skd_update_return_id(const char* strSchedule, int* pId);

int app_skd_add(const char* strSchedule);
int app_skd_delete(int id);
int app_skd_update(const char* strSchedule);
int app_skd_delete_all();
int app_skd_list(int* pList);
const sSkd_t* app_skd_get(int id);

int app_skd_parse_schedule_string(sSkd_t* pSkd, const char* strSkd);
int app_skd_parse_schedule(sSkd_t* pSkd, const cJSON* pJsonSkd);

void app_skd_build_json_value(cJSON* pValue, const sSkd_t* pSkd);

void app_skd_print_schedule(const sSkd_t* pSkd);
void app_skd_print_schedule_list();

void app_skd_get_storage_file_name(char* buffer, int stack);

int app_skd_add_to_storage(const char* strSkd, int stack);
int app_skd_delete_from_storage(int stack);

void app_skd_run(sSkd_t* pSkd);
bool app_skd_check_condition(sSkd_t* pSkd);
void app_skd_run_action(sSkd_t* pSkd);

uint8_t app_skd_weekday_binary(int weekday);

void app_skd_flash_handle(void);

#endif /* __APP_SCHEDULE_H */
