/*
 * app_endpoint_link.h
 *
 *  Created on: 11 Aug 2020
 *      Author: tongv
 */

#ifndef SMART_SWITCH_MAIN_APP_ENDPOINT_LINK_H_
#define SMART_SWITCH_MAIN_APP_ENDPOINT_LINK_H_
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "cJSON.h"
#include <string.h>
#include "esp_log.h"
#include "config.h"
#include <stdio.h>
#include <time.h>
#include "app_spiffs.h"
#include <stdbool.h>

#define MAX_ENDPOINT_LINK 5
#define APP_STORAGE_FILE_LINK_ENDPOINT_1	"/spiffs/endpoint_1.txt"
#define APP_STORAGE_FILE_LINK_ENDPOINT_2	"/spiffs/endpoint_2.txt"
#define APP_STORAGE_FILE_LINK_ENDPOINT_3	"/spiffs/endpoint_3.txt"
#define APP_STORAGE_FILE_LINK_ENDPOINT_4	"/spiffs/endpoint_4.txt"
#define APP_LINK_ENDPOINT_BUFFER_SIZE	    2048
#define APP_LINK_ENDPOINT_ID_SIZE			64

typedef struct {
	int iEndpoint;
	int iSrcEndpoint;
	int iDesEndpoint;
	char chsrcDevId[APP_LINK_ENDPOINT_ID_SIZE];
	char chdesDevId[APP_LINK_ENDPOINT_ID_SIZE];
	/*parameters for handle in program*/
	bool bExist;
	bool bChanged;
} sEndpointLink_t;

extern sEndpointLink_t g_endpoint_link;
extern sEndpointLink_t g_endpoint_link_arr[MAX_ENDPOINT_LINK];

int  app_endpoint_link_add(const char* strEndpointLink);
int  app_endpoint_link_delete(int endpoint);
int  app_endpoint_link_parse(const cJSON* pJson_epl);
int  app_endpoint_link_get_file(int endpoint);
void app_endpoint_link_init();
int  app_endpoint_link_delete_all();

const char* app_endpoint_link_get_file_name(int endpoint_link);

void app_endpoint_link_copy(sEndpointLink_t* pEplinkOut, const sEndpointLink_t* pEplinkIn);
void app_endpoint_link_flash_handle();
void app_endpoint_link_build_json_value(cJSON* pValue, int ep);
#endif /* SMART_SWITCH_MAIN_APP_ENDPOINT_LINK_H_ */
