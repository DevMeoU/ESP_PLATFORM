/*
 * app_timer.c
 *
 *  Created on: Dec 25, 2019
 *      Author: Admin
 */

#include "app_time_server.h"
//#include "app_wifi_mode.h"
#include "config.h"

static void time_ntp_init(void);
static void time_sync_notification_cb(struct timeval *tv);

static bool bSync = false;
static void time_ntp_init(void)
{
    LREP(__FUNCTION__, "Initializing NTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif

    sntp_init();
}

static void time_sync_notification_cb(struct timeval *tv)
{
    LREP(__FUNCTION__, "Notification of a time synchronization event");
}

void app_time_server_init(void) {
	time_t now;

	time_ntp_init();

	time(&now);

	setenv("TZ", "UTC", 1);
	tzset();
}

struct tm* app_time_server_get_timetm( uint64_t timestamp, float UTC) {
	const time_t rawtime = (const time_t) timestamp + (const time_t)(UTC*3600);
	return localtime(&rawtime);
};

uint64_t app_time_server_get_timestamp(struct tm* moment) {
	time_t ret = mktime(moment);
	return (uint64_t)ret;
}
uint64_t app_time_server_get_timestamp_now() {
	time_t t = time(0);
	return (uint64_t)t;
}

bool app_time_server_sync() {
	if (bSync == false)
		bSync =  sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED;
	return bSync;
}
