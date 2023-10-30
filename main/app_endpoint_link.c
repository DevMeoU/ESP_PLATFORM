/*
 * app_endpoint_link.c
 *
 *  Created on: 11 Aug 2020
 *      Author: tongv
 */
#include "app_endpoint_link.h"

sEndpointLink_t g_endpoint_link_arr[MAX_ENDPOINT_LINK];
sEndpointLink_t g_endpoint_link;

static char endpoint_linkBuffer[APP_LINK_ENDPOINT_BUFFER_SIZE] = {0};
/*
 *
 */
void app_endpoint_link_init()
{
	LREP(__FUNCTION__,"START");
	for(int i=1;i<MAX_ENDPOINT_LINK;i++)
	{
		memset(&g_endpoint_link_arr[i], 0, sizeof(sEndpointLink_t));
		if(app_endpoint_link_get_file(i)==ERROR_CODE_OK)
		{
			app_endpoint_link_copy(&g_endpoint_link_arr[i], &g_endpoint_link);
			g_endpoint_link_arr[i].bChanged = false;
#if 0
			LREP_WARNING(__FUNCTION__, "Endpoint link %d" , i);
			LREP_WARNING(__FUNCTION__, "\t\tSource EP:%d, ID:%s", g_endpoint_link_arr[i].iSrcEndpoint, g_endpoint_link_arr[i].chsrcDevId);
			LREP_WARNING(__FUNCTION__, "\t\tDestination EP:%d, ID:%s", g_endpoint_link_arr[i].iDesEndpoint, g_endpoint_link_arr[i].chdesDevId);
#endif
		}
	}
	LREP(__FUNCTION__,"END");
}
/*
 *
 */
int  app_endpoint_link_add(const char* strEndpointLink)
{
	LREP(__FUNCTION__,"%s",strEndpointLink);
	cJSON *pJson_epl = cJSON_Parse(strEndpointLink);
	int ret = app_endpoint_link_parse(pJson_epl);
//	if(ret == ERROR_CODE_OK)
//	{
//		char* data_string = cJSON_PrintUnformatted(pJson_epl);
//		const char* filename = app_endpoint_link_get_file_name(g_endpoint_link.iEndpoint);
//		if (filename) {
//			if (app_spiffs_check_file_exist(filename)) app_spiffs_delete_file(filename);
//			if (app_spiffs_write_file(filename, data_string) != ESP_OK) {
//				LREP_ERROR(__FUNCTION__, "Error writing to %s", filename);
//				ret = ERROR_CODE_UNDEFINED;
//			}
//			else {
//				LREP(__FUNCTION__, "writing endpoint %d done", g_endpoint_link.iEndpoint);
//			}
//		}
//		else {
//			LREP_ERROR(__FUNCTION__, "Endpoint not valid %d", g_endpoint_link.iEndpoint);
//			ret = ERROR_CODE_UNDEFINED;
//		}
//		free(data_string);
//	}
	cJSON_Delete(pJson_epl);

	if (ret == ERROR_CODE_OK) {
		int i = g_endpoint_link.iEndpoint;

		app_endpoint_link_copy(&g_endpoint_link_arr[i], &g_endpoint_link);
		g_endpoint_link_arr[i].bChanged = true;

		LREP_WARNING(__FUNCTION__, "Endpoint link %d" , i);
		LREP_WARNING(__FUNCTION__, "Source EP:%d, ID:%s", g_endpoint_link_arr[i].iSrcEndpoint, g_endpoint_link_arr[i].chsrcDevId);
		LREP_WARNING(__FUNCTION__, "Destination EP:%d, ID:%s", g_endpoint_link_arr[i].iDesEndpoint, g_endpoint_link_arr[i].chdesDevId);
	}

	return ret;
}
/*
 *
 */
int  app_endpoint_link_delete(int endpoint)
{
//	LREP(__FUNCTION__,"start");
//	int ret = ERROR_CODE_UNDEFINED;

//	const char* filename = app_endpoint_link_get_file_name(endpoint);

//	if (filename) {
//		if (app_spiffs_check_file_exist(filename)) {
//			if (app_spiffs_delete_file(filename) == ESP_OK) {
//				ret = ERROR_CODE_OK;
//			}
//			else ret = ERROR_CODE_UNDEFINED;
//		}
//		else ret = ERROR_CODE_OK;
//
//	}
//	else return ERROR_CODE_OK;

	memset(&g_endpoint_link_arr[endpoint], 0, sizeof(sEndpointLink_t));
	g_endpoint_link_arr[endpoint].bExist = false;
	g_endpoint_link_arr[endpoint].bChanged = true;

	return ERROR_CODE_OK;
}
/*
 *
 */
int  app_endpoint_link_parse(const cJSON* pJson_epl)
{
	LREP(__FUNCTION__,"start");
	if(pJson_epl ==NULL)
	{
		LREP_ERROR(__FUNCTION__,"Null ERROR");
		return ERROR_CODE_INVALID_PARAM;
	}
	if(!cJSON_IsObject(pJson_epl))
	{
		LREP_ERROR(__FUNCTION__,"Not object ERROR");
		return ERROR_CODE_INVALID_PARAM;
	}
	cJSON* pEndpoint 	= cJSON_GetObjectItem(pJson_epl, "endpoint");
	cJSON* pSrcDevId 	= cJSON_GetObjectItem(pJson_epl, "srcDevId");
	cJSON* pSrcEndpoint		= cJSON_GetObjectItem(pJson_epl, "srcEndpoint");
	cJSON* pDesDevId = cJSON_GetObjectItem(pJson_epl, "desDevId");
	cJSON* pDesEndpoint = cJSON_GetObjectItem(pJson_epl, "desEndpoint");

	if(pEndpoint == NULL || pSrcDevId == NULL || pSrcEndpoint == NULL || pDesDevId == NULL || pDesEndpoint == NULL
			|| !cJSON_IsString(pSrcDevId) || !cJSON_IsNumber(pSrcEndpoint) || !cJSON_IsString(pDesDevId)
			|| !cJSON_IsNumber(pEndpoint)|| !cJSON_IsNumber(pDesEndpoint))
	{
		LREP_ERROR(__FUNCTION__,"lack essential values");
		return ERROR_CODE_INVALID_PARAM;
	}

	memset(&g_endpoint_link, 0 , sizeof(sEndpointLink_t));
	strncpy(g_endpoint_link.chsrcDevId, pSrcDevId->valuestring, APP_LINK_ENDPOINT_ID_SIZE);
	strncpy(g_endpoint_link.chdesDevId, pDesDevId->valuestring, APP_LINK_ENDPOINT_ID_SIZE);
	g_endpoint_link.iEndpoint = pEndpoint->valueint;
	g_endpoint_link.iSrcEndpoint =pSrcEndpoint->valueint;
	g_endpoint_link.iDesEndpoint =pDesEndpoint->valueint;
	return ERROR_CODE_OK;
}
/*
 *
 */
int app_endpoint_link_get_file(int endpoint)
{
	int ret = ERROR_CODE_INVALID_PARAM;

	const char* filename = app_endpoint_link_get_file_name(endpoint);

	if (filename) {
		if (app_spiffs_check_file_exist(filename) ) {
			if (app_spiffs_read_file(filename, endpoint_linkBuffer, APP_LINK_ENDPOINT_BUFFER_SIZE-1) == ESP_OK) {
				cJSON *pJson_epl = cJSON_Parse(endpoint_linkBuffer);
				ret = app_endpoint_link_parse(pJson_epl);
			}
		}
	}
	return ret;
}
/*
 *
 */
int app_endpoint_link_delete_all()
{
	LREP(__FUNCTION__,"start");
	int ret = ERROR_CODE_OK;

	if (app_endpoint_link_delete(1) != ERROR_CODE_OK) {
		ret = ERROR_CODE_UNDEFINED;
	}

	if (app_endpoint_link_delete(2) != ERROR_CODE_OK) {
		ret = ERROR_CODE_UNDEFINED;
	}

	if (app_endpoint_link_delete(3) != ERROR_CODE_OK) {
		ret = ERROR_CODE_UNDEFINED;
	}

	if (app_endpoint_link_delete(4) != ERROR_CODE_OK) {
		ret = ERROR_CODE_UNDEFINED;
	}

	LREP(__FUNCTION__," stop %d",ret);
	return ret;
}

const char* app_endpoint_link_get_file_name(int endpoint_link) {
#ifdef DEVICE_TYPE
	const char* filename = (endpoint_link == 1) ? APP_STORAGE_FILE_LINK_ENDPOINT_1 :
						(endpoint_link == 2) ? APP_STORAGE_FILE_LINK_ENDPOINT_2 :
								(endpoint_link == 3) ? APP_STORAGE_FILE_LINK_ENDPOINT_3 : NULL;
	return filename;
#else
	return NULL;
#endif
}

void app_endpoint_link_copy(sEndpointLink_t* pEplinkOut, const sEndpointLink_t* pEplinkIn) {
	pEplinkOut->iEndpoint    = pEplinkIn->iEndpoint;
	strncpy(pEplinkOut->chsrcDevId, pEplinkIn->chsrcDevId, APP_LINK_ENDPOINT_ID_SIZE);
	pEplinkOut->iSrcEndpoint = pEplinkIn->iSrcEndpoint;
	strncpy(pEplinkOut->chdesDevId, pEplinkIn->chdesDevId, APP_LINK_ENDPOINT_ID_SIZE);
	pEplinkOut->iDesEndpoint = pEplinkIn->iDesEndpoint;
	pEplinkOut->bExist = true;
}

void app_endpoint_link_flash_handle() {
	for(int i=1;i<MAX_ENDPOINT_LINK;i++) {
		if (g_endpoint_link_arr[i].bChanged) {
			const char* filename = app_endpoint_link_get_file_name(i);
			if (!filename) continue;
			if (app_spiffs_check_file_exist(filename)) {
				LREP_WARNING(__FUNCTION__, "Delete %s",filename);
				app_spiffs_delete_file(filename);
			}

			if (g_endpoint_link_arr[i].bExist) {
				char *data_string;
				cJSON *pDevV = cJSON_CreateObject();
				app_endpoint_link_build_json_value(pDevV, i);
				data_string = cJSON_PrintUnformatted(pDevV);
				LREP_WARNING(__FUNCTION__, "Save %s", filename);
				LREP(__FUNCTION__, "%s", data_string);
				app_spiffs_write_file(filename, data_string);
				free(data_string);
				cJSON_Delete(pDevV);
			}

			g_endpoint_link_arr[i].bChanged = false;
		}
	}
}

void app_endpoint_link_build_json_value(cJSON* pValue, int ep) {
	if (pValue==NULL) return;
	cJSON_AddNumberToObject(pValue, "endpoint",ep);
	cJSON_AddStringToObject(pValue, "srcDevId",		g_endpoint_link_arr[ep].chsrcDevId);
	cJSON_AddNumberToObject(pValue, "srcEndpoint",	g_endpoint_link_arr[ep].iSrcEndpoint);
	cJSON_AddStringToObject(pValue, "desDevId",		g_endpoint_link_arr[ep].chdesDevId);
	cJSON_AddNumberToObject(pValue, "desEndpoint",	g_endpoint_link_arr[ep].iDesEndpoint);
}
