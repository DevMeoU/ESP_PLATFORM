/*
 * app_timer.h
 *
 *  Created on: Dec 25, 2019
 *      Author: Admin
 */

#ifndef MAIN_APP_TIME_SERVER_H_
#define MAIN_APP_TIME_SERVER_H_

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_sntp.h"

void app_time_server_init(void);

uint64_t app_time_server_get_timestamp_now();
uint64_t app_time_server_get_timestamp(struct tm* moment);
struct tm* app_time_server_get_timetm( uint64_t timestamp, float UTC);

bool app_time_server_sync();

#endif /* MAIN_APP_TIME_SERVER_H_ */
