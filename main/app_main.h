/*
 * app_main.h
 *
 *  Created on: Dec 16, 2019
 *      Author: Admin
 */

#ifndef MAIN_APP_MAIN_H_
#define MAIN_APP_MAIN_H_

#include <string.h>
#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"
#include "driver/gpio.h"

#define TIMER_DIVIDER         16  								//  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds
#define TIMER_INTERVAL0_SEC   1									// sample test interval for the first timer
#define TEST_WITHOUT_RELOAD   0        							// testing will be done without auto reload
#endif /* MAIN_APP_MAIN_H_ */
