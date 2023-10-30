/*
 * app_mqtt.c
 *
 *  Created on: Dec 24, 2019
 *      Author: Admin
 */

#include "app_mqtt.h"
#include "config.h"
#include "app_fs_mgr.h"
#include "app_cmd_mgr.h"
#include "app_time_server.h"

static char strSubscribeTopic[MQTT_MAXIMUM_TOPIC_LENGTH] = "/from/app/to/device";
static char strPublishTopic[MQTT_MAXIMUM_TOPIC_LENGTH] = "/from/device/to/app";
static char strAllertTopic[MQTT_MAXIMUM_TOPIC_LENGTH] = "/from/device/to/be";
static char strRxMessage[MQTT_MAXIMUM_MESSAGE_LENGTH] = {0};
static char strRxTopic[MQTT_MAXIMUM_TOPIC_LENGTH];
static char strSubscribeTopic_Endpoint_Link[MQTT_MAXIMUM_TOPIC_LENGTH] = {0};
static char strPublishTopic_Endpoint_Link[MQTT_MAXIMUM_TOPIC_LENGTH] = {0};

//#define BROKER_DEV_TLS
//#define BROKER_DEV_TCP
//#define BROKER_PRODUCT_TLS
//#define BROKER_PRODUCT_TCP

#ifdef BROKER_DEV_TLS
static char strBrokerUrl[100] = "mqtts://broker-dev.vconnex.vn:1884";
static char strBrokerUser[100] = "VKX-SmartHome";
static char strBrokerPass[100] = "Sm@rtH0me!2020";
#else /*BROKER_DEV_TLS*/
#ifdef BROKER_DEV_TCP
static char strBrokerUrl[100] = "mqtt://203.162.94.38:1883";
static char strBrokerUser[100] = {0};
static char strBrokerPass[100] = {0};
#else /*BROKER_DEV_TCP*/
#ifdef BROKER_PRODUCT_TLS
static char strBrokerUrl[100] = "mqtts://smarthome-broker.vconnex.vn:1884";
static char strBrokerUser[100] = "VKX-SmartHome";
static char strBrokerPass[100] = "Sm@rtH0me!2020";
#else /*BROKER_PRODUCT_TLS*/
#ifdef BROKER_PRODUCT_TCP
static char strBrokerUrl[100] = "mqtt://scpbroker.thingxyz.net:1883";
static char strBrokerUser[100] = {0};
static char strBrokerPass[100] = {0};
#else /*BROKER_PRODUCT_TCP*/
static char strBrokerUrl[100] = {0};
static char strBrokerUser[100] = {0};
static char strBrokerPass[100] = {0};
#endif /*BROKER_PRODUCT_TCP*/
#endif /*BROKER_PRODUCT_TLS*/
#endif /*BROKER_DEV_TCP*/
#endif /*BROKER_DEV_TLS*/

static char strForceOtaUrl[100];
static char strBeSharedKey[100];

sCloudInfo_t g_CloudInfo = {
	.pBrokerUrl = strBrokerUrl,
	.pBrokerUsr = strBrokerUser,
	.pBrokerPass = strBrokerPass,
	.pTopicSub = strSubscribeTopic,
	.pTopicPub = strPublishTopic,
	.pTopicAllert = strAllertTopic,
	.pForceOtaUrl = strForceOtaUrl,
	.pBeSharedKey = strBeSharedKey,
};

bool g_bMqttstatus = false;
bool g_bMqttPause = false;
uint64_t g_LastestMessageTimestamp = 0;
static esp_mqtt_client_handle_t client;

static xQueueHandle mqttRxQueue = NULL;
static TaskHandle_t xMqttRxHandler = NULL;

static void task_mqtt_rx(void *pvParameter);

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void app_mqtt_start(void) {
	LREP_WARNING(__FUNCTION__, "Free memory: %d bytes", esp_get_free_heap_size());
	if (esp_get_free_heap_size() < 70000) {
		LREP_WARNING(__FUNCTION__, "free heap size too small");
		esp_restart();
	}
	if(mqttRxQueue == NULL) {
		mqttRxQueue = xQueueCreate(2, MQTT_MAXIMUM_MESSAGE_LENGTH);
		xTaskCreate(&task_mqtt_rx,"task_mqtt_rx", 4096, NULL, 3, &xMqttRxHandler);
	}
#ifdef BROKER_DEV_TLS
#else /*BROKER_DEV_TLS*/
#ifdef BROKER_DEV_TCP
#else /*BROKER_DEV_TCP*/
#ifdef BROKER_PRODUCT_TLS
#else /*BROKER_PRODUCT_TLS*/
#ifdef BROKER_PRODUCT_TCP
#else /*BROKER_PRODUCT_TCP*/
	app_filesystem_read_cloud_info(&g_CloudInfo);
#endif /*BROKER_PRODUCT_TCP*/
#endif /*BROKER_PRODUCT_TLS*/
#endif /*BROKER_DEV_TCP*/
#endif /*BROKER_DEV_TLS*/

	LREP(__FUNCTION__,"MQTT_BROKER_ADDR  %s %d", strBrokerUrl,strlen(strBrokerUrl));
	LREP(__FUNCTION__,"MQTT_USERNAME  %s %d", strBrokerUser,strlen(strBrokerUser));
	LREP(__FUNCTION__,"MQTT_PASSWORD  %s %d", strBrokerPass,strlen(strBrokerPass));

	LREP(__FUNCTION__,"publish topic %s %d", strPublishTopic,strlen(strPublishTopic));
	LREP(__FUNCTION__,"subscribe topic %s %d", strSubscribeTopic,strlen(strSubscribeTopic));
	LREP(__FUNCTION__,"allert topic %s %d", strAllertTopic, strlen(strAllertTopic));
	LREP(__FUNCTION__,"force OTA skd URL %s %d", strForceOtaUrl, strlen(strForceOtaUrl));
	LREP(__FUNCTION__,"be shared key %s, %d", strBeSharedKey, strlen(strBeSharedKey));

	app_mqtt_endpoint_link_get_topic(strSubscribeTopic_Endpoint_Link, app_cmd_get_deviceId());
	LREP(__FUNCTION__, "sub endpoint link topic %s",strSubscribeTopic_Endpoint_Link);

	if (strlen(strBrokerUrl)==0) {
		LREP(__FUNCTION__,"Broker URL NULL");
		return;
	}

	LREP(__FUNCTION__,"Create MQTT client");
	memset(strRxMessage, 0, MQTT_MAXIMUM_MESSAGE_LENGTH);
	memset(strRxTopic, 0, MQTT_MAXIMUM_TOPIC_LENGTH);

    esp_mqtt_client_config_t mqtt_cfg = {
        .uri	= strBrokerUrl,
		.keepalive = 10,
		.username = strBrokerUser,
	    .password = strBrokerPass,
		.refresh_connection_after_ms = 120000,//20000,//120000,
//		.disable_auto_reconnect = true,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void app_mqtt_stop(void)
{
	TASK_DELETE_SAFE(xMqttRxHandler);

	if (mqttRxQueue) {
		vQueueDelete(mqttRxQueue);
		mqttRxQueue = NULL;
	}

	if(client == NULL) return;
	esp_mqtt_client_unsubscribe(client, strSubscribeTopic);
	esp_mqtt_client_stop(client);
	esp_mqtt_client_destroy(client);
	client = NULL;

	g_bMqttstatus = false;
	g_bMqttPause = false;
}

void app_mqtt_disconnect(void) {
	if (client == NULL) return;
	esp_mqtt_client_disconnect(client);
	int x_count = 0;
	while (g_bMqttstatus == true) {
		delay_ms(1000);
		if (x_count++ > 60) break;
	}
	g_bMqttPause = true;
}

void app_mqtt_reconnect(int timeout_s) {
	if (client == NULL) return;
	g_bMqttPause = false;
	esp_mqtt_client_reconnect(client);
	int x_count  = 0;
	while (g_bMqttstatus == false) {
		if (x_count++ > timeout_s) break;
		delay_ms(1000);
	}
}


bool app_mqtt_response(const char* message) {
	return app_mqtt_buffer_response(message, strlen(message));
}

void app_mqtt_report_be(const char* message) {
	app_mqtt_buffer_report_be(message, strlen(message));
}

bool app_mqtt_buffer_response(const char* msg, int msglen) {
	if(!g_bMqttstatus) return false;
	if(strlen(strPublishTopic) == 0) return false;
	int publishRet = esp_mqtt_client_publish(client, strPublishTopic,  (const char*)msg, msglen, MQTT_PUB_QOS, 0);
#if MQTT_DEBUG
	LREP(__FUNCTION__, "publish return %d", publishRet);
#endif
	return (publishRet >= 0);
}

void app_mqtt_buffer_report_be(const char* msg, int msglen) {
	if(!g_bMqttstatus) return;
	if(strlen(strAllertTopic) == 0) return;
	int publishRet = esp_mqtt_client_publish(client, strAllertTopic,  (const char*)msg, msglen, MQTT_PUB_QOS, 0);
#if MQTT_DEBUG
	LREP(__FUNCTION__, "Free memory: %d bytes", esp_get_free_heap_size());
	LREP(__FUNCTION__, "publish return %d", publishRet);
#endif
}

/*
 *
 */
void app_mqtt_endpoint_link_transmit(const char* strDesId,const char* message)
{
	if(!g_bMqttstatus) return;
	app_mqtt_endpoint_link_get_topic(strPublishTopic_Endpoint_Link, strDesId);
#if MQTT_DEBUG
	LREP_WARNING(__FUNCTION__, "PUBLISH TOPIC %s",strPublishTopic_Endpoint_Link);
#endif
	esp_mqtt_client_publish(client, strPublishTopic_Endpoint_Link, message, strlen(message), MQTT_PUB_QOS, 0);
	LREP_WARNING(__FUNCTION__, "%s",message);
}

esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
	static bool bFirstConnect = true;
	client = event->client;
    int iMsgId;
    switch (event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
#if MQTT_DEBUG
        	LREP(__FUNCTION__, "MQTT_EVENT_CONNECTED ");
#endif
        	if (g_bMqttPause == false) {
            iMsgId = esp_mqtt_client_subscribe(client, strSubscribeTopic, MQTT_SUB_QOS);
#if MQTT_DEBUG
            LREP(__FUNCTION__, "sent subscribe successful, iMsgId = %d", iMsgId);
#endif
            iMsgId = esp_mqtt_client_subscribe(client, strSubscribeTopic_Endpoint_Link, MQTT_SUB_QOS);
#if MQTT_DEBUG
            LREP(__FUNCTION__, "sent subscribe successful, iMsgId = %d", iMsgId);
#endif
        	}
            g_bMqttstatus = true;
            if (bFirstConnect) {
            	g_bRequestGetData = true;
            	bFirstConnect = false;
            }
            break;
        case MQTT_EVENT_DISCONNECTED:
#if MQTT_DEBUG
        		LREP_ERROR(__FUNCTION__, "MQTT_EVENT_DISCONNECTED");
#endif
        	if(g_bMqttstatus) {
        		g_bMqttstatus = false;
        	}
            break;
        case MQTT_EVENT_SUBSCRIBED:
#if 0
        	LREP(__FUNCTION__, "MQTT_EVENT_SUBSCRIBED, iMsgId = %d", event->msg_id);
#endif
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
#if 0
        	LREP(__FUNCTION__, "MQTT_EVENT_UNSUBSCRIBED, iMsgId = %d", event->msg_id);
#endif
            break;
        case MQTT_EVENT_PUBLISHED:
#if 0
        	LREP(__FUNCTION__, "MQTT_EVENT_PUBLISHED, iMsgId = %d", event->msg_id);
#endif
            break;
        case MQTT_EVENT_DATA:
#if 0
        	LREP(__FUNCTION__, "MQTT_EVENT_DATA ");
#endif
			memset(strRxMessage, 0, MQTT_MAXIMUM_MESSAGE_LENGTH);
			memset(strRxTopic, 0, MQTT_MAXIMUM_TOPIC_LENGTH);
			if(event->data_len > (MQTT_MAXIMUM_MESSAGE_LENGTH-1) || event->topic_len > (MQTT_MAXIMUM_TOPIC_LENGTH-1)) break;
			memcpy(strRxTopic, event->topic, event->topic_len);
			memcpy(strRxMessage, event->data, event->data_len);
			if (esp_get_free_heap_size() < 40000) LREP_WARNING(__FUNCTION__, "Free memory: %d bytes", esp_get_free_heap_size());
			mqtt_message_receive_handle(strRxTopic, strRxMessage);
            break;
        case MQTT_EVENT_ERROR:
#if MQTT_DEBUG
        	LREP_ERROR(__FUNCTION__, "MQTT_EVENT_ERROR");
        	LREP_WARNING(__FUNCTION__, "free Heap:%d,%d", esp_get_free_heap_size(), heap_caps_get_free_size(MALLOC_CAP_8BIT));
#endif
            break;
        default:
#if 0
        	LREP(__FUNCTION__, "Other event id:%d", event->event_id);
#endif
            break;
    }
    return ESP_OK;
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
#if MQTT_DEBUG
    ESP_LOGD(__FUNCTION__, "[app_mqtt][mqtt_event_handler]: Event dispatched from event loop base = %s, event_id = %d",
																			base, event_id);
#endif
    mqtt_event_handler_cb(event_data);
}

esp_err_t mqtt_message_receive_handle(char* strTopic, char* strMessage) {
#if MQTT_DEBUG
	LREP(__FUNCTION__,"topic: %s", strTopic);
	LREP(__FUNCTION__, "message %s", strMessage);
#endif
	app_mqtt_reset_subscribed_topic();
	if(strcmp(strTopic,strSubscribeTopic_Endpoint_Link)==0)
	{
#if MQTT_DEBUG
		LREP_WARNING(__FUNCTION__,"Eplink receive message %s",strMessage);
#endif
		app_cmd_handle_buffer(strMessage, strlen(strMessage), CMD_TYPE_MQTT_EPLINK);
	}
	else if (xQueueSend(mqttRxQueue, strMessage,  10 / portTICK_RATE_MS) != pdTRUE) {
		LREP_ERROR(__FUNCTION__, "xQueue send failed");
	}
	return ESP_OK;
}

void app_mqtt_reset_subscribed_topic(void) {
	if(!g_bMqttstatus) return;
	LREP(__FUNCTION__, "Reset topic");
	esp_mqtt_client_unsubscribe(client, strSubscribeTopic);
	esp_mqtt_client_subscribe(client, strSubscribeTopic, MQTT_SUB_QOS);
	esp_mqtt_client_unsubscribe(client, strSubscribeTopic_Endpoint_Link);
	esp_mqtt_client_subscribe(client, strSubscribeTopic_Endpoint_Link, MQTT_SUB_QOS);
	g_LastestMessageTimestamp = app_time_server_get_timestamp_now();
}

void task_mqtt_rx(void *pvParameter) {
	static char rxMessage[MQTT_MAXIMUM_MESSAGE_LENGTH] = {0};
	for(;;) {
		g_bRun_MqttTask = true;
		if(xQueueReceive(mqttRxQueue, rxMessage, (portTickType)portMAX_DELAY) == pdTRUE) {
			app_cmd_handle_buffer(rxMessage, strlen(rxMessage), CMD_TYPE_MQTT);
			memset(rxMessage, 0, MQTT_MAXIMUM_MESSAGE_LENGTH);
		}
		else vTaskDelay(10/portTICK_PERIOD_MS);
	}
}

void app_mqtt_endpoint_link_get_topic(char* topicBuffer, const char* strDeviceId) {
	memset(topicBuffer, 0, MQTT_MAXIMUM_TOPIC_LENGTH);
	strcpy(topicBuffer,MQTT_TOPIC_SUB_ENDPOINT_LINK);
	strcat(topicBuffer, strDeviceId);
}

void app_mqtt_delete_cloud_info(void) {
	app_filesystem_delete_cloud_info();
	memset(g_CloudInfo.pBrokerUrl, 0, sizeof(strBrokerUrl));
	memset(g_CloudInfo.pBrokerUsr, 0, sizeof(strBrokerUser));
	memset(g_CloudInfo.pBrokerPass, 0, sizeof(strBrokerPass));
	memset(g_CloudInfo.pForceOtaUrl, 0, sizeof(strForceOtaUrl));
	memset(g_CloudInfo.pTopicPub, 0, sizeof(strPublishTopic));
	memset(g_CloudInfo.pTopicSub, 0 , sizeof(strSubscribeTopic));
	memset(g_CloudInfo.pTopicAllert, 0, sizeof(strAllertTopic));
}

bool app_mqtt_check_cloud_setting(void) {
	if (strlen(g_CloudInfo.pBrokerUrl) > 0) return true;
	return false;
}
