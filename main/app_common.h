/*
 * app_common.h
 *
 *  Created on: Jan 31, 2020
 *      Author: Admin
 */

#ifndef MAIN_APP_COMMON_H_
#define MAIN_APP_COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_system.h"

void delay_ms(int ms);
void delay_us(int us);

#endif /* MAIN_APP_COMMON_H_ */
