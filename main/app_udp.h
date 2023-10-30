/*
 * app_udp.h
 *
 *  Created on: Mar 18, 2020
 *      Author: laian
 */

#ifndef __APP_UDP_H
#define __APP_UDP_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "ESP_LOG.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "config.h"
#include "app_wifi_mode.h"
//#include "app_control_json.h"

#define APP_UDP_INPUT_PORT		5000
#define APP_UDP_MAX_MSG_LEN 	MAX_MESSAGE_LENGTH
#define APP_UDP_MAX_ADDR_LEN	64
#define APP_UDP_BROADCAST_IP	"255.255.255.255"
#define APP_UDP_BROADCAST_PORT	5001

void app_udp_start_server();

esp_err_t app_udp_client_send_broadcast(const char* message);
esp_err_t app_udp_client_send_unicast(const char* message);
esp_err_t app_udp_client_send(const char* str_ip, int port, const char* message);

esp_err_t app_udp_client_send_buffer_broadcast(const char* message, int msgLen);
esp_err_t app_udp_client_send_buffer_unicast(const char* message, int msgLen);
esp_err_t app_udp_client_send_buffer(const char* str_ip, int port, const char* message, int msgLen);

esp_err_t app_udp_client_send_broadcast_endpoint_link(const char* message);
#endif /* __APP_UDP_H */
