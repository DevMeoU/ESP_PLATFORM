/*
 * main.c
 *
 *  Created on: Dec 13, 2019
 *      Author: Admin
 */

#include "app_button.h"
#include "app_gpio.h"
#include "app_wifi_mode.h"
#include "app_common.h"
#include "config.h"
#include "app_endpoint_link.h"
#include "app_button_CY8CMBR3xxx.h"
#include "app_ble_mesh.h"
#include "app_button_CAP1206.h"
#include "I2C.h"

TaskHandle_t xbuttonHandle = NULL;
uint8_t m_u8ButtonData[3];
uint8_t m_u8ButtonCnt = 0;
uint8_t m_u8ButtonCur = 0;
uint8_t m_u8ButtonPre = 0;
uint16_t m_u16ButtonRepeat = 0;
uint8_t m_u8ButtonInput = 0;
uint8_t m_u8ZeroFilter;
bool m_bCalibReq = false;
eButtonType_t g_ButtonType = BUTTON_TYPE_MAX;
static void task_button(void *parm);

/*
 * gpio config
 */
void button_gpio_config()
{
	LREP(__FUNCTION__, "[app_button][button_gpio_config]");

	gpio_config_t button_config;
	button_config.intr_type 	= GPIO_INTR_DISABLE;
	button_config.mode 			= GPIO_MODE_INPUT;
	button_config.pin_bit_mask	= BUTTON_PIN_SEL;
	button_config.pull_down_en	= GPIO_PULLDOWN_DISABLE;
	button_config.pull_up_en	= GPIO_PULLUP_ENABLE;
	gpio_config(&button_config);

	gpio_config_t power_control_config;
	power_control_config.intr_type 		= GPIO_INTR_DISABLE;
	power_control_config.mode 			= GPIO_MODE_OUTPUT;
	power_control_config.pin_bit_mask	= 1ULL << BUTTON_POWER_CONTROL_PIN;
	power_control_config.pull_down_en	= GPIO_PULLDOWN_DISABLE;
	power_control_config.pull_up_en		= GPIO_PULLUP_ENABLE;
	gpio_config(&power_control_config);

	BUTTON_POWER_ON();
}

/*
 * button scans
 */
void button_scans()
{
	if (g_ButtonType == BUTTON_TYPE_CMBR3108) {
		static uint8_t data_wr[2] = {0};
		static uint8_t data_rd[2] = {0};

		data_wr[0] = MBR_REG_BUTTON_STATUS;
		app_cy8cmbr3xxx_write(data_wr, 1);

		if (app_cy8cmbr3xxx_read(data_rd, 2)) {
			m_u8ButtonInput = data_rd[0];
		}
		else {
			LREP_ERROR(__FUNCTION__, "CY8CMBR3116 scan");
			m_u8ButtonInput = 0;
		}
	}
	else if (g_ButtonType == BUTTON_TYPE_CAP1206) {
		uint8_t u8Input = 0;
		app_cap1206_read_byte(CAP1206_REG_SENSOR_INPUT_STATUS, &u8Input);
		if (u8Input) {
			m_u8ButtonInput = u8Input;
			m_u8ZeroFilter = 0;
			uint8_t u8R_MainControl = 0;
			app_cap1206_read_byte(CAP1206_REG_MAIN_CONTROL, &u8R_MainControl);
			app_cap1206_write_byte(CAP1206_REG_MAIN_CONTROL, u8R_MainControl & 0xFE);
	//		LREP(__FUNCTION__, "m_u8ButtonInput 0x%02X", m_u8ButtonInput);
		}
		else {
			if (m_u16ButtonRepeat < BUTTON_WAIT_1_2 && m_u8ZeroFilter++>=6) m_u8ButtonInput = u8Input;
			else if (m_u16ButtonRepeat >= BUTTON_WAIT_1_2 && m_u8ZeroFilter++>=10) m_u8ButtonInput = u8Input;
	//		LREP_WARNING(__FUNCTION__, "m_u8ButtonInput 0x%02X", m_u8ButtonInput);
		}

		if (m_bCalibReq) {
			if (!(m_u8ButtonCur != 0 && m_u16ButtonRepeat < BUTTON_WAIT_3)) {
//				LREP_WARNING(__FUNCTION__, "Calib");
				if (app_cap1206_set_calibration(CAP1206_CS3_CAL | CAP1206_CS4_CAL | CAP1206_CS5_CAL | CAP1206_CS6_CAL))  m_bCalibReq = false;
			}
		}
	}
	else if (g_ButtonType == BUTTON_TYPE_TS04) {
		m_u8ButtonData[0] = gpio_get_level(BUTTON_SWITCH_1_PIN);
		m_u8ButtonData[1] = gpio_get_level(BUTTON_SWITCH_2_PIN);
		m_u8ButtonData[2] = gpio_get_level(BUTTON_SWITCH_3_PIN);
		m_u8ButtonData[3] = gpio_get_level(BUTTON_SWITCH_4_PIN);
		if (m_bCalibReq) {
			if (!(m_u8ButtonCur != 0 && m_u16ButtonRepeat < BUTTON_WAIT_3)) {
//				LREP_WARNING(__FUNCTION__, "Calib");
				BUTTON_POWER_OFF();
				delay_ms(10);
				BUTTON_POWER_ON();
				m_bCalibReq = false;
			}
		}
	}
}

/*
 *
 */
uint8_t button_analys()
{

	if (g_ButtonType == BUTTON_TYPE_CMBR3108) {
#ifdef DEVICE_TYPE
		if(m_u8ButtonInput & 0x20) return 1;
		else if(m_u8ButtonInput & 0x40) return 2;
		else if(m_u8ButtonInput & 0x80) return 3;
		else return 0;
#endif
	}
	else if (g_ButtonType == BUTTON_TYPE_CAP1206) {
#ifdef DEVICE_TYPE
		if (m_u8ButtonInput & 0x20) return 1;
		else if (m_u8ButtonInput & 0x10) return 2;
		else if (m_u8ButtonInput & 0x08) return 3;
		else return 0;
#endif
	}
	else if (g_ButtonType == BUTTON_TYPE_TS04) {
#ifdef DEVICE_TYPE
		if(m_u8ButtonData[0] == 0)		return 1;
		else if(m_u8ButtonData[1] == 0)	return 2;
		else if(m_u8ButtonData[2] == 0)	return 3;
		else                       		return 0;
#endif
	}
	return 0;
}

/*
 *
 */
uint8_t button_sysState_change(uint8_t u8ButtonCur, uint16_t u16ButtonRepeat)
{
	uint8_t u8changed = 0;
	static bool bAuto = false;
	static bool bManual = false;
	if(u8ButtonCur != 0) {
		if(u16ButtonRepeat == BUTTON_WAIT_1) {
			LREP(__FUNCTION__, "button %d !!!", u8ButtonCur);
			gpio_set_led_normal();
#ifdef DEVICE_TYPE
			if (!(app_is_wifi_mode() ? app_wifi_config_mode_end() : app_ble_mesh_config_end())) {
				uint8_t channel = (u8ButtonCur == BUTTON_SWITCH_1) ? SW_CHANNEL_1 : (u8ButtonCur == BUTTON_SWITCH_2) ? SW_CHANNEL_2 : (u8ButtonCur == BUTTON_SWITCH_3) ? SW_CHANNEL_3 : 0;
				if (app_touch_change_channel_status(channel)) buzzer();
			}
			else buzzer();
#endif
		}
		else if (u16ButtonRepeat == BUTTON_WAIT_2) {
			LREP(__FUNCTION__, "Auto");
			buzzer_config();
			gpio_set_led_auto();
			bAuto = true;
		}
		else if (u16ButtonRepeat == BUTTON_WAIT_3) {
			LREP(__FUNCTION__, "Manual");
			buzzer_config();
			gpio_set_led_manual();
			bAuto = false; bManual = true;
		}
		u8changed=2;
	}
	else {
		if (bAuto) {
			if (app_is_wifi_mode()) g_bRequestBluFi = true;
			else g_bRequestBleMeshProv = true;
		}
		else if (bManual) {
			if (app_is_wifi_mode()) g_bRequestSoftap = true;
		}
		bAuto = false; bManual = false;
	}
	return u8changed;
}

/*
 *
 */
void button_process()
{
	m_u8ButtonPre = m_u8ButtonCur;
	m_u8ButtonCur = button_analys();

	if(m_u8ButtonPre == m_u8ButtonCur) m_u16ButtonRepeat++;
	else m_u16ButtonRepeat = 0;

	if(button_sysState_change(m_u8ButtonCur, m_u16ButtonRepeat) == 1)
		m_u16ButtonRepeat = 0;
}

/*
 *
 */
void button_init()
{
	LREP(__FUNCTION__, "[app_button] button init !!!");
	ESP_ERROR_CHECK(i2c_master_init());

	for (int i=0; i<5; i++){
		if (app_cy8cmbr3xxx_init()) {
			LREP(__FUNCTION__, "CMBR3108 success @ %d try", i + 1);
			g_ButtonType = BUTTON_TYPE_CMBR3108;
			goto DETECT_FINISH;
		}

		if (app_cap1206_init()) {
			LREP(__FUNCTION__, "CAP1206 success @ %d try", i + 1);
			g_ButtonType = BUTTON_TYPE_CAP1206;
			goto DETECT_FINISH;
		}
	}
	i2c_master_deinit();

	button_gpio_config();
	g_ButtonType = BUTTON_TYPE_TS04;

	DETECT_FINISH: LREP_WARNING(__FUNCTION__, "Touch Chip detected %s", (g_ButtonType == BUTTON_TYPE_CMBR3108) ? "CMBR3108" :
			(g_ButtonType == BUTTON_TYPE_CAP1206) ? "CAP1206" : (g_ButtonType == BUTTON_TYPE_TS04) ? "TS04" : "NONE");
	xTaskCreate(task_button, "task_button", 4096, NULL, 3, &xbuttonHandle);
}

/*
 * task process button
 */
void task_button(void *parm)
{
	while(1) {
		LREP(__FUNCTION__, "Button Config");
		if (button_config()) break;
		delay_ms(100);
	}


	static int xCount = 0;
    for (;;)
    {
    	g_bRun_ButtonTask = true;
        button_scans();
        button_process();
        if (xCount++>=2000) {button_recalib(); xCount = 0;}
        delay_ms(10);
    }
}

void 	button_recalib() {
	if (g_ButtonType == BUTTON_TYPE_CMBR3108) return;
	else if (g_ButtonType == BUTTON_TYPE_CAP1206) m_bCalibReq = true;
	else if (g_ButtonType == BUTTON_TYPE_TS04) m_bCalibReq = true;
}

bool	button_config() {
	if (g_ButtonType == BUTTON_TYPE_CMBR3108) return app_cy8cmbr3xxx_config();
	else if (g_ButtonType == BUTTON_TYPE_CAP1206) return app_cap1206_config();
	else if (g_ButtonType == BUTTON_TYPE_TS04) 	return true;
	return false;
}
