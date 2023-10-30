#include "app_main.h"
#include "app_button.h"
#include "app_wifi_mode.h"
#include "app_mqtt.h"
#include "app_time_server.h"
#include "config.h"
#include "app_schedule.h"
#include "app_ota.h"
#include "app_udp.h"
#include "app_gpio.h"
#include "app_endpoint_link.h"
#include "app_cmd_mgr.h"
#include "app_led_rgb.h"
#include "app_ble.h"
#include "app_ble_mesh.h"

#include "stdbool.h"

bool g_bRequestGetData = false;
bool g_bRequestWifiConfig = false;
bool g_bRequestOta = false;
bool g_bRequestForceOtaAuto = false;

bool g_bRequestSmartconfig = false;
bool g_bRequestSoftap = false;
bool g_bRequestBluFi = false;
bool g_bRequestExitConfig = false;
bool g_bRequestBleMeshProv = false;

uint64_t g_MainTime = 0;

void nvs_init();

void app_main_wifi(void);
void app_main_ble_mesh(void);

void app_main_wifi_req(void);

static TaskHandle_t xBeCallTaskHandle = NULL;
static void task_be_call(void* pvParameters);

bool g_bRun_MainTask = false;
bool g_bRun_ButtonTask = false;
bool g_bRun_RelayTask = false;
bool g_bRun_BuzzerTask = false;
bool g_bRun_GpioTask = false;
bool g_bRun_MqttTask = false;
bool g_bRun_UdpTask = false;
bool g_bRun_SkdTask = false;

TaskHandle_t xResetHandle = NULL;
static void task_reset(void *pvParameter);

static int xCount100ms = 0;
static int xCount1min = 0;
static char strTimeInfo[64] = {0};
static struct tm m_sTimeInfo;
static int otaResetCount = 0;
static int wifiResetCount = 0;

void app_main(void)
{
	ESP_LOGI(__FUNCTION__,"Hello, there is no sweeter innocence than our gentle sin");
	LREP(__FUNCTION__, "[APP] Startup..");
	LREP(__FUNCTION__, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
	LREP(__FUNCTION__, "[APP] IDF version: %s", esp_get_idf_version());

	nvs_init();
	app_ota_running_partition_check();
	app_spiffs_init();
	gpio_init();
	button_init();

	if (app_is_wifi_mode()) {
		LREP_WARNING(__FUNCTION__, "WIFI mode");
		app_main_wifi();
	}
	else {
		LREP_WARNING(__FUNCTION__, "BLE MESH mode");
		app_main_ble_mesh();
	}
}

void app_main_wifi(void) {
	app_wifi_init();

	app_skd_init();
	app_force_ota_init();

	app_mqtt_start();
	app_udp_start_server();
	app_time_server_init();
	app_endpoint_link_init();

	if (app_mqtt_check_cloud_setting() == false) {
		app_ble_blufi_begin(); app_ble_blufi_unlimited();
	}
	xTaskCreate(&task_reset,"task_reset",2048, NULL, 3, &xResetHandle);
	xTaskCreate(&task_be_call, "task_be_call", 8192, NULL, 5, &xBeCallTaskHandle);
	while (1) { /*main loop*/
		g_bRun_MainTask = true;
		g_MainTime = app_time_server_get_timestamp_now();
		gpio_update_night();

		app_main_wifi_req();

		if (xCount100ms >= 600) { // 60s
			xCount100ms = 0;
			app_cmd_GetDataResend();
			xCount1min++;
			m_sTimeInfo = *app_time_server_get_timetm(g_MainTime, 7);
			strftime(strTimeInfo, sizeof(strTimeInfo), "%c", &m_sTimeInfo);
			LREP(__FUNCTION__, "The current date/time in Viet Nam is: %s", strTimeInfo);
			LREP_WARNING(__FUNCTION__, "Free memory: %d bytes", esp_get_free_heap_size());
//			LREP(__FUNCTION__, "Night %s", g_NightControl.bNight ? "TRUE" : "FALSE");

			if(g_otaRunning) {
				if(otaResetCount++>=4) {
					LREP_ERROR(__FUNCTION__, "OTA failed, force stop OTA");
					esp_restart();
				}
			} else otaResetCount = 0;

			if ((xCount1min  % 30) == 0 && g_espGotIp == false && bSoftApOver == true) {
				wifiResetCount++;
				LREP_ERROR(__FUNCTION__, "wifi not connect, refresh wifi %d", wifiResetCount);
				app_wifi_refresh();
				if (wifiResetCount > 180 && app_gpio_is_idle()) esp_restart();
			}
			else if (g_espGotIp && wifiResetCount > 0) {
				wifiResetCount--;
			}

			if (xCount1min % 10 == 0 ) { //10 min
				app_force_ota_process();
			}
		}

		app_gpio_config_flash_handle();
		app_skd_flash_handle();
		app_endpoint_link_flash_handle();

		xCount100ms++;
		delay_ms(100);
	} /*main loop*/
}

void app_main_wifi_req(void) {
	if (g_bRequestGetData) g_bRequestGetData = false, app_cmd_GetData();

	if  (g_bRequestBluFi) g_bRequestBluFi = false, app_ble_blufi_begin();
	else if (g_bRequestSoftap) g_bRequestSoftap = false, app_wifi_softap();

	if (g_bRequestOta) g_bRequestOta = false, app_ota_start();

	if (g_bRequestForceOtaAuto)g_bRequestForceOtaAuto = false, app_force_ota_start();
}

void app_main_ble_mesh(void) {
	app_ble_mesh_init();

	while (1) {
		g_MainTime = app_time_server_get_timestamp_now();
		if (g_bRequestBleMeshProv) {
			app_ble_mesh_prov_begin();
			g_bRequestBleMeshProv = false;
		}

//		LREP_WARNING(__FUNCTION__, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
		delay_ms(200);
	}
}


void nvs_init() {
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
	  ESP_ERROR_CHECK(nvs_flash_erase());
	  ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}

void task_be_call(void *pvParameters) {
	delay_ms(30000);

	if (!app_time_server_sync()) goto EXIT_TASK;
	if (!g_blufiOver) goto EXIT_TASK;
	if (!bSoftApOver) goto EXIT_TASK;
	if (strlen(g_CloudInfo.pForceOtaUrl) == 0 || strlen(g_CloudInfo.pBeSharedKey) == 0) goto EXIT_TASK;
	while (1) {
		app_mqtt_disconnect();
		app_cmd_send_check_force_ota();
		break;
	}
	app_mqtt_reconnect(60);
	EXIT_TASK:
	TASK_DELETE_SAFE(xBeCallTaskHandle);
}

void task_reset(void *pvParameter) {
	delay_ms(10000);
	bool bReset;
	while(1) {
		bReset = false;
		delay_ms(60000);
		if (g_bRun_MainTask == false) {
			LREP_ERROR(__FUNCTION__, "Main task blocked");
			bReset = true;
		}
		if (g_bRun_ButtonTask == false) {
			LREP_ERROR(__FUNCTION__, "Button task blocked");
			bReset = true;
		}
		if (g_bRun_RelayTask == false) {
			LREP_ERROR(__FUNCTION__, "Relay task blocked");
			bReset = true;
		}
		if (g_bRun_BuzzerTask == false) {
			LREP_ERROR(__FUNCTION__, "Buzzer task blocked");
			bReset = true;
		}
		if (g_bRun_GpioTask == false) {
			LREP_ERROR(__FUNCTION__, "Gpio task blocked");
			bReset = true;
		}
		if (g_bRun_SkdTask == false) {
			LREP_ERROR(__FUNCTION__, "Schedule task blocked");
			bReset = true;
		}

		if (bReset) {
			esp_restart();
		}
		else {
			g_bRun_MainTask = false;
			g_bRun_ButtonTask = false;
			g_bRun_RelayTask = false;
			g_bRun_BuzzerTask = false;
			g_bRun_GpioTask = false;
			g_bRun_SkdTask = false;
		}
	}
}
