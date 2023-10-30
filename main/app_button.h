/*
 * app_button.h
 *
 *  Created on: Dec 13, 2019
 *      Author: Admin
 */

#ifndef MAIN_APP_BUTTON_H_
#define MAIN_APP_BUTTON_H_

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUTTON_SWITCH_1			1
#define BUTTON_SWITCH_2			2
#define BUTTON_SWITCH_3			3
#define BUTTON_SWITCH_4			4
#define BUTTON_SWITCH_1_PIN		GPIO_NUM_21
#define BUTTON_SWITCH_2_PIN		GPIO_NUM_19
#define BUTTON_SWITCH_3_PIN		GPIO_NUM_18
#define BUTTON_SWITCH_4_PIN		GPIO_NUM_5

#define	BUTTON_PIN_SEL			((1ULL << BUTTON_SWITCH_1_PIN) | (1ULL << BUTTON_SWITCH_2_PIN) | (1ULL << BUTTON_SWITCH_3_PIN)| (1ULL << BUTTON_SWITCH_4_PIN))

#define BUTTON_POWER_CONTROL_PIN	GPIO_NUM_22

#define	BUTTON_POWER_ON()	gpio_set_level(BUTTON_POWER_CONTROL_PIN, 1)
#define	BUTTON_POWER_OFF()	gpio_set_level(BUTTON_POWER_CONTROL_PIN, 0)

#define	BUTTON_WAIT_1			5
#define BUTTON_WAIT_1_2			100
#define	BUTTON_WAIT_2			300//500
#define BUTTON_WAIT_3			500//1000
#define	BUTTON_WAIT_4			2000

#define	BUTTON_NUMBER			5

typedef enum {
	BUTTON_TYPE_TS04,
	BUTTON_TYPE_CAP1206,
	BUTTON_TYPE_CMBR3108,
	BUTTON_TYPE_MAX,
} eButtonType_t;

void 	button_gpio_config();
void 	button_scans(void);
uint8_t button_analys(void);
uint8_t button_sysState_change(uint8_t u8ButtonCur, uint16_t u16ButtonRepeat);
void 	button_process(void);
void 	button_init();
void 	button_recalib();
bool	button_config();

#endif /* MAIN_APP_BUTTON_H_ */
