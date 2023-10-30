/*
 * app_led_rgb.c
 *
 *  Created on: 29 Oct 2020
 *      Author: tongv
 */
#include "app_led_rgb.h"
#include "app_common.h"
#include "config.h"

rmt_config_t config;

led_strip_t *strip1 = NULL;
led_strip_t *strip2 = NULL;
led_strip_t *strip3 = NULL;
led_strip_t *strip4 = NULL;

led_strip_t** g_pStrip[APP_LED_MAX_CHANNEL] = {&strip1, &strip2, &strip3, &strip4};

int m_led_r,m_led_g, m_led_b;

sRgbColor_t g_ColorButton1 = {.red = 0, .green = 0, .blue = 0};
sRgbColor_t g_ColorButton2 = {.red = 0, .green = 0, .blue = 0};
sRgbColor_t g_ColorButton3 = {.red = 0, .green = 0, .blue = 0};
sRgbColor_t g_ColorButton4 = {.red = 0, .green = 0, .blue = 0};

/*
 *
 */
void ws2812_rmt_init(const gpio_num_t gpio,const rmt_channel_t channel,const uint8_t mem_block_num,int tx_duty)
{
	  config.rmt_mode = RMT_MODE_TX;
	  config.channel = channel;
	  config.gpio_num = gpio;
	  config.mem_block_num = mem_block_num;
	  config.clk_div = 3;

	  config.tx_config.loop_en = false;
	  config.tx_config.carrier_en = false;
	  config.tx_config.idle_output_en = true;
	  config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
	  config.tx_config.carrier_duty_percent = tx_duty;

	  rmt_config(&config);
	  rmt_driver_install(config.channel, 0, 0);
}
/*
 *
 */
void ws2812_rmt_install_ws2812_driver()
{
	config.channel=RMT_CHANNEL_1;
	led_strip_config_t strip_config1 = LED_STRIP_DEFAULT_CONFIG(24, (led_strip_dev_t)config.channel);
	strip1 = led_strip_new_rmt_ws2812(&strip_config1);
	if (!strip1) {
		LREP_ERROR(__FUNCTION__, "install WS2812 driver failed 1");
		return;
	}

	//
	config.channel=RMT_CHANNEL_2;
	led_strip_config_t strip_config2 = LED_STRIP_DEFAULT_CONFIG(24, (led_strip_dev_t)config.channel);
	strip2 = led_strip_new_rmt_ws2812(&strip_config2);
	if (!strip2) {
		LREP_ERROR(__FUNCTION__, "install WS2812 driver failed 2");
		return;
	}
	//
	config.channel=RMT_CHANNEL_3;
	led_strip_config_t strip_config3 = LED_STRIP_DEFAULT_CONFIG(24, (led_strip_dev_t)config.channel);
	strip3 = led_strip_new_rmt_ws2812(&strip_config3);
	if (!strip3) {
		LREP_ERROR(__FUNCTION__, "install WS2812 driver failed 3");
		return;
	}

	//
	config.channel=RMT_CHANNEL_4;
	led_strip_config_t strip_config4 = LED_STRIP_DEFAULT_CONFIG(24, (led_strip_dev_t)config.channel);
	strip4 = led_strip_new_rmt_ws2812(&strip_config4);
	if (!strip4) {
		LREP_ERROR(__FUNCTION__, "install WS2812 driver failed 4");
		return;
	}
}

/*
 *
 */
void app_led_init()
{
	ws2812_rmt_init(LED1_PIN,APP_LED_CHANNEL_1,1,2);
	ws2812_rmt_init(LED2_PIN,APP_LED_CHANNEL_2,1,2);
	ws2812_rmt_init(LED3_PIN,APP_LED_CHANNEL_3,1,99);
	ws2812_rmt_init(LED4_PIN,APP_LED_CHANNEL_4,1,99);
	ws2812_rmt_install_ws2812_driver();
}
/*
 *
 */
void app_led_bt_ctl_all(sRgbColor_t color)
{
	static bool firstUpdate = true;
#ifdef DEVICE_TYPE
	bool update = app_led_rgb_cf_neq(color, g_ColorButton1) || app_led_rgb_cf_neq(color, g_ColorButton2) ||
			app_led_rgb_cf_neq(color, g_ColorButton3) || app_led_rgb_cf_neq(color, g_ColorButton4);
	if (update || firstUpdate) {
		strip1->clear(strip1, 100);
		strip2->clear(strip2, 100);
		strip3->clear(strip3, 100);
//		strip4->clear(strip4, 100);

		vTaskDelay(pdMS_TO_TICKS(10));

		strip1->set_pixel(strip1, 0, color.red, color.green, color.blue);
		strip2->set_pixel(strip2, 0, color.red, color.green, color.blue);
		strip3->set_pixel(strip3, 0, color.red, color.green, color.blue);
//		strip4->set_pixel(strip4, 0, color.red, color.green, color.blue);

		strip1->refresh(strip1, 100);
		strip2->refresh(strip2, 100);
		strip3->refresh(strip3, 100);
//		strip4->refresh(strip4, 100);

		g_ColorButton1 = color; g_ColorButton2 = color, g_ColorButton3 = color;
		firstUpdate = false;
	}
#endif
}
/*
 *
 */
void app_led_bt_ctl(int btn ,sRgbColor_t color)
{
	if (btn > APP_LED_MAX_CHANNEL || btn <=0) return;

	led_strip_t* pStrip = *g_pStrip[btn-1];
	sRgbColor_t* pColorButton = (btn == 1) ? (&g_ColorButton1) : (btn == 2) ? (&g_ColorButton2) : (btn == 3) ? (&g_ColorButton3) : (&g_ColorButton4);

//	bool update = app_led_rgb_cf_neq(color, *pColorButton);
//	if (!update) return;

//	pStrip->clear(pStrip, 100);
//	vTaskDelay(pdMS_TO_TICKS(10));
	pStrip->set_pixel(pStrip, 0, color.red, color.green, color.blue);
	pStrip->refresh(pStrip, 100);

	*pColorButton = color;
}

bool app_led_rgb_cf_eq(sRgbColor_t colorA, sRgbColor_t colorB) //compare equal
{
	if (colorA.blue != colorB.blue || colorA.green != colorB.green || colorA.red != colorB.red) return false;
	return true;
}

bool app_led_rgb_cf_neq(sRgbColor_t colorA, sRgbColor_t colorB) //compare not equal
{
	if (colorA.blue != colorB.blue || colorA.green != colorB.green || colorA.red != colorB.red) return true;
	return false;
}

sRgbColor_t app_led_rgb_convert_color_u32_to_rgb(uint32_t u32Color) {
	sRgbColor_t color = {
		.red = (u32Color >> 16) & 0xFF,
		.green = (u32Color>>8) & 0xFF,
		.blue = u32Color & 0xFF
	};
	return color;
}

uint32_t app_led_rgb_convert_color_rgb_to_u32(sRgbColor_t color) {
	uint32_t u32Color = 0;
	u32Color = color.red; u32Color <<= 8;
	u32Color += color.green; u32Color <<= 8;
	u32Color += color.blue;
	return u32Color;
}

#define MAX(__X, __Y) ((__X > __Y) ? __X : __Y)
#define MIN(__X, __Y) ((__X < __Y) ? __X : __Y)

sHSL_Color_t rgb_to_hsl(sRgbColor_t rgb_color) {
	sHSL_Color_t hsl_color = {.H = 0, .S = 0, .L = 0};
	float f_R = (float)rgb_color.red / 255.0;
	float f_G = (float)rgb_color.green / 255.0;
	float f_B = (float)rgb_color.blue / 255.0;

	float C_Max = MAX(MAX(f_R, f_G), f_B);
	float C_Min = MIN(MIN(f_R, f_G), f_B);

	float delta = C_Max - C_Min;

	/*Lightness*/
	float f_L = (C_Max + C_Min) / 2;
	hsl_color.L = f_L * 100;

	/*Saturation*/
	float f_S = 0;
	if (delta == 0 || f_L == 1 || f_L == 0) f_S = 0;
	else if (f_L > 0.5) f_S = delta / (1 - (2 * f_L - 1));
	else f_S = delta / (1 + (2 * f_L - 1));
	hsl_color.S = f_S * 100;

	/*Hue*/
	float  f_H = 0;
	if (delta == 0) f_H = 0;
	else if (C_Max == f_R) f_H = ((f_G - f_B) / 6 ) / delta;
	else if (C_Max == f_G) f_H = (float)1.0 /3 +  ((f_B - f_R) / 6) / delta;
	else f_H = (float)2.0 /3 + ((f_R - f_G) / 6) / delta;
	if (f_H < 0) f_H += 1;
	if (f_H > 1) f_H -= 1;
	hsl_color.H = (int)(f_H * 360);

	return hsl_color;
}

sRgbColor_t hsl_to_rgb(sHSL_Color_t hsl_color) {
	sRgbColor_t rgb_color = {.red = 0, .green = 0, .blue = 0};
	float f_S = (float) hsl_color.S / 100;
	float f_L = (float) hsl_color.L / 100;
	float f_H = (float)hsl_color.H / 360;

	if (f_S == 0) {
		rgb_color.red = rgb_color.green = rgb_color.blue = (int)(f_L * 255);
	}
	else {
		float v1, v2;

		v2 = (f_L < 0.5) ? (f_L * (1 + f_S)) : ((f_L + f_S) - (f_L * f_S));
		v1 = 2 * f_L - v2;

		rgb_color.red = (uint8_t)(255 * hue_to_rgb(v1, v2, f_H + 1.0/3));
		rgb_color.green = (uint8_t)(255 * hue_to_rgb(v1, v2, f_H));
		rgb_color.blue = (uint8_t)(255 * hue_to_rgb(v1, v2, f_H - 1.0/3));
	}
	return rgb_color;
}

float hue_to_rgb(float v1, float v2, float  vH) {
	if (vH < 0)
		vH += 1;

	if (vH > 1)
		vH -= 1;

	if ((6 * vH) < 1)
		return (v1 + (v2 - v1) * 6 * vH);

	if ((2 * vH) < 1)
		return v2;

	if ((3 * vH) < 2)
		return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

	return v1;
}

sRgbColor_t app_led_rgb(uint32_t u32Color, int lightness) {
	sRgbColor_t color_origin = app_led_rgb_convert_color_u32_to_rgb(u32Color);

	if (lightness > 100) lightness = 100;
	if (lightness < 0) lightness = 0;
	if (lightness == 100) return color_origin;

	sHSL_Color_t hsl_color = rgb_to_hsl(color_origin);

	hsl_color.L = (float)hsl_color.L * lightness / 100;

	sRgbColor_t color = hsl_to_rgb(hsl_color);

	return color;
}
