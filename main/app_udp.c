/*
 * app_udp.c
 *
 *  Created on: Mar 18, 2020
 *      Author: laian
 */

#include "app_udp.h"
#include "app_cmd_mgr.h"

static TaskHandle_t  xUdpTaskHandle = NULL;
static char app_udp_rx_buffer[APP_UDP_MAX_MSG_LEN] = {0};
static char app_udp_client_addr[APP_UDP_MAX_ADDR_LEN] = {0};
static uint16_t app_udp_client_port = 5001;
static void task_udp(void* param);

void app_udp_start_server() {
	xTaskCreate(task_udp, "task_udp", 4096, NULL, 3, &xUdpTaskHandle);
}

static void task_udp(void* param) {
	for(;;) {
		while(!app_wifi_udp_ready()) {
			vTaskDelay(100 / portTICK_PERIOD_MS);
		}

		struct sockaddr_in serverSockAddr = {
			.sin_addr.s_addr = htonl(INADDR_ANY),
			.sin_family = AF_INET,
			.sin_port = htons(APP_UDP_INPUT_PORT),
		};

		int serverSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
		if (serverSock < 0) {
			LREP_ERROR(__FUNCTION__, "Unable to create socket: errno %d", errno);
			break;
		}
		LREP(__FUNCTION__,"Socket created");

		int err = bind(serverSock, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr));
        if (err < 0) {
            LREP_ERROR(__FUNCTION__, "Socket unable to bind: errno %d", errno);
            break;
        }
        LREP(__FUNCTION__, "Socket binded");

        while(1) {
        	LREP(__FUNCTION__, "Waiting for data");
            struct sockaddr_in6 sourceAddr;
            socklen_t socklen = sizeof(sourceAddr);
            int len = recvfrom(serverSock, app_udp_rx_buffer, APP_UDP_MAX_MSG_LEN - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);
        	if(len < 0) {
                LREP_ERROR(__FUNCTION__, "recvfrom failed: errno %d", errno);
                break;
        	}
        	else {
        		memset(app_udp_client_addr, 0, APP_UDP_MAX_ADDR_LEN);
        		if (sourceAddr.sin6_family == PF_INET) {
					inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, app_udp_client_addr, APP_UDP_MAX_ADDR_LEN - 1);
				} else if (sourceAddr.sin6_family == PF_INET6) {
					inet6_ntoa_r(sourceAddr.sin6_addr, app_udp_client_addr, APP_UDP_MAX_ADDR_LEN - 1);
				}
        		app_udp_client_port = ntohs(sourceAddr.sin6_port);
        		LREP(__FUNCTION__, "Received %d bytes from %s:%d", len, app_udp_client_addr, app_udp_client_port);
        		app_cmd_handle_buffer(app_udp_rx_buffer, len, CMD_TYPE_UDP);
                memset(app_udp_rx_buffer, 0, APP_UDP_MAX_MSG_LEN);
        	}
        }

        if (serverSock != -1) {
            LREP_ERROR(__FUNCTION__, "Shutting down socket and restarting...");
            shutdown(serverSock, 0);
            close(serverSock);
        }
	}
	TASK_DELETE_SAFE(xUdpTaskHandle);
}

esp_err_t app_udp_client_send_broadcast(const char* message) {
	return app_udp_client_send(app_wifi_get_broadcast_ip(), APP_UDP_BROADCAST_PORT, message);
}

esp_err_t app_udp_client_send_broadcast_endpoint_link(const char* message)
{
	return app_udp_client_send(app_wifi_get_broadcast_ip(), APP_UDP_INPUT_PORT, message);
}

esp_err_t app_udp_client_send_unicast(const char* message) {
	return app_udp_client_send(app_udp_client_addr, app_udp_client_port, message);
}

esp_err_t app_udp_client_send(const char* str_ip, int port, const char* message) {
	if(!app_wifi_udp_ready()) return ESP_FAIL;

	esp_err_t ret = ESP_FAIL;

    if (app_udp_client_send_buffer(str_ip, port, message, strlen(message)) == ESP_OK) {
    	ret = ESP_OK;
    }
    return ret;
}

esp_err_t app_udp_client_send_buffer_broadcast(const char* message, int msgLen) {
	return app_udp_client_send_buffer(app_wifi_get_broadcast_ip(), APP_UDP_BROADCAST_PORT, message, msgLen);
}

esp_err_t app_udp_client_send_buffer_unicast(const char* message, int msgLen) {
	return app_udp_client_send_buffer(app_udp_client_addr, app_udp_client_port, message, msgLen);
}

esp_err_t app_udp_client_send_buffer(const char* str_ip, int port, const char* message, int msgLen) {
	if(!app_wifi_udp_ready()) return ESP_FAIL;
	struct sockaddr_in dest_addr = {
		.sin_addr.s_addr = inet_addr(str_ip),
		.sin_family = AF_INET,
		.sin_port = htons(port),
	};

	int clientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	esp_err_t ret = ESP_FAIL;

	do {
		if (clientSock < 0) {
			LREP_ERROR(__FUNCTION__, "Unable to create socket: errno %d", errno);
			break;
		}
		LREP(__FUNCTION__, "Socket created, sending to %s:%d buffer %d bytes", str_ip, port, msgLen);

		int err = sendto(clientSock, message, msgLen, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (err < 0) {
			LREP_ERROR(__FUNCTION__, "Error occurred during sending: errno %d", errno);
			break;
		}

		ret = ESP_OK;
	} while(false);

	if (clientSock != -1) {
		shutdown(clientSock, 0);
		close(clientSock);
	}
	return ret;
}

