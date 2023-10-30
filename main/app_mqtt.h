/*
 * app_mqtt.h
 *
 *  Created on: Dec 24, 2019
 *      Author: Admin
 */

#ifndef MAIN_APP_MQTT_H_
#define MAIN_APP_MQTT_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "config.h"

#define MQTT_MAXIMUM_MESSAGE_LENGTH 		MAX_MESSAGE_LENGTH
#define MQTT_MAXIMUM_TOPIC_LENGTH			128

#define MQTT_DEBUG		0

#define	MQTT_TOPIC_SUB_ENDPOINT_LINK		"TOPIC-VCX/SmartHome-V2/Command/"

#define MQTT_SUB_QOS			1
#define MQTT_PUB_QOS			1

typedef struct {
	char* pBrokerUrl;
	char* pBrokerUsr;
	char* pBrokerPass;
	char* pTopicSub;
	char* pTopicPub;
	char* pTopicAllert;
	char* pForceOtaUrl;
	char* pBeSharedKey;
} sCloudInfo_t;

extern bool g_bMqttstatus;
extern uint64_t g_LastestMessageTimestamp;
extern sCloudInfo_t g_CloudInfo;

void app_mqtt_start(void);
void app_mqtt_stop(void);
void app_mqtt_disconnect(void);
void app_mqtt_reconnect(int timeout_s);

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event);
esp_err_t mqtt_message_receive_handle(char* strTopic, char* strMessage);

bool app_mqtt_response(const char* message);
void app_mqtt_report_be(const char* message);

void app_mqtt_reset_subscribed_topic(void);

bool app_mqtt_buffer_response(const char* msg, int msglen);
void app_mqtt_buffer_report_be(const char* msg, int msglen);

void app_mqtt_endpoint_link_transmit(const char* strDesId,const char* message);

void app_mqtt_endpoint_link_get_topic(char* topicBuffer, const char* strDeviceId);

void app_mqtt_delete_cloud_info(void);
bool app_mqtt_check_cloud_setting(void);

#endif /* MAIN_APP_MQTT_H_ */
