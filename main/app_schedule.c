/*
 * app_schedule.c
 *
 *  Created on: Mar 4, 2020
 *      Author: laian
 */

#include "app_schedule.h"


static sSkd_t SchedulesList[APP_SKD_MAXIMUM];
static int skdCount;
static char skdBuffer[APP_SKD_BUFFER_SIZE] = {0};
static int StupidId = -1;
static TaskHandle_t xScheduleHandle = NULL;

static void task_schedule(void *parm);

void app_skd_init() {
	char skdStorageFile[64] = {0};
	for(int i=0; i<APP_SKD_MAXIMUM; i++) {
		SchedulesList[i].bExist = false;
		SchedulesList[i].bBusy = false;
		SchedulesList[i].bSw1Action = false;
		SchedulesList[i].bSw2Action = false;
		SchedulesList[i].bSw3Action = false;
		SchedulesList[i].bSkdRun = false;
		SchedulesList[i].iStack = i+1;
		SchedulesList[i].bChanged = false;
	}
	skdCount = 0;

	for(int i=0; i<APP_SKD_MAXIMUM; i++) {
		app_skd_get_storage_file_name(skdStorageFile, SchedulesList[i].iStack);
		if(app_spiffs_check_file_exist(skdStorageFile)) {
			if(app_spiffs_read_file(skdStorageFile, skdBuffer, APP_SKD_BUFFER_SIZE-1) == ESP_OK) {
				LREP(__FUNCTION__, "%s content\r\n%s", skdStorageFile, skdBuffer);
				if(app_skd_parse_schedule_string(&SchedulesList[i], skdBuffer) != ERROR_CODE_OK) {
					LREP_ERROR(__FUNCTION__, "file %s content invalid and will be delete", skdStorageFile);
					app_spiffs_delete_file(skdStorageFile);
				}
				else {
					SchedulesList[i].bExist = true;
					skdCount++;
				}
			}
			else {
				LREP_ERROR(__FUNCTION__, "Error read from %s", skdStorageFile);
			}
		}
	}
	app_skd_print_schedule_list();

	xTaskCreate(task_schedule, "task_schedule", 4096, NULL, 3, &xScheduleHandle);

}

int app_skd_add_return_id(const char* strSchedule, int* pId) {
	int ret = app_skd_add(strSchedule);
	*pId = StupidId;
	return ret;
}

int app_skd_add(const char* strSchedule) {
	if(skdCount >= APP_SKD_MAXIMUM)
	{
		LREP_ERROR(__FUNCTION__,"skd ID maximum ");
		return ERROR_CODE_SCHEDULE_FULL;
	}
	sSkd_t* pSkd = NULL;
	for(int i = 0; i < APP_SKD_MAXIMUM; i++) {
		if(SchedulesList[i].bExist == false) {
			pSkd = &SchedulesList[i];
			break;
		}
	}
	if(pSkd == NULL) {
		LREP_ERROR(__FUNCTION__,"owl night is a stupid piece of shit, everything will be crash");
		return ERROR_CODE_SCHEDULE_FULL;
	}
	int ret= app_skd_parse_schedule_string(pSkd, strSchedule) ;

	if(ret == ERROR_CODE_OK) {
		for(int i = 0; i < APP_SKD_MAXIMUM; i++) {
			if(SchedulesList[i].bExist && SchedulesList[i].iID == pSkd->iID) {
				LREP_ERROR(__FUNCTION__, "another schedule with same id existed");
				return ERROR_CODE_OK; //ERROR_CODE_SCHEDULE_ID_EXISTED;
			}
		}
#if 0
		if(app_skd_add_to_storage(strSchedule, pSkd->iStack) == ERROR_CODE_OK) {
			skdCount++;
			pSkd->bExist = true;
			ret = ERROR_CODE_OK;
		} else {
			LREP_ERROR(__FUNCTION__,"add to storage error");
			ret= ERROR_CODE_UNDEFINED;
		}
#else
		pSkd->bChanged = true;
		skdCount++;
		pSkd->bExist = true;
		ret = ERROR_CODE_OK;
#endif
	} else {
		LREP_ERROR(__FUNCTION__,"data input invalid");
	}
	app_skd_print_schedule(pSkd);
	return ret;
}

int app_skd_delete(int id) {
	sSkd_t* pSkd = NULL;
	for(int i=0; i<APP_SKD_MAXIMUM; i++) {
		if(SchedulesList[i].iID == id && SchedulesList[i].bExist) {
			pSkd = &SchedulesList[i];
			break;
		}
	}
	if(pSkd==NULL) {
		LREP_ERROR(__FUNCTION__, "schedule with id %d not found", id);
		return ERROR_CODE_SCHEDULE_NOT_INIT;
	}
	int timeOutCount = 0;
	while(pSkd->bBusy) {
		vTaskDelay(10);
		if(timeOutCount++>10) {
			return ERROR_CODE_UNDEFINED;
		}
	}
	pSkd->bExist = false;
	skdCount--;
#if 0
	app_skd_delete_from_storage(pSkd->iStack);
#else
	pSkd->bChanged = true;
#endif
	return ERROR_CODE_OK;
}

int app_skd_delete_all() {
	for(int i=0; i<APP_SKD_MAXIMUM; i++) {
		if(SchedulesList[i].bExist) {
			LREP_WARNING(__FUNCTION__,"delete skd %d @%d", SchedulesList[i].iID,i);
			SchedulesList[i].bExist = false;
#if 0
			app_skd_delete_from_storage(SchedulesList[i].iStack);
#else
			SchedulesList[i].bChanged = true;
#endif
		}
	}
	skdCount = 0;
	return ERROR_CODE_OK;
}
int app_skd_update_return_id(const char* strSchedule, int* pId) {
	int ret = app_skd_update(strSchedule);
	*pId = StupidId;
	return ret;
}

int app_skd_update(const char* strSchedule) {
	sSkd_t skdBuffer;
	int ret = app_skd_parse_schedule_string(&skdBuffer, strSchedule);
	if (ret != ERROR_CODE_OK) {
		LREP_ERROR(__FUNCTION__,"data input invalid");
		return ret;
	}
	sSkd_t* pSkd = NULL;
	for(int i=0; i<APP_SKD_MAXIMUM; i++) {
		if(SchedulesList[i].iID == skdBuffer.iID && SchedulesList[i].bExist) {
			pSkd = &SchedulesList[i];
			break;
		}
	}
	if(pSkd==NULL) {
		LREP_ERROR(__FUNCTION__, "schedule with id %d not found", skdBuffer.iID);
		return ERROR_CODE_SCHEDULE_NOT_INIT;
	}
	int timeOutCount = 0;
	while(pSkd->bBusy) {
		vTaskDelay(10);
		if(timeOutCount++>10) {
			return ERROR_CODE_UNDEFINED;
		}
	}
	pSkd->bBusy = true;
#if 0
	ret= app_skd_add_to_storage(strSchedule, pSkd->iStack);
	if (ret == ERROR_CODE_OK) {
		skdBuffer.bExist = true;
		skdBuffer.iStack = pSkd->iStack;
		(*pSkd) = skdBuffer;
	}
	else {
		LREP_ERROR(__FUNCTION__,"add to storage error");
	}
#else
	skdBuffer.bExist = true;
	skdBuffer.iStack = pSkd->iStack;
	(*pSkd) = skdBuffer;
	pSkd->bChanged = true;
#endif
	pSkd->bBusy = false;
	app_skd_print_schedule(pSkd);
	return ret;
}

int app_skd_list(int* pList) {
	if (pList == NULL) return skdCount;
	for (int i = 0; i < APP_SKD_MAXIMUM; i++) {
		if (SchedulesList[i].bExist) {
			*pList = SchedulesList[i].iID;
			pList++;
		}
	}
	return skdCount;
}

const sSkd_t* app_skd_get(int id) {
	for (int i = 0; i < APP_SKD_MAXIMUM; i++) {
		if (SchedulesList[i].bExist && SchedulesList[i].iID == id) {
			return &SchedulesList[i];
		}
	}
	return NULL;
}

int app_skd_parse_schedule_string(sSkd_t* pSkd, const char* strSkd) {
	pSkd->bSw1Action = false;
	pSkd->bSw2Action = false;
	pSkd->bSw3Action = false;
	pSkd->bSw4Action = false;
	pSkd->bCurt1Action = false;
	pSkd->bCurt2Action = false;
	pSkd->bSkdRun = false;
	cJSON* pJsonSkd = cJSON_Parse(strSkd);
	if(pJsonSkd == NULL) return ERROR_CODE_JSON_FORMAT;
	int ret = app_skd_parse_schedule(pSkd, pJsonSkd);
	cJSON_Delete(pJsonSkd);
	return ret;
}

int app_skd_parse_schedule(sSkd_t* pSkd, const cJSON* pJsonSkd) {
	if(pJsonSkd == NULL || pSkd == NULL) {
		LREP_ERROR(__FUNCTION__,"Null ERROR");
		return ERROR_CODE_JSON_FORMAT;
	}
	if(!cJSON_IsObject(pJsonSkd)) {
		LREP_ERROR(__FUNCTION__,"Not object ERROR");
		return ERROR_CODE_JSON_FORMAT;
	}
	cJSON* pActive 	= cJSON_GetObjectItem(pJsonSkd, "activate");
	cJSON* pStatus 	= cJSON_GetObjectItem(pJsonSkd, "status");
	cJSON* pId		= cJSON_GetObjectItem(pJsonSkd, "id");
	cJSON* pScheduleType = cJSON_GetObjectItem(pJsonSkd, "scheduleType");
	cJSON* pTimezone = cJSON_GetObjectItem(pJsonSkd, "timezone");
	cJSON* pCondition = cJSON_GetObjectItem(pJsonSkd, "condition");
	cJSON* pActionArr = cJSON_GetObjectItem(pJsonSkd, "action");
	if(pActive == NULL || pStatus == NULL || pId == NULL || pScheduleType == NULL || pTimezone == NULL || pCondition == NULL || pActionArr == NULL) {
		LREP_ERROR(__FUNCTION__, "lack essential values");
		return ERROR_CODE_INVALID_PARAM;
	}
	if(cJSON_IsNumber(pActive) && cJSON_IsNumber(pStatus) && cJSON_IsNumber(pId) &&cJSON_IsNumber(pScheduleType) &&
			cJSON_IsNumber(pTimezone) && cJSON_IsObject(pCondition) && cJSON_IsArray(pActionArr)) {
		pSkd->iActive = pActive->valueint;
		pSkd->iStatus = pStatus->valueint;
		pSkd->iID = pId->valueint;
		StupidId = pSkd->iID;
		pSkd->iSkdType = pScheduleType->valueint;
		pSkd->fTimeZone = pTimezone->valuedouble;

		cJSON* pLogicType = cJSON_GetObjectItem(pCondition, "logicType");
		cJSON* pCondValue = cJSON_GetObjectItem(pCondition, "values");
		if(pLogicType == NULL || pCondValue == NULL || !cJSON_IsNumber(pLogicType) || !cJSON_IsArray(pCondValue)) {
			LREP_ERROR(__FUNCTION__,"condition is not good");
			return ERROR_CODE_INVALID_PARAM;
		}
		else {
			pSkd->iLogicType = pLogicType->valueint;
			if(cJSON_GetArraySize(pCondValue) < 1) {
				LREP_ERROR(__FUNCTION__,"lack condition");
				return ERROR_CODE_INVALID_PARAM;
			}
			cJSON* pValue = cJSON_GetArrayItem(pCondValue, 0);
			if(pValue == NULL || !cJSON_IsObject(pValue)) {
				LREP_ERROR(__FUNCTION__,"condition ERROR");
				return ERROR_CODE_INVALID_PARAM;
			}
			cJSON* pExcuteTime = cJSON_GetObjectItem(pValue, "executionTime");
			cJSON* pLoopDay = cJSON_GetObjectItem(pValue, "loopDays");
			if(pExcuteTime == NULL || !cJSON_IsNumber(pExcuteTime) ) {
				LREP_ERROR(__FUNCTION__,"execution time");
				return ERROR_CODE_INVALID_PARAM;
			}
			pSkd->u64ExcutionTime = (uint64_t)pExcuteTime->valuedouble;
			if(pLoopDay == NULL || !cJSON_IsNumber(pLoopDay)){
				pSkd->u8LoopDays = 0;
			} else	pSkd->u8LoopDays = (uint8_t)pLoopDay->valueint;
		}
		if(cJSON_GetArraySize(pActionArr) < 1) {
			LREP_ERROR(__FUNCTION__, "Action array is not good");
			return ERROR_CODE_INVALID_PARAM;
		}
		cJSON* pAction = cJSON_GetArrayItem(pActionArr , 0);
		if(pAction==NULL || !cJSON_IsObject(pAction)) {
			LREP_ERROR(__FUNCTION__, "action is not good");
			return ERROR_CODE_INVALID_PARAM;
		}
		cJSON* pDevType = cJSON_GetObjectItem(pAction,"devT");
		cJSON* pActionType = cJSON_GetObjectItem(pAction,"actionType");
		cJSON* pExtAddr = cJSON_GetObjectItem(pAction,"devExtAddr");
		cJSON* pDevVArr = cJSON_GetObjectItem(pAction,"devV");
		if(pDevType == NULL || pActionType == NULL || pExtAddr == NULL || pDevVArr == NULL ||
				!cJSON_IsNumber(pDevType) || !cJSON_IsNumber(pActionType) || !cJSON_IsString(pExtAddr) || !cJSON_IsArray(pDevVArr)) {
			LREP_ERROR(__FUNCTION__, "device type, action type, extend address, devV are no good");
			return ERROR_CODE_INVALID_PARAM;
		}
		if(strcmp(pExtAddr->valuestring, app_wifi_get_mac_str()) != 0 || pDevType->valueint != DEVICE_TYPE || pActionType->valueint != 1) {
			LREP_ERROR(__FUNCTION__,"extend address, device type, action type are not suitable");
			return ERROR_CODE_INVALID_PARAM;
		}
		for(int i=0; i<cJSON_GetArraySize(pDevVArr); i++) {
			cJSON* pDevV = cJSON_GetArrayItem(pDevVArr, i);
			if(pDevV == NULL || !cJSON_IsObject(pDevV)) continue;
			cJSON* pParam = cJSON_GetObjectItem(pDevV, "param");
			cJSON* pParamValue = cJSON_GetObjectItem(pDevV, "value");
			if(pParam == NULL || pParamValue== NULL || !cJSON_IsString(pParam) || !cJSON_IsNumber(pParamValue)) {
				LREP_WARNING(__FUNCTION__,"param value is not good");
				continue;
			}
#ifdef DEVICE_TYPE
			if(strcmp(pParam->valuestring, SWITCH_1) == 0) {
				pSkd->iSwitch1 = pParamValue->valueint;
				pSkd->bSw1Action = true;
			}
			else if (strcmp(pParam->valuestring, SWITCH_2) == 0) {
				pSkd->iSwitch2 = pParamValue->valueint;
				pSkd->bSw2Action = true;
			}
			else if (strcmp(pParam->valuestring, SWITCH_3) == 0) {
				pSkd->iSwitch3 = pParamValue->valueint;
				pSkd->bSw3Action = true;
			}
			else LREP_WARNING(__FUNCTION__,"param is not good %s", pParam->valuestring);
#endif
		}
	}
	else {
		LREP_ERROR(__FUNCTION__,"values type");
		return ERROR_CODE_INVALID_PARAM;
	}
	return ERROR_CODE_OK;
}

void app_skd_build_json_value(cJSON* pValue, const sSkd_t* pSkd) {
	if (pValue == NULL) return;
	cJSON_AddNumberToObject(pValue, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(pValue, "devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(pValue, "activate", pSkd->iActive);
	cJSON_AddNumberToObject(pValue, "status", pSkd->iStatus);
	cJSON_AddNumberToObject(pValue, "id", pSkd->iID);
	cJSON_AddNumberToObject(pValue, "scheduleType", pSkd->iSkdType);
	cJSON_AddNumberToObject(pValue, "timezone", pSkd->fTimeZone);

	cJSON* pCondition = cJSON_AddObjectToObject(pValue, "condition");
	cJSON_AddNumberToObject(pCondition, "logicType", pSkd->iLogicType);
	cJSON* pValuesArr = cJSON_AddArrayToObject(pCondition, "values");
	cJSON* pCondValue = cJSON_CreateObject();
	cJSON_AddNumberToObject(pCondValue, "conditionType", pSkd->iConditionType);
	cJSON_AddNumberToObject(pCondValue, "executionTime", pSkd->u64ExcutionTime);
	cJSON_AddNumberToObject(pCondValue, "loopDays", pSkd->u8LoopDays);
	cJSON_AddItemToArray(pValuesArr, pCondValue);

	cJSON* pActionArr = cJSON_AddArrayToObject(pValue, "action");
	cJSON* pAction = cJSON_CreateObject();
	cJSON_AddNumberToObject(pAction, "actionType", 1);
	cJSON_AddNumberToObject(pAction, "devT", DEVICE_TYPE);
	cJSON_AddStringToObject(pAction, "devExtAddr", app_wifi_get_mac_str());
	cJSON_AddNumberToObject(pAction, "devSubA", 1);

	cJSON* pDevVArr = cJSON_AddArrayToObject(pAction, "devV");
#ifdef DEVICE_TYPE
	if (pSkd->bSw1Action) PARAM_INT_VALUE_ADD(pDevVArr, SWITCH_1, pSkd->iSwitch1);
	if (pSkd->bSw2Action) PARAM_INT_VALUE_ADD(pDevVArr, SWITCH_2, pSkd->iSwitch2);
	if (pSkd->bSw3Action) PARAM_INT_VALUE_ADD(pDevVArr, SWITCH_3, pSkd->iSwitch3);
#endif
	cJSON_AddItemToArray(pActionArr, pAction);
}

void app_skd_print_schedule(const sSkd_t* pSkd) {
	LREP(__FUNCTION__,"Schedule Id %d", pSkd->iID);
	LREP_RAW("\tstatus %d", pSkd->iStatus);
	LREP_RAW("\tactivate %d", pSkd->iActive);
	LREP_RAW("\ttimezone %f\r\n", pSkd->fTimeZone);
	LREP_RAW("\tSchedule type %d", pSkd->iSkdType);
	LREP_RAW("\tlogic type %d", pSkd->iLogicType);
	LREP_RAW("\tcondition type %d",pSkd->iConditionType);
	LREP_RAW("\texecution time %lld", pSkd->u64ExcutionTime);
	LREP_RAW("\tloop day %d\r\n", pSkd->u8LoopDays);
#ifdef DEVICE_TYPE
	if(pSkd->bSw1Action) LREP_RAW("\tSwitch 1 %d", pSkd->iSwitch1);
	if(pSkd->bSw2Action) LREP_RAW("\tSwitch 2 %d", pSkd->iSwitch2);
	if(pSkd->bSw3Action) LREP_RAW("\tSwitch 3 %d", pSkd->iSwitch3);
	LREP_RAW("\r\n");
#endif
}

void app_skd_print_schedule_list() {
	LREP(__FUNCTION__,"Total schedule %d", skdCount);
	for(int i=0; i<APP_SKD_MAXIMUM; i++) {
		if(SchedulesList[i].bExist)
		app_skd_print_schedule(SchedulesList+i);
	}
}

void app_skd_get_storage_file_name(char* buffer, int stack) {
	sprintf(buffer, APP_SKD_STORAGE_FILE, stack);
}

int app_skd_add_to_storage(const char* strSkd, int stack) {
	char storageFile[64] = {0};
	app_skd_get_storage_file_name(storageFile, stack);

	app_spiffs_delete_file(storageFile);
	if(app_spiffs_write_file(storageFile, strSkd) != ESP_OK) {
		LREP_ERROR(__FUNCTION__,"Error writing to %s", storageFile);
		return ERROR_CODE_UNDEFINED;
	}
	return ERROR_CODE_OK;
}

int app_skd_delete_from_storage(int stack) {
	char storageFile[64] = {0};
	app_skd_get_storage_file_name(storageFile, stack);

	app_spiffs_delete_file(storageFile);
	return ERROR_CODE_OK;
}

void task_schedule(void *parm) {
	for(;;) {
		g_bRun_SkdTask = true;
		if(app_time_server_sync()) {
			for(int i=0; i<APP_SKD_MAXIMUM; i++) {
				sSkd_t* pSkd = &SchedulesList[i];
				if(pSkd->bExist && !pSkd->bBusy && pSkd->iActive == 1) {
					pSkd->bBusy = true;
					app_skd_run(pSkd);
					pSkd->bBusy = false;
				}
			}
		}
		vTaskDelay(30000/portTICK_PERIOD_MS);
	}
}

void app_skd_run(sSkd_t* pSkd) {
	if(app_skd_check_condition(pSkd)) {
		if (pSkd->bSkdRun) {
			pSkd->bSkdRun = false;
		}
		else {
			LREP_WARNING(__FUNCTION__, "Skd id %d run action ", pSkd->iID);
			app_skd_run_action(pSkd);
			pSkd->bSkdRun = true;
		}
	}
}

bool app_skd_check_condition(sSkd_t* pSkd) {
	if(pSkd->u8LoopDays == 0) {
		uint64_t timeNow = app_time_server_get_timestamp_now();
		if((timeNow > pSkd->u64ExcutionTime) && (int)abs(timeNow - pSkd->u64ExcutionTime) <= 60)
			return true;
	}
	else {
		/*get hour & minute from condition*/
		struct tm tm_TimeCheck = *app_time_server_get_timetm(pSkd->u64ExcutionTime, pSkd->fTimeZone);
		/*get week_day, hour, minute from system*/
		struct tm tm_TimeNow = *app_time_server_get_timetm(app_time_server_get_timestamp_now(), pSkd->fTimeZone);
		/*check loop day*/
		uint8_t u8wday = app_skd_weekday_binary(tm_TimeNow.tm_wday);
		if((u8wday & pSkd->u8LoopDays) == 0) {
			LREP_WARNING(__FUNCTION__,"Loopday");
			return false;
		}
		/*check hour & minute*/
		if((int)abs(tm_TimeCheck.tm_hour - tm_TimeNow.tm_hour) == 0 ) {
			if((int)abs(tm_TimeCheck.tm_min - tm_TimeNow.tm_min) <= 0)
				return true;
		}
	}
	return false;
}

void app_skd_run_action(sSkd_t* pSkd) {
#ifdef DEVICE_TYPE
	if(pSkd->bSw1Action) {
		app_remote_change_channel_status(SW_CHANNEL_1, pSkd->iSwitch1);
	}
	if(pSkd->bSw2Action) {
		app_remote_change_channel_status(SW_CHANNEL_2, pSkd->iSwitch2);
	}
	if(pSkd->bSw3Action) {
		app_remote_change_channel_status(SW_CHANNEL_3, pSkd->iSwitch3);
	}
#endif
}

uint8_t app_skd_weekday_binary(int weekday) {
	switch (weekday) {
	case 0:
		return APP_SKD_SUNDAY;
	case 1:
		return APP_SKD_MONDAY;
	case 2:
		return APP_SKD_TUESDAY;
	case 3:
		return APP_SKD_WEDNESDAY;
	case 4:
		return APP_SKD_THURSDAY;
	case 5:
		return APP_SKD_FRIDAY;
	case 6:
		return APP_SKD_SATURDAY;
	default:
		break;
	}
	return 0x00;
}

void app_skd_flash_handle(void) {
	for (int i =0; i < APP_SKD_MAXIMUM; ++i) {
		if (!SchedulesList[i].bBusy && SchedulesList[i].bChanged) {
			SchedulesList[i].bBusy = true;
			if (SchedulesList[i].bExist) {
				char *data_string;
				cJSON *pDevV = cJSON_CreateObject();
				app_skd_build_json_value(pDevV, &SchedulesList[i]);
				data_string = cJSON_PrintUnformatted(pDevV);
				LREP_WARNING(__FUNCTION__, "Update skd %d @%d", SchedulesList[i].iID, SchedulesList[i].iStack);
				LREP(__FUNCTION__, "%s", data_string);
				app_skd_add_to_storage(data_string, SchedulesList[i].iStack);
				free(data_string);
				cJSON_Delete(pDevV);
			}
			else {
				LREP_WARNING(__FUNCTION__, "Delete skd %d @%d", SchedulesList[i].iID, SchedulesList[i].iStack);
				app_skd_delete_from_storage(SchedulesList[i].iStack);
			}
			SchedulesList[i].bChanged = false;
			SchedulesList[i].bBusy = false;
		}
	}
}
