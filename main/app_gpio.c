/*
 * app_gpio.c
 *
 *  Created on: 7 Feb 2020
 *      Author: tongv
 */
#include "app_gpio.h"
#include "app_button.h"
#include "app_mqtt.h"
#include "app_common.h"
#include "config.h"
#include "app_endpoint_link.h"
#include "app_cmd_mgr.h"
#include "app_wifi_mode.h"
#include "app_time_server.h"
#include "app_fs_mgr.h"
#include "app_ble_mesh.h"

bool m_bBuzzerStatusRqs = false;
bool m_bBuzzerConfigRqs = false;

bool m_bBuzzerRun = false;

uint8_t m_u8GpioCnt = 0;
uint8_t m_u8LedCnt = 0;

uint8_t g_u8Cnt10ms = 0;
uint8_t g_u8Cnt100ms = 0;

TaskHandle_t xgpioHandle = NULL;
TaskHandle_t xledHandle = NULL;
TaskHandle_t xEndpointHandle = NULL;
TaskHandle_t xBuzzerHandle = NULL;

sRgbColor_t g_Color_Btn_ON = RGB_COLOR_YELLOW;
sRgbColor_t g_Color_Btn_OFF = RGB_COLOR_WHITE;

sSwRgbConfig_t g_Config;
sSwRgbConfig_t g_ConfigSaved;

sChannelControl_t g_ChannelControl[5];

sRgbLedControl_t g_RgbLedControl;

sNightModeControl_t g_NightControl;

static void task_gpio(void *parm);
static void task_buzzer(void* param);
static void task_eplink(void* param);

static intr_handle_t s_timer_handle;


//*****************************************
//*****************************************
//********** TIMER TG0 INTERRUPT **********
//*****************************************
//*****************************************
static void timer_tg0_isr(void* arg)
{
	static int io_state = 0;

	//Reset irq and set for next time
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;
    //----- HERE EVERY #uS -----
    if (m_bBuzzerRun) {
    	gpio_set_level(BUZZER_PIN, (io_state ++) % 2);
    }
    else {
    	BUZZER_OFF;
    	io_state = 0;
    }
}
//******************************************
//******************************************
//********** TIMER TG0 INITIALISE **********
//******************************************
//******************************************
void timer_tg0_initialise (int timer_period_us)
{
    timer_config_t config = {
            .alarm_en = true,				//Alarm Enable
            .counter_en = false,			//If the counter is enabled it will start incrementing / decrementing immediately after calling timer_init()
            .intr_type = TIMER_INTR_LEVEL,	//Is interrupt is triggered on timer’s alarm (timer_intr_mode_t)
            .counter_dir = TIMER_COUNT_UP,	//Does counter increment or decrement (timer_count_dir_t)
            .auto_reload = true,			//If counter should auto_reload a specific initial value on the timer’s alarm, or continue incrementing or decrementing.
            .divider = 80   				//Divisor of the incoming 80 MHz (12.5nS) APB_CLK clock. E.g. 80 = 1uS per timer tick
    };

    timer_init(TIMER_GROUP_0, TIMER_0, &config);
    timer_set_counter_value(TIMER_GROUP_0, TIMER_0, 0);
    timer_set_alarm_value(TIMER_GROUP_0, TIMER_0, timer_period_us);
    timer_enable_intr(TIMER_GROUP_0, TIMER_0);
    timer_isr_register(TIMER_GROUP_0, TIMER_0, &timer_tg0_isr, NULL, 0, &s_timer_handle);

    timer_start(TIMER_GROUP_0, TIMER_0);
}

#define __CHANNEL_MODE(__MODE)  (__MODE == SW_MODE_ON_OFF) ? "ON/OFF" : (__MODE == SW_MODE_COUNT_DOWN) ? "Count down" : "unknow"
#define __CHANNEL_CONTROL_MODE(__MODE) (__MODE == SW_CTRL_MODE_NORMAL) ? "NORMAL" : (__MODE == SW_CTRL_MODE_DISABLE) ? "DISABLE" : (__MODE == SW_CTRL_MODE_DISABLE_TOUCH) ? "DISABLE TOUCH" : (__MODE == SW_CTRL_MODE_DISABLE_REMOTE) ? "DISABLE REMOTE" : "unknow"

void gpio_init()
{
	//init gpio
	gpio_config_t ctr_gpio_config;
	ctr_gpio_config.intr_type 		= GPIO_INTR_DISABLE;
	ctr_gpio_config.mode 			= GPIO_MODE_OUTPUT;
	ctr_gpio_config.pin_bit_mask	= GPIO_PIN_SEL;
	ctr_gpio_config.pull_down_en	= GPIO_PULLDOWN_DISABLE;
	ctr_gpio_config.pull_up_en		= GPIO_PULLUP_ENABLE;
	gpio_config(&ctr_gpio_config);

	gpio_config_t mode_config;
	mode_config.intr_type = GPIO_INTR_DISABLE;
	mode_config.mode = GPIO_MODE_INPUT;
	mode_config.pin_bit_mask = (1ULL << MODE_PIN);
	mode_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
	mode_config.pull_up_en = GPIO_PULLUP_ENABLE;
	gpio_config(&mode_config);

	timer_tg0_initialise(183); //183

	app_led_init();
	RELAY_1_OFF; RELAY_2_OFF; RELAY_3_OFF; RELAY_4_OFF;

	gpio_set_original_config(&g_Config);
	app_filesystem_read_config(&g_Config);
	gpio_copy_config(&g_ConfigSaved, &g_Config);

	gpio_set_channel_config();
	gpio_set_led_config();
	gpio_set_night_config();
#if 1
	LREP_WARNING(__FUNCTION__,"GPIO config");
	LREP_WARNING(__FUNCTION__,"\tBuzzer Enb %d", g_Config.iBuzzerEnb);
	LREP_WARNING(__FUNCTION__,"\tLed Enb %d", g_Config.iLedEnb);
	LREP_WARNING(__FUNCTION__,"\tLed Rgb On %d", g_Config.u32RgbOn);
	LREP_WARNING(__FUNCTION__,"\tLed Rgb Off %d", g_Config.u32RgbOff);
	LREP_WARNING(__FUNCTION__,"\tLed Rgb lightness %d", g_Config.led_lighness);
	LREP_WARNING(__FUNCTION__,"\tChannel 1");
	LREP_WARNING(__FUNCTION__,"\t\t mode %s" , __CHANNEL_MODE(g_Config.channel_1_mode));
	LREP_WARNING(__FUNCTION__,"\t\t countdown time %d" , g_Config.channel_1_countdown);
	LREP_WARNING(__FUNCTION__,"\t\t control mode %s", __CHANNEL_CONTROL_MODE(g_Config.channel_1_control_mode));
	LREP_WARNING(__FUNCTION__,"\t\t led %s", (g_Config.channel_1_led_off) ? "DISABLE" : "ENABLE");
	LREP_WARNING(__FUNCTION__,"\t\t Rgb On %d", g_Config.channel_1_rgb_on);
	LREP_WARNING(__FUNCTION__,"\t\t Rgb Off %d", g_Config.channel_1_rgb_off);
	LREP_WARNING(__FUNCTION__,"\t\t Rgb Lightness %d", g_Config.channel_1_lightness);
	LREP_WARNING(__FUNCTION__,"\tChannel 2");
	LREP_WARNING(__FUNCTION__,"\t\t mode %s" , __CHANNEL_MODE(g_Config.channel_2_mode));
	LREP_WARNING(__FUNCTION__,"\t\t countdown time %d" , g_Config.channel_2_countdown);
	LREP_WARNING(__FUNCTION__,"\t\t control mode %s", __CHANNEL_CONTROL_MODE(g_Config.channel_2_control_mode));
	LREP_WARNING(__FUNCTION__,"\t\t led %s", (g_Config.channel_2_led_off) ? "DISABLE" : "ENABLE");
	LREP_WARNING(__FUNCTION__,"\t\t Rgb On %d", g_Config.channel_2_rgb_on);
	LREP_WARNING(__FUNCTION__,"\t\t Rgb Off %d", g_Config.channel_2_rgb_off);
	LREP_WARNING(__FUNCTION__,"\t\t Rgb Lightness %d", g_Config.channel_2_lightness);
	LREP_WARNING(__FUNCTION__,"\tChannel 3");
	LREP_WARNING(__FUNCTION__,"\t\t mode %s" , __CHANNEL_MODE(g_Config.channel_3_mode));
	LREP_WARNING(__FUNCTION__,"\t\t countdown time %d" , g_Config.channel_3_countdown);
	LREP_WARNING(__FUNCTION__,"\t\t control mode %s", __CHANNEL_CONTROL_MODE(g_Config.channel_3_control_mode));
	LREP_WARNING(__FUNCTION__,"\t\t led %s", (g_Config.channel_3_led_off) ? "DISABLE" : "ENABLE");
	LREP_WARNING(__FUNCTION__,"\t\t Rgb On %d", g_Config.channel_3_rgb_on);
	LREP_WARNING(__FUNCTION__,"\t\t Rgb Off %d", g_Config.channel_3_rgb_off);
	LREP_WARNING(__FUNCTION__,"\t\t Rgb Lightness %d", g_Config.channel_3_lightness);
	LREP_WARNING(__FUNCTION__,"\tChannel 4");
	LREP_WARNING(__FUNCTION__,"\t\t mode %s" , __CHANNEL_MODE(g_Config.channel_4_mode));
	LREP_WARNING(__FUNCTION__,"\t\t countdown time %d" , g_Config.channel_4_countdown);
	LREP_WARNING(__FUNCTION__,"\t\t control mode %s", __CHANNEL_CONTROL_MODE(g_Config.channel_4_control_mode));
	LREP_WARNING(__FUNCTION__,"\t\t led %s", (g_Config.channel_4_led_off) ? "DISABLE" : "ENABLE");
	LREP_WARNING(__FUNCTION__,"\t\t Rgb On %d", g_Config.channel_4_rgb_on);
	LREP_WARNING(__FUNCTION__,"\t\t Rgb Off %d", g_Config.channel_4_rgb_off);
	LREP_WARNING(__FUNCTION__,"\t\t Rgb Lightness %d", g_Config.channel_4_lightness);
	LREP_WARNING(__FUNCTION__,"\tCurtain 1 cycle %d", g_Config.curtain_cycle);
	LREP_WARNING(__FUNCTION__,"\tCurtain 2 cycle %d", g_Config.curtain_2_cycle);
	LREP_WARNING(__FUNCTION__,"\tNight mode %s", g_Config.iNightmodeEnb ? "ENABLE" : "DISABLE");
	if (g_Config.iNightmodeEnb) {
		LREP_WARNING(__FUNCTION__,"\t\tNight timezone %d", g_Config.iNightTimezone);
		struct tm nightBegin = *app_time_server_get_timetm(g_Config.u32NightBegin, g_Config.iNightTimezone);
		struct tm nightEnd = *app_time_server_get_timetm(g_Config.u32NightEnd, g_Config.iNightTimezone);
		LREP_WARNING(__FUNCTION__,"\t\tNight begin %d:%d - %d", nightBegin.tm_hour, nightBegin.tm_min, g_Config.u32NightBegin);
		LREP_WARNING(__FUNCTION__,"\t\tNight end %d:%d - %d", nightEnd.tm_hour, nightEnd.tm_min, g_Config.u32NightEnd);
	}
	LREP_WARNING(__FUNCTION__,"\tSave state %s", g_Config.saveStt ? "ENABLE" : "DISABLE");
#endif
	xTaskCreate(task_eplink, "task_eplink", 4096, NULL, 3, &xEndpointHandle);
	xTaskCreate(task_buzzer, "task_buzzer", 1024, NULL, 3, &xBuzzerHandle);
	xTaskCreate(task_gpio, "task_gpio", 4096, NULL, 3, &xgpioHandle);
}

void buzzer() {
	m_bBuzzerStatusRqs = true;
}

void buzzer_config() {
	m_bBuzzerConfigRqs = true;
}

bool app_touch_change_channel_status(uint8_t channel) {
#ifdef DEVICE_TYPE
	if (g_ChannelControl[channel].bTouchDisable) return false;
	if (channel == 0 || channel > 4) return false;
	g_ChannelControl[channel].iStatusReq = (g_ChannelControl[channel].iStatus) ? 0 : 1;
	g_ChannelControl[channel].bEplinkReq = true;
	LREP_WARNING(__FUNCTION__, "channel %d", channel);
	return true;
#endif
}

bool app_remote_change_channel_status(uint8_t channel, int status) {
	if (g_ChannelControl[channel].bRemoteDisable) return false;
//	bool ret = (status != g_ChannelControl[channel].iStatus);
	g_ChannelControl[channel].iStatusReq = status;
	g_ChannelControl[channel].bEplinkReq = true;
//	return ret;
	return true;
}

void app_remote_eplink_change_channel_status(uint8_t channel, int status) {
	if (g_ChannelControl[channel].bRemoteDisable) return;
	g_ChannelControl[channel].iStatusReq = status;
}

void gpio_scan_channel(uint8_t channel) {
	sChannelControl_t* pControl = &g_ChannelControl[channel];
	if (pControl->iRelay == 0) return;
	if (pControl->bRemoteDisable && pControl->bTouchDisable)  pControl->iStatusReq = 0;
	if (pControl->bfirstRun || pControl->iStatus != pControl->iStatusReq) {
		if (pControl->bfirstRun && g_Config.saveStt) pControl->iStatus = pControl->iStatusReq = app_filesytem_read_status(channel);
		else {
			if (pControl->iStatusReq == -1) pControl->iStatusReq = pControl->iStatus ? 0 : 1;
			pControl->iStatus = pControl->iStatusReq;
		}
		if (pControl->iStatus) { app_relay_ctrl(pControl->iRelay, 1); pControl->u64Ontime = 0; }
		else app_relay_ctrl(pControl->iRelay, 0);
		pControl->bfirstRun = false; g_bRequestGetData = true;
		if (g_Config.saveStt) app_filesytem_write_status(channel, pControl->iStatus);
#ifdef DEVICE_TYPE
		switch(channel) {
		case 1: g_RgbLedControl.sttRqs[1] = pControl->iStatus ? true : false; break;
		case 2: g_RgbLedControl.sttRqs[2] = pControl->iStatus ? true : false; break;
		case 3: g_RgbLedControl.sttRqs[3] = pControl->iStatus ? true : false; break;
		default:
			break;
		}
#endif
	}
	else if (pControl->iStatus) {
		if (pControl->iMode == SW_MODE_COUNT_DOWN) {
			if (pControl->u64Ontime != 0) {
				if ((g_MainTime - pControl->u64Ontime) > pControl->iCountDown) {
					pControl->iStatusReq = 0;
					pControl->u64Ontime = 0;
					pControl->bEplinkReq = true;
				}
			}
			else pControl->u64Ontime = g_MainTime;
		}
	}
}

void gpio_scan_buzzer(void) {
	if (g_Config.iBuzzerEnb == 0 || g_NightControl.bNight) {
		m_bBuzzerStatusRqs = false;
		m_bBuzzerConfigRqs = false;
		return;
	}

	if (m_bBuzzerStatusRqs) {
		m_bBuzzerStatusRqs = false;
		m_bBuzzerRun = true;
		delay_ms(100);
		m_bBuzzerRun = false;
//		delay_ms(100);
	}
	else if (m_bBuzzerConfigRqs) {
		m_bBuzzerConfigRqs = false;
		for (int i =0; i< 3; i++){
			m_bBuzzerRun = true;
			delay_ms(100);
			m_bBuzzerRun = false;
			delay_ms(100);
		}
	}
}

#define __LED_BLINK(__LED, __STATUS, __STATUS_REQ, __BLINK_CNT, __IMPACT, __STT) \
do {\
	if (g_RgbLedControl.disable[__LED]) {\
		app_led_bt_ctl(__LED, RGB_COLOR_BLACK);\
		break;\
	}\
	if (__STATUS != __STATUS_REQ) {\
		if (__BLINK_CNT >= 6) {\
			__STATUS = __STATUS_REQ; __BLINK_CNT = 0; __IMPACT = g_MainTime;\
			app_led_bt_ctl(__LED, __STATUS ? g_RgbLedControl.onColor[__LED] : g_RgbLedControl.offColor[__LED]);\
		}\
		else {\
			if((__STT%5==0)) __BLINK_CNT++;\
			if (__BLINK_CNT %2) app_led_bt_ctl(__LED, RGB_COLOR_BLACK);\
			else app_led_bt_ctl(__LED, __STATUS_REQ ? g_RgbLedControl.onColor[__LED] : g_RgbLedControl.offColor[__LED]);\
		}\
	}\
} while(0)

#define __LED_BREAK(__LED, __STATUS, __IMPACT) \
do {\
	if (g_RgbLedControl.disable[__LED]) {\
		app_led_bt_ctl(__LED, RGB_COLOR_BLACK);\
		break;\
	}\
	if (__IMPACT != 0) {\
		if ((g_MainTime - __IMPACT) > 10) {\
			__IMPACT = 0; app_led_bt_ctl(__LED, RGB_COLOR_BLACK);\
		}\
		else {\
			if (__STATUS) app_led_bt_ctl(__LED, g_RgbLedControl.onColor[__LED]);\
			else app_led_bt_ctl(__LED, g_RgbLedControl.offColor[__LED]);\
		}\
	}\
	else app_led_bt_ctl(__LED, RGB_COLOR_BLACK);\
} while(0)


#define __LED_ON_OFF(__LED, __STATUS, __IMPACT) \
do {\
	if (g_RgbLedControl.disable[__LED]) {\
		app_led_bt_ctl(__LED, RGB_COLOR_BLACK);\
		break;\
	}\
	if (__STATUS) app_led_bt_ctl(__LED, g_RgbLedControl.onColor[__LED]);\
	else app_led_bt_ctl(__LED, g_RgbLedControl.offColor[__LED]);\
	__IMPACT = 1;\
}while(0)

#define __LED_REQ_ON_OFF(__LED, __STATUS, __STATUS_REQ, __IMPACT) \
do {\
	if (g_RgbLedControl.disable[__LED]) {\
		app_led_bt_ctl(__LED, RGB_COLOR_BLACK);\
		break;\
	}\
	if (__STATUS != __STATUS_REQ) {\
		__STATUS = __STATUS_REQ;\
		app_led_bt_ctl(__LED, __STATUS ? g_RgbLedControl.onColor[__LED] : g_RgbLedControl.offColor[__LED]);\
		__IMPACT = g_MainTime;\
	}\
} while (0)

#define __LED_BLINK_CONTROL(__LED)	__LED_BLINK(__LED, g_RgbLedControl.stt[__LED], g_RgbLedControl.sttRqs[__LED], g_RgbLedControl.blink[__LED], g_RgbLedControl.impact[__LED], g_RgbLedControl.iCount)
#define __LED_BREAK_CONTROL(__LED)  __LED_BREAK(__LED, g_RgbLedControl.stt[__LED], g_RgbLedControl.impact[__LED])
#define __LED_ON_OFF_CONTROL(__LED) __LED_ON_OFF(__LED, g_RgbLedControl.stt[__LED], g_RgbLedControl.impact[__LED])
#define __LED_REQ_ON_OFF_CONTROL(__LED) __LED_REQ_ON_OFF(__LED, g_RgbLedControl.stt[__LED], g_RgbLedControl.sttRqs[__LED], g_RgbLedControl.impact[__LED])
#define __LED_COMMOM_IMPACT()		\
do {\
	g_RgbLedControl.impact[0] = (g_RgbLedControl.impact[0] > g_RgbLedControl.impact[1]) ? g_RgbLedControl.impact[0] : g_RgbLedControl.impact[1];\
	g_RgbLedControl.impact[0] = (g_RgbLedControl.impact[0] > g_RgbLedControl.impact[2]) ? g_RgbLedControl.impact[0] : g_RgbLedControl.impact[2];\
	g_RgbLedControl.impact[0] = (g_RgbLedControl.impact[0] > g_RgbLedControl.impact[3]) ? g_RgbLedControl.impact[0] : g_RgbLedControl.impact[3];\
	g_RgbLedControl.impact[0] = (g_RgbLedControl.impact[0] > g_RgbLedControl.impact[4]) ? g_RgbLedControl.impact[0] : g_RgbLedControl.impact[4];\
	g_RgbLedControl.impact[1] = g_RgbLedControl.impact[1] ? g_RgbLedControl.impact[0] : 0;\
	g_RgbLedControl.impact[2] = g_RgbLedControl.impact[2] ? g_RgbLedControl.impact[0] : 0;\
	g_RgbLedControl.impact[3] = g_RgbLedControl.impact[3] ? g_RgbLedControl.impact[0] : 0;\
	g_RgbLedControl.impact[4] = g_RgbLedControl.impact[4] ? g_RgbLedControl.impact[0] : 0;\
}while(0)


void gpio_scan_leds() {
	g_RgbLedControl.iCount++;

	if (g_RgbLedControl.ledMode == LED_MODE_BLINK) {
		if ((g_RgbLedControl.iCount % 20) == 0) app_led_bt_ctl_all(g_RgbLedControl.blinkColor);
		else if ((g_RgbLedControl.iCount % 20) == 10) app_led_bt_ctl_all(RGB_COLOR_BLACK);
	}
	else if (g_RgbLedControl.stt[1] == g_RgbLedControl.sttRqs[1] &&
			g_RgbLedControl.stt[2] == g_RgbLedControl.sttRqs[2] &&
			g_RgbLedControl.stt[3] == g_RgbLedControl.sttRqs[3] &&
			g_RgbLedControl.stt[4] == g_RgbLedControl.sttRqs[4]) {
		if (g_NightControl.bNight || g_Config.iLedEnb == 0) {
			__LED_BREAK_CONTROL(1);
			__LED_BREAK_CONTROL(2);
			__LED_BREAK_CONTROL(3);
			__LED_BREAK_CONTROL(4);
		}
		else {
#ifdef DEVICE_TYPE
			__LED_ON_OFF_CONTROL(1);
			__LED_ON_OFF_CONTROL(2);
			__LED_ON_OFF_CONTROL(3);
#endif
		}
	}
	else if (!app_is_connected()) {
		__LED_BLINK_CONTROL(1);
		__LED_BLINK_CONTROL(2);
		__LED_BLINK_CONTROL(3);
		__LED_BLINK_CONTROL(4);
		__LED_COMMOM_IMPACT();
	}
	else {
		__LED_REQ_ON_OFF_CONTROL(1);
		__LED_REQ_ON_OFF_CONTROL(2);
		__LED_REQ_ON_OFF_CONTROL(3);
		__LED_REQ_ON_OFF_CONTROL(4);
		__LED_COMMOM_IMPACT();
	}
}

void task_buzzer(void* param) {
	for(;;) {
		g_bRun_BuzzerTask = true;
		gpio_scan_buzzer();
		delay_ms(50);
	}
}

void task_gpio(void *parm) {
	if(g_Config.iLedEnb == 0) app_led_bt_ctl_all(RGB_COLOR_BLACK);
	for (;;) {
		g_bRun_GpioTask = true;
#ifdef DEVICE_TYPE
		gpio_scan_channel(1);
		gpio_scan_channel(2);
		gpio_scan_channel(3);
		gpio_scan_channel(4);
		gpio_scan_leds();
#endif
		delay_ms(10);
	}
}

void task_eplink(void *parm) {
	for (;;) {
		g_bRun_RelayTask = true;
#ifdef DEVICE_TYPE
		if (g_ChannelControl[1].bEplinkReq && g_ChannelControl[1].iStatus == g_ChannelControl[1].iStatusReq) {
			g_ChannelControl[1].bEplinkReq = false;
			if(strlen(g_endpoint_link_arr[1].chdesDevId) > 0 && strcmp(g_endpoint_link_arr[1].chdesDevId,"null") != 0)
				app_cmd_send_EndpointSetData(1, app_cmd_get_deviceId());
		}

		if (g_ChannelControl[2].bEplinkReq && g_ChannelControl[2].iStatus == g_ChannelControl[2].iStatusReq) {
			g_ChannelControl[2].bEplinkReq = false;
			if(strlen(g_endpoint_link_arr[2].chdesDevId) > 0 && strcmp(g_endpoint_link_arr[2].chdesDevId,"null") != 0)
				app_cmd_send_EndpointSetData(2, app_cmd_get_deviceId());
		}

		if (g_ChannelControl[3].bEplinkReq && g_ChannelControl[3].iStatus == g_ChannelControl[3].iStatusReq) {
			g_ChannelControl[3].bEplinkReq = false;
			if(strlen(g_endpoint_link_arr[3].chdesDevId) > 0 && strcmp(g_endpoint_link_arr[3].chdesDevId,"null") != 0)
				app_cmd_send_EndpointSetData(3, app_cmd_get_deviceId());
		}

#endif
		delay_ms(100);
	}
}

void gpio_set_led_color(int channel, int on_off, int color) {
#ifdef DEVICE_TYPE
	if (channel == 0) {
		if (on_off == 1)
			g_Config.u32RgbOn = g_Config.channel_1_rgb_on = g_Config.channel_2_rgb_on = g_Config.channel_3_rgb_on = g_Config.channel_4_rgb_on = color;
		else
			g_Config.u32RgbOff = g_Config.channel_1_rgb_off = g_Config.channel_2_rgb_off = g_Config.channel_3_rgb_off = g_Config.channel_4_rgb_off = color;
	}
	else if (channel == SW_CHANNEL_1) {
		if (on_off) g_Config.channel_1_rgb_on = color;
		else g_Config.channel_1_rgb_off = color;
	}
	else if (channel == SW_CHANNEL_2) {
		if (on_off) g_Config.channel_2_rgb_on = color;
		else g_Config.channel_2_rgb_off = color;
	}
	else if (channel == SW_CHANNEL_3) {
		if (on_off) g_Config.channel_3_rgb_on = color;
		else g_Config.channel_3_rgb_off = color;
	}
	else if (channel == SW_CHANNEL_4) {
		if (on_off) g_Config.channel_4_rgb_on = color;
		else g_Config.channel_4_rgb_off = color;
	}
#endif
	gpio_set_led_config();
}

void gpio_set_led_lightness(int channel, int lightness) {
	if (channel == 0)
		g_Config.led_lighness = g_Config.channel_1_lightness = g_Config.channel_2_lightness = g_Config.channel_3_lightness = g_Config.channel_4_lightness = lightness;
	else if (channel == SW_CHANNEL_1)
		g_Config.channel_1_lightness = lightness;
	else if (channel == SW_CHANNEL_2)
		g_Config.channel_2_lightness = lightness;
	else if (channel == SW_CHANNEL_3)
		g_Config.channel_3_lightness = lightness;
	else if (channel == SW_CHANNEL_4)
		g_Config.channel_4_lightness = lightness;
	gpio_set_led_config();
}

void gpio_set_original_config(sSwRgbConfig_t* pConfig) {
	pConfig->iBuzzerEnb = 1;
	pConfig->iLedEnb = 1;
	pConfig->u32RgbOn = COLOR_ON_DEFAULT;
	pConfig->u32RgbOff = COLOR_OFF_DEFAULT;
	pConfig->channel_1_mode = SW_MODE_ON_OFF;
	pConfig->channel_1_countdown = 30;
	pConfig->channel_2_mode = SW_MODE_ON_OFF;
	pConfig->channel_2_countdown = 30;
	pConfig->channel_3_mode = SW_MODE_ON_OFF;
	pConfig->channel_3_countdown = 30;
	pConfig->channel_4_mode = SW_MODE_ON_OFF;
	pConfig->channel_4_countdown = 30;
	pConfig->curtain_cycle = 60;
	pConfig->curtain_2_cycle = 60;
	pConfig->channel_1_control_mode = SW_CTRL_MODE_NORMAL;
	pConfig->channel_2_control_mode = SW_CTRL_MODE_NORMAL;
	pConfig->channel_3_control_mode = SW_CTRL_MODE_NORMAL;
	pConfig->channel_4_control_mode = SW_CTRL_MODE_NORMAL;
	pConfig->channel_1_led_off = 0;
	pConfig->channel_1_rgb_on = COLOR_ON_DEFAULT;
	pConfig->channel_1_rgb_off = COLOR_OFF_DEFAULT;
	pConfig->channel_2_led_off = 0;
	pConfig->channel_2_rgb_on = COLOR_ON_DEFAULT;
	pConfig->channel_2_rgb_off = COLOR_OFF_DEFAULT;
	pConfig->channel_3_led_off = 0;
	pConfig->channel_3_rgb_on = COLOR_ON_DEFAULT;
	pConfig->channel_3_rgb_off = COLOR_OFF_DEFAULT;
	pConfig->channel_4_led_off = 0;
	pConfig->channel_4_rgb_on = COLOR_ON_DEFAULT;
	pConfig->channel_4_rgb_off = COLOR_OFF_DEFAULT;
	pConfig->iNightmodeEnb = 0;
	pConfig->u32NightBegin = NIGHT_BEGIN_DEFAULT;
	pConfig->u32NightEnd = NIGHT_END_DEFAULT;
	pConfig->iNightTimezone = NIGHT_TZ_DEFAULT;
	pConfig->channel_1_lightness = LIGHTNESS_DEFAULT;
	pConfig->channel_2_lightness = LIGHTNESS_DEFAULT;
	pConfig->channel_3_lightness = LIGHTNESS_DEFAULT;
	pConfig->channel_4_lightness = LIGHTNESS_DEFAULT;
	pConfig->led_lighness = LIGHTNESS_DEFAULT;
	pConfig->saveStt = 0;
}

void gpio_copy_config(sSwRgbConfig_t* pConfigOut, const sSwRgbConfig_t* pConfigIn) {
	memcpy(pConfigOut, pConfigIn, sizeof(sSwRgbConfig_t));
}

bool gpio_config_compare(const sSwRgbConfig_t* pConfigX, const sSwRgbConfig_t* pConfigY) {
	if (memcmp(pConfigX, pConfigY, sizeof(sSwRgbConfig_t))!= 0) return false;
	return true;
}

void app_gpio_config_flash_handle(void) {
	if (!gpio_config_compare(&g_ConfigSaved, &g_Config)) {
		LREP_WARNING(__FUNCTION__,"Config change");
		gpio_copy_config(&g_ConfigSaved, &g_Config);
		app_filesystem_write_config(&g_ConfigSaved);
	}
}

void gpio_set_channel_config(void) {
	static bool firstrun = true;
	if (firstrun) {
		firstrun = false;
		for (int i = 1; i<=4; ++i) {
			g_ChannelControl[i].iStatus = 0;
			g_ChannelControl[i].iStatusReq = 0;
			g_ChannelControl[i].u64Ontime = 0;
			g_ChannelControl[i].bfirstRun = true;
			g_ChannelControl[i].bEplinkReq = false;
		}
	}
	for (int i = 1; i<=4; ++i) {
		g_ChannelControl[i].iMode = (i==1) ? g_Config.channel_1_mode : (i==2) ? g_Config.channel_2_mode : (i==3) ? g_Config.channel_3_mode : g_Config.channel_4_mode;
		g_ChannelControl[i].iCountDown = (i==1) ? g_Config.channel_1_countdown :(i==2) ? g_Config.channel_2_countdown :
				(i==3) ? g_Config.channel_3_countdown : g_Config.channel_4_countdown;
	}
	g_ChannelControl[1].bTouchDisable = g_Config.channel_1_control_mode == SW_CTRL_MODE_DISABLE || g_Config.channel_1_control_mode == SW_CTRL_MODE_DISABLE_TOUCH;
	g_ChannelControl[1].bRemoteDisable = g_Config.channel_1_control_mode == SW_CTRL_MODE_DISABLE || g_Config.channel_1_control_mode == SW_CTRL_MODE_DISABLE_REMOTE;
	g_ChannelControl[2].bTouchDisable = g_Config.channel_2_control_mode == SW_CTRL_MODE_DISABLE || g_Config.channel_2_control_mode == SW_CTRL_MODE_DISABLE_TOUCH;
	g_ChannelControl[2].bRemoteDisable = g_Config.channel_2_control_mode == SW_CTRL_MODE_DISABLE || g_Config.channel_2_control_mode == SW_CTRL_MODE_DISABLE_REMOTE;
	g_ChannelControl[3].bTouchDisable = g_Config.channel_3_control_mode == SW_CTRL_MODE_DISABLE || g_Config.channel_3_control_mode == SW_CTRL_MODE_DISABLE_TOUCH;
	g_ChannelControl[3].bRemoteDisable = g_Config.channel_3_control_mode == SW_CTRL_MODE_DISABLE || g_Config.channel_3_control_mode == SW_CTRL_MODE_DISABLE_REMOTE;
	g_ChannelControl[4].bTouchDisable = g_Config.channel_4_control_mode == SW_CTRL_MODE_DISABLE || g_Config.channel_4_control_mode == SW_CTRL_MODE_DISABLE_TOUCH;
	g_ChannelControl[4].bRemoteDisable = g_Config.channel_4_control_mode == SW_CTRL_MODE_DISABLE || g_Config.channel_4_control_mode == SW_CTRL_MODE_DISABLE_REMOTE;

#ifdef DEVICE_TYPE
	g_ChannelControl[1].iRelay = 1;
	g_ChannelControl[2].iRelay = 2;
	g_ChannelControl[3].iRelay = 3;
	g_ChannelControl[4].iRelay = 0;
#endif

}

void gpio_set_led_config(void) {
	static bool firstrun = true;
#ifdef DEVICE_TYPE
	g_RgbLedControl.onColor[1] = app_led_rgb(g_Config.channel_1_rgb_on, g_Config.channel_1_lightness);
	g_RgbLedControl.onColor[2] = app_led_rgb(g_Config.channel_2_rgb_on, g_Config.channel_2_lightness);
	g_RgbLedControl.onColor[3] = app_led_rgb(g_Config.channel_3_rgb_on, g_Config.channel_3_lightness);

	g_RgbLedControl.offColor[1] = app_led_rgb(g_Config.channel_1_rgb_off, g_Config.channel_1_lightness);
	g_RgbLedControl.offColor[2] = app_led_rgb(g_Config.channel_2_rgb_off, g_Config.channel_2_lightness);
	g_RgbLedControl.offColor[3] = app_led_rgb(g_Config.channel_3_rgb_off, g_Config.channel_3_lightness);

	g_RgbLedControl.disable[1] = (g_Config.channel_1_led_off == 0) ? false : true;
	g_RgbLedControl.disable[2] = (g_Config.channel_2_led_off == 0) ? false : true;
	g_RgbLedControl.disable[3] = (g_Config.channel_3_led_off == 0) ? false : true;

	if (firstrun) {
		firstrun = false;
		app_led_bt_ctl(1, g_RgbLedControl.offColor[1]);
		app_led_bt_ctl(2, g_RgbLedControl.offColor[2]);
		app_led_bt_ctl(3, g_RgbLedControl.offColor[3]);
		app_led_bt_ctl(4, RGB_COLOR_BLACK);
	}
#endif
}

void gpio_set_led_auto(void) {
	g_RgbLedControl.ledMode = LED_MODE_BLINK;
	g_RgbLedControl.blinkColor = RGB_OFF_DEFAULT;
}

void gpio_set_blufi_connected(void) {
	g_RgbLedControl.ledMode = LED_MODE_BLINK;
	g_RgbLedControl.blinkColor = RGB_BLUFI_CONNECT;
}

void gpio_set_led_manual(void) {
	g_RgbLedControl.ledMode = LED_MODE_BLINK;
	g_RgbLedControl.blinkColor = RGB_ON_DEFAULT;
}

void gpio_set_led_normal(void) {
	g_RgbLedControl.ledMode = LED_MODE_AUTO;
}

void gpio_disable_led_channel(int channel) {
	if (channel == SW_CHANNEL_1) 		g_Config.channel_1_led_off = 1;
	else if (channel == SW_CHANNEL_2) 	g_Config.channel_2_led_off = 1;
	else if (channel == SW_CHANNEL_3) 	g_Config.channel_3_led_off = 1;
	else if (channel == SW_CHANNEL_4) 	g_Config.channel_4_led_off = 1;
	gpio_set_led_config();
}

void gpio_enable_led_channel(int channel) {
	if (channel == SW_CHANNEL_1) 		g_Config.channel_1_led_off = 0;
	else if (channel == SW_CHANNEL_2) 	g_Config.channel_2_led_off = 0;
	else if (channel == SW_CHANNEL_3) 	g_Config.channel_3_led_off = 0;
	else if (channel == SW_CHANNEL_4) 	g_Config.channel_4_led_off = 0;
	gpio_set_led_config();
}

void gpio_set_channel_type(int channel, int type) {
	if (type != SW_MODE_ON_OFF && type != SW_MODE_COUNT_DOWN) return;
	if (channel == SW_CHANNEL_1)  		g_Config.channel_1_mode = type;
	else if (channel == SW_CHANNEL_2) 	g_Config.channel_2_mode = type;
	else if (channel == SW_CHANNEL_3) 	g_Config.channel_3_mode = type;
	else if (channel == SW_CHANNEL_4) 	g_Config.channel_4_mode = type;
	gpio_set_channel_config();
}

void gpio_set_channel_countdown(int channel, int countdown) {
	if (countdown < 0) return;
	if (channel == SW_CHANNEL_1)  		g_Config.channel_1_countdown = countdown;
	else if (channel == SW_CHANNEL_2) 	g_Config.channel_2_countdown = countdown;
	else if (channel == SW_CHANNEL_3) 	g_Config.channel_3_countdown = countdown;
	else if (channel == SW_CHANNEL_4) 	g_Config.channel_4_countdown = countdown;
	gpio_set_channel_config();
}

void gpio_set_channel_ctrl_mode(int channel, int ctr_mode) {
	if (ctr_mode != SW_CTRL_MODE_NORMAL && ctr_mode != SW_CTRL_MODE_DISABLE &&
			ctr_mode != SW_CTRL_MODE_DISABLE_TOUCH && ctr_mode != SW_CTRL_MODE_DISABLE_REMOTE) return;
	if (channel == SW_CHANNEL_1)  		g_Config.channel_1_control_mode = ctr_mode;
	else if (channel == SW_CHANNEL_2) 	g_Config.channel_2_control_mode = ctr_mode;
	else if (channel == SW_CHANNEL_3) 	g_Config.channel_3_control_mode = ctr_mode;
	else if (channel == SW_CHANNEL_4) 	g_Config.channel_4_control_mode = ctr_mode;
	gpio_set_channel_config();
}

void gpio_disable_nightmode(void) {
	g_Config.iNightmodeEnb = 0;
	gpio_set_night_config();
}

void gpio_enable_nightmode(void) {
	g_Config.iNightmodeEnb = 1;
	gpio_set_night_config();
}

void gpio_set_night_begin(uint32_t time_begin) {
	g_Config.u32NightBegin = time_begin;
	gpio_set_night_config();
}

void gpio_set_night_end(uint32_t time_end) {
	g_Config.u32NightEnd = time_end;
	gpio_set_night_config();
}

void gpio_set_night_timezone(int timezone) {
	g_Config.iNightTimezone = timezone;
	gpio_set_night_config();
}

void gpio_set_night_config(void) {
	g_NightControl.tm_begin = *app_time_server_get_timetm(g_Config.u32NightBegin, g_Config.iNightTimezone);
	g_NightControl.tm_end = *app_time_server_get_timetm(g_Config.u32NightEnd, g_Config.iNightTimezone);
	LREP(__FUNCTION__,"\t\tNight begin %d:%d - %d", g_NightControl.tm_begin.tm_hour, g_NightControl.tm_begin.tm_min, g_Config.u32NightBegin);
	LREP(__FUNCTION__,"\t\tNight end %d:%d - %d", g_NightControl.tm_end.tm_hour, g_NightControl.tm_end.tm_min, g_Config.u32NightEnd);
	gpio_update_night();
}

void gpio_update_night(void) {
	if (!app_time_server_sync() || g_Config.iNightmodeEnb == 0) g_NightControl.bNight = false;
	else {
		struct tm tm_now = *app_time_server_get_timetm(g_MainTime, g_Config.iNightTimezone);
		if (g_NightControl.tm_begin.tm_hour == g_NightControl.tm_end.tm_hour && g_NightControl.tm_begin.tm_min == g_NightControl.tm_end.tm_min)  {
			if (tm_now.tm_hour == g_NightControl.tm_begin.tm_hour && tm_now.tm_min == g_NightControl.tm_begin.tm_min) g_NightControl.bNight = true;
			else g_NightControl.bNight = false;
		}
		else if ((g_NightControl.tm_begin.tm_hour == g_NightControl.tm_end.tm_hour && g_NightControl.tm_begin.tm_min < g_NightControl.tm_end.tm_min) ||
				g_NightControl.tm_begin.tm_hour < g_NightControl.tm_end.tm_hour) {
			if (tm_now.tm_hour > g_NightControl.tm_end.tm_hour ||
					(tm_now.tm_hour == g_NightControl.tm_end.tm_hour && tm_now.tm_min >= g_NightControl.tm_end.tm_min)) g_NightControl.bNight = false;
			else if (tm_now.tm_hour < g_NightControl.tm_begin.tm_hour ||
					(tm_now.tm_hour == g_NightControl.tm_begin.tm_hour && tm_now.tm_min < g_NightControl.tm_begin.tm_min)) g_NightControl.bNight = false;
			else g_NightControl.bNight = true;
		}
		else if ((g_NightControl.tm_begin.tm_hour == g_NightControl.tm_end.tm_hour && g_NightControl.tm_begin.tm_min > g_NightControl.tm_end.tm_min) ||
				g_NightControl.tm_begin.tm_hour > g_NightControl.tm_end.tm_hour) {
			if (tm_now.tm_hour > g_NightControl.tm_begin.tm_hour ||
					(tm_now.tm_hour == g_NightControl.tm_begin.tm_hour && tm_now.tm_min >= g_NightControl.tm_begin.tm_min)) g_NightControl.bNight = true;
			else if (tm_now.tm_hour < g_NightControl.tm_end.tm_hour ||
					(tm_now.tm_hour == g_NightControl.tm_end.tm_hour && tm_now.tm_min < g_NightControl.tm_end.tm_min)) g_NightControl.bNight = true;
			else g_NightControl.bNight = false;
		}
	}
}

bool app_is_wifi_mode(void) {
#ifdef BLE_MESH_ENABLE
	return false;
#endif
	return true;
	// future use
	if (gpio_get_level(MODE_PIN) == 1) return true;
	else return false;
}

bool app_is_connected(void) {
	if (app_is_wifi_mode()) return g_bMqttstatus;
	else return app_ble_mesh_provisioned();
}

bool app_gpio_is_idle(void) {
#ifdef DEVICE_TYPE
	if (g_ChannelControl[1].iStatusReq != 0) return false;
	if (g_ChannelControl[2].iStatusReq != 0) return false;
	if (g_ChannelControl[3].iStatusReq != 0) return false;
#endif
	return true;
}

void app_relay_ctrl(int relay, int onoff) {
	LREP(__FUNCTION__, "TURN %s Relay %d", onoff ? "ON" : "OFF", relay);
	if (relay == 1) {
		if (onoff) RELAY_1_ON;
		else RELAY_1_OFF;
	}
	else if (relay == 2) {
		if (onoff) RELAY_2_ON;
		else RELAY_2_OFF;
	}
	else if (relay == 3) {
		if (onoff) RELAY_3_ON;
		else RELAY_3_OFF;
	}
	else if (relay == 4) {
		if (onoff) RELAY_4_ON;
		else RELAY_4_OFF;
	}
}
