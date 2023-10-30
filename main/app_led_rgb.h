/*
 * app_led_rgb.h
 *
 *  Created on: 29 Oct 2020
 *      Author: tongv
 */

#ifndef SMART_SWITCH_MAIN_APP_LED_RGB_H_
#define SMART_SWITCH_MAIN_APP_LED_RGB_H_
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "led_strip.h"


#define LED1_PIN			   			23
#define LED2_PIN			   			17
#define LED3_PIN			   			27
#define LED4_PIN			   			14

#define APP_LED_CHANNEL_1			   	1
#define APP_LED_CHANNEL_2			   	2
#define APP_LED_CHANNEL_3			   	3
#define APP_LED_CHANNEL_4			   	4

#define APP_LED_MAX_CHANNEL				4

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} sRgbColor_t;

typedef struct {
	int H; /*0-360*/
	int S; /*0-100*/
	int L; /*0-100*/
} sHSL_Color_t;

void ws2812_rmt_init(const gpio_num_t gpio,const rmt_channel_t channel,const uint8_t mem_block_num,int tx_duty);
void ws2812_rmt_install_ws2812_driver();
void app_led_init();
void app_led_bt_ctl_all(sRgbColor_t color);
void app_led_bt_ctl(int button, sRgbColor_t color);

bool app_led_rgb_cf_eq(sRgbColor_t colorA, sRgbColor_t colorB); //compare equal
bool app_led_rgb_cf_neq(sRgbColor_t colorA, sRgbColor_t colorB); //compare not equal

sRgbColor_t app_led_rgb_convert_color_u32_to_rgb(uint32_t u32Color);
uint32_t app_led_rgb_convert_color_rgb_to_u32(sRgbColor_t color);

sHSL_Color_t rgb_to_hsl(sRgbColor_t rgb_color);
sRgbColor_t hsl_to_rgb(sHSL_Color_t hsl_color);
float hue_to_rgb(float v1, float v2, float  vH);

sRgbColor_t app_led_rgb(uint32_t u32Color, int lightness);

#define RGB_COLOR(__R,__G,__B) 		(sRgbColor_t){.red = __R, .green = __G, .blue = __B}

#define RGB_COLOR_BLACK				RGB_COLOR(0,0,0)
#define RGB_COLOR_WHITE				RGB_COLOR(255,255,255)
#define RGB_COLOR_SILVER			RGB_COLOR(192,192,192)
#define RGB_COLOR_GRAY				RGB_COLOR(128,128,128)

#define RGB_COLOR_RED				RGB_COLOR(255,0,0)
#define RGB_COLOR_LIME				RGB_COLOR(0,255,0)
#define RGB_COLOR_BLUE				RGB_COLOR(0,0,255)
#define RGB_COLOR_YELLOW			RGB_COLOR(255,255,0)
#define RGB_COLOR_GREEN				RGB_COLOR(0,128,0)
#define RGB_COLOR_PURPLE			RGB_COLOR(128,0,128)

#define RGB_COLOR_CYAN				RGB_COLOR(0,255,255)
#define RGB_COLOR_MAGENTA			RGB_COLOR(255,0,255)
#define RGB_COLOR_MAROON			RGB_COLOR(128,0,0)
#define RGB_COLOR_OLIVE				RGB_COLOR(128,128,0)
#define RGB_COLOR_TEAL				RGB_COLOR(0,128,128)
#define RGB_COLOR_NAVY				RGB_COLOR(0,0,128)

#define RGB_COLOR_FOREST_GREEN		RGB_COLOR(34,139,34)
#define RGB_COLOR_LIME_GREEN		RGB_COLOR(50,205,50)

#endif /* SMART_SWITCH_MAIN_APP_LED_RGB_H_ */
