/*
 * app_gpio.h
 *
 *  Created on: 7 Feb 2020
 *      Author: tongv
 */

#ifndef MAIN_APP_GPIO_H_
#define MAIN_APP_GPIO_H_
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "esp32/rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/timer.h"
#include "driver/periph_ctrl.h"
#include "esp_log.h"
#include "config.h"
#include "app_led_rgb.h"

#define SW_CHANNEL_1 		1
#define SW_CHANNEL_2		2
#define SW_CHANNEL_3		3
#define SW_CHANNEL_4		4

typedef enum __SWITCH_MODE {
	SW_MODE_ON_OFF = 1,
	SW_MODE_COUNT_DOWN = 2,
} sw_mode_t;

typedef enum __SWITCH_CONTROL_MODE {
	SW_CTRL_MODE_NORMAL = 1,
	SW_CTRL_MODE_DISABLE = 2,
	SW_CTRL_MODE_DISABLE_TOUCH = 3,
	SW_CTRL_MODE_DISABLE_REMOTE = 4,
} sw_control_mode_t;

typedef enum __LED_MODE {
	LED_MODE_NORMAL = 0,
	LED_MODE_AUTO = 1,
	LED_MODE_MANUAL = 2,
	LED_MODE_BLINK,
} sw_led_mode_t;

typedef struct {
	int iBuzzerEnb;
	int iLedEnb;
	uint32_t u32RgbOn;
	uint32_t u32RgbOff;
	/**/
	int channel_1_mode;
	int channel_2_mode;
	int channel_3_mode;
	int channel_4_mode;
	int channel_1_countdown;
	int channel_2_countdown;
	int channel_3_countdown;
	int channel_4_countdown;
	/**/
	int curtain_cycle;
	int curtain_2_cycle;
	/**/
	int channel_1_control_mode;
	int channel_2_control_mode;
	int channel_3_control_mode;
	int channel_4_control_mode;
	/**/
	int channel_1_led_off;
	uint32_t channel_1_rgb_on;
	uint32_t channel_1_rgb_off;
	int channel_2_led_off;
	uint32_t channel_2_rgb_on;
	uint32_t channel_2_rgb_off;
	int channel_3_led_off;
	uint32_t channel_3_rgb_on;
	uint32_t channel_3_rgb_off;
	int channel_4_led_off;
	uint32_t channel_4_rgb_on;
	uint32_t channel_4_rgb_off;
	/**/
	int iNightmodeEnb;
	uint32_t u32NightBegin;
	uint32_t u32NightEnd;
//	float fNightTimezone;
	int iNightTimezone;
	/**/
	int channel_1_lightness;
	int channel_2_lightness;
	int channel_3_lightness;
	int channel_4_lightness;
	int led_lighness;
	/**/
	int saveStt;
} sSwRgbConfig_t;

typedef struct {
	int iRelay;
	int iStatus;
	int iStatusReq;
	int iMode;
	int iCountDown;
	uint64_t u64Ontime;
	bool bfirstRun;
	bool bEplinkReq;
	bool bTouchDisable;
	bool bRemoteDisable;
} sChannelControl_t;

typedef struct {
	sw_led_mode_t 	ledMode;
	sRgbColor_t 	blinkColor;
	uint64_t		impact[5];
	uint8_t 		blink[5];
	bool 			stt[5];
	bool 			sttRqs[5];
	int 			iCount;
	sRgbColor_t		onColor[5];
	sRgbColor_t 	offColor[5];
	bool			disable[5];
} sRgbLedControl_t;

typedef struct {
	struct tm tm_begin;
	struct tm tm_end;
	bool bNight;
} sNightModeControl_t;

extern bool m_bBuzzerStatusRqs;

extern sSwRgbConfig_t g_Config;
extern sSwRgbConfig_t g_ConfigSaved;

extern sChannelControl_t g_ChannelControl[5];

extern sNightModeControl_t g_NightControl;

void timer_tg0_initialise (int timer_period_us);
void gpio_init();
void buzzer();
void buzzer_config();

bool app_touch_change_channel_status(uint8_t channel);
bool app_remote_change_channel_status(uint8_t channel, int status);
void app_remote_eplink_change_channel_status(uint8_t channel, int status);

void gpio_scan_channel(uint8_t channel);
void gpio_scan_leds();

void gpio_set_led_color(int channel, int on_off, int color);
void gpio_set_led_lightness(int channel, int lightness);
void gpio_set_original_config(sSwRgbConfig_t* pConfig);
void gpio_copy_config(sSwRgbConfig_t* pConfigOut, const sSwRgbConfig_t* pConfigIn);
bool gpio_config_compare(const sSwRgbConfig_t* pConfigX, const sSwRgbConfig_t* pConfigY);
void app_gpio_config_flash_handle(void);
void gpio_set_channel_config(void);
void gpio_set_led_config(void);

void gpio_set_led_auto(void);
void gpio_set_led_manual(void);
void gpio_set_blufi_connected(void);
void gpio_set_led_normal(void);
void gpio_disable_led_channel(int channel);
void gpio_enable_led_channel(int channel);
void gpio_set_channel_type(int channel, int type);
void gpio_set_channel_countdown(int channel, int countdown);
void gpio_set_channel_ctrl_mode(int channel, int ctr_mode);

void gpio_disable_nightmode(void);
void gpio_enable_nightmode(void);
void gpio_set_night_begin(uint32_t time_begin);
void gpio_set_night_end(uint32_t time_end);
void gpio_set_night_timezone(int timezone);

void gpio_set_night_config(void);
void gpio_update_night(void);

bool app_is_wifi_mode(void);
bool app_is_connected(void);
bool app_gpio_is_idle(void);

void app_relay_ctrl(int relay, int onoff);

#define RGB_ON_DEFAULT 		RGB_COLOR(0xFF, 0x4C, 0x00)			//RGB_COLOR_MAROON
#define RGB_OFF_DEFAULT		RGB_COLOR(0x18, 0xE4, 0x72)			//RGB_COLOR_NAVY
#define RGB_BLUFI_CONNECT	RGB_COLOR_NAVY

#define COLOR_ON_DEFAULT 	app_led_rgb_convert_color_rgb_to_u32(RGB_ON_DEFAULT)
#define COLOR_OFF_DEFAULT 	app_led_rgb_convert_color_rgb_to_u32(RGB_OFF_DEFAULT)

#define LIGHTNESS_DEFAULT 	100

#define NIGHT_TZ_DEFAULT 		7
#define NIGHT_BEGIN_DEFAULT 	1638183600	//6:00 pm
#define NIGHT_END_DEFAULT		1638140400	//6:00 am
#endif /* MAIN_APP_GPIO_H_ */
