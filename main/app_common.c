/*
 * app_common.c
 *
 *  Created on: Jan 31, 2020
 *      Author: Admin
 */

#include "app_common.h"
#include "config.h"

void delay_ms(int ms)
{
	vTaskDelay(ms/portTICK_PERIOD_MS);
}

void delay_us(int us)
{
	ets_delay_us(us);
}
