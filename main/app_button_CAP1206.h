/*
 * app_button_CAP1206.h
 *
 *  Created on: Jul 16, 2021
 *      Author: laian
 */

#ifndef __APP_BUTTON_CAP1206_H
#define __APP_BUTTON_CAP1206_H

#include "sdkconfig.h"
#include "config.h"
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "I2C.h"

bool app_cap1206_init(void);
bool app_cap1206_config(void);

bool app_cap1206_set_sensitive(uint8_t sensitive); /*CAP1206_DELTA_SENSE_xx*/
bool app_cap1206_set_input_enable(uint8_t inputEnable); /*CAP1206_CSx_ENABLE*/
bool app_cap1206_set_input_config(uint8_t max_dur, uint8_t rpt_rate, uint8_t m_press); /*CAP1206_MAX_DUR_xxx, CAP1206_RPT_RATE_xx, CAP1206_M_PRESS_xx*/
bool app_cap1206_set_average_sample_config(uint8_t avg, uint8_t samp_time, uint8_t cycle_time); /*CAP1206_AVG_x, CAP1206_SAMP_TIME_x, CAP1206_CYCLE_TIME_x*/
bool app_cap1206_set_input_threshold(int channel, uint8_t threshold);
bool app_cap1206_set_multiple_touch_config(int mult_blk_en);
bool app_cap1206_set_calibration(uint8_t active); /*CAP1206_CSx_CAL*/

bool app_cap1206_write_byte(uint8_t reg, uint8_t data);
bool app_cap1206_read_byte(uint8_t reg, uint8_t* pData);
bool app_cap1206_smbus_send_byte(uint8_t reg);
bool app_cap1206_smbus_recv_byte(uint8_t* pData);

#define CAP1206_ADDRESS 0x28 //I2C 7-bit address

#define CAP1206_REG_MAIN_CONTROL				0x00
#define CAP1206_REG_GENERAL_STATUS				0x02
#define CAP1206_REG_SENSOR_INPUT_STATUS			0x03
#define CAP1206_REG_NOISE_FLAG_STATUS			0x0A

#define CAP1206_REG_SENSOR_INPUT_1_DELTA_COUNT	0x10
#define CAP1206_REG_SENSOR_INPUT_2_DELTA_COUNT	0x11
#define CAP1206_REG_SENSOR_INPUT_3_DELTA_COUNT	0x12
#define CAP1206_REG_SENSOR_INPUT_4_DELTA_COUNT	0x13
#define CAP1206_REG_SENSOR_INPUT_5_DELTA_COUNT	0x14
#define CAP1206_REG_SENSOR_INPUT_6_DELTA_COUNT	0x15

#define CAP1206_REG_SENSITIVE					0x1F
#define CAP1206_REG_CONFIGURATION				0x20
#define CAP1206_REG_SENSOR_INPUT_ENABLE			0x21
#define CAP1206_REG_SENSOR_INPUT_CONF			0x22
#define CAP1206_REG_SENSOR_INPUT_CONF_2			0x23
#define CAP1206_REG_AVERAGE_SAMPLE_CONFIG		0x24
#define CAP1206_REG_CALIB_ACTIVE_STATUS			0x26
#define CAP1206_REG_INTERRUPT_ENABLE			0x27

#define CAP1206_REG_MULTIPLE_TOUCH_CONFIG		0x2A

#define CAP1206_REG_SENSOR_INPUT_1_THRESHOLD	0x30
#define CAP1206_REG_SENSOR_INPUT_2_THRESHOLD	0x31
#define CAP1206_REG_SENSOR_INPUT_3_THRESHOLD	0x32
#define CAP1206_REG_SENSOR_INPUT_4_THRESHOLD	0x33
#define CAP1206_REG_SENSOR_INPUT_5_THRESHOLD	0x34
#define CAP1206_REG_SENSOR_INPUT_6_THRESHOLD	0x35

#define CAP1206_REG_PID						0xFD
#define CAP1206_REG_MID						0xFE
#define CAP1206_REG_REV						0xFF

/*CAP1206_DELTA_SENSE_xx*/
#define CAP1206_DELTA_SENSE_128				0b000
#define CAP1206_DELTA_SENSE_64				0b001
#define CAP1206_DELTA_SENSE_32				0b010	//default
#define CAP1206_DELTA_SENSE_16				0b011
#define CAP1206_DELTA_SENSE_8				0b100
#define CAP1206_DELTA_SENSE_4				0b101
#define CAP1206_DELTA_SENSE_2				0b110
#define CAP1206_DELTA_SENSE_1				0b111

/*CAP1206_CSx_ENABLE*/
#define CAP1206_CS1_ENABLE					0x01
#define CAP1206_CS2_ENABLE					0x02
#define CAP1206_CS3_ENABLE					0x04
#define CAP1206_CS4_ENABLE					0x08
#define CAP1206_CS5_ENABLE					0x10
#define CAP1206_CS6_ENABLE					0x20

/*CAP1206_MAX_DUR_xxx*/
#define CAP1206_MAX_DUR_560					0x00
#define CAP1206_MAX_DUR_840					0x01
#define CAP1206_MAX_DUR_1120				0x02
#define CAP1206_MAX_DUR_1400				0x03
#define CAP1206_MAX_DUR_1680				0x04
#define CAP1206_MAX_DUR_2240				0x05
#define CAP1206_MAX_DUR_2800				0x06
#define CAP1206_MAX_DUR_3360				0x07
#define CAP1206_MAX_DUR_3920				0x08
#define CAP1206_MAX_DUR_4480				0x09
#define CAP1206_MAX_DUR_5600				0x0A	//default
#define CAP1206_MAX_DUR_6720				0x0B
#define CAP1206_MAX_DUR_7840				0x0C
#define CAP1206_MAX_DUR_8906				0x0D
#define CAP1206_MAX_DUR_10080				0x0E
#define CAP1206_MAX_DUR_11200				0x0F

/*CAP1206_RPT_RATE_xx*/
#define CAP1206_RPT_RATE_35					0x00
#define CAP1206_RPT_RATE_70					0x01
#define CAP1206_RPT_RATE_105				0x02
#define CAP1206_RPT_RATE_140				0x03
#define CAP1206_RPT_RATE_175				0x04	//default
#define CAP1206_RPT_RATE_210				0x05
#define CAP1206_RPT_RATE_245				0x06
#define CAP1206_RPT_RATE_280				0x07
#define CAP1206_RPT_RATE_315				0x08
#define CAP1206_RPT_RATE_350				0x09
#define CAP1206_RPT_RATE_385				0x0A
#define CAP1206_RPT_RATE_420				0x0B
#define CAP1206_RPT_RATE_455				0x0C
#define CAP1206_RPT_RATE_490				0x0D
#define CAP1206_RPT_RATE_525				0x0E
#define CAP1206_RPT_RATE_560				0x0F

/*CAP1206_M_PRESS_xx*/
#define CAP1206_M_PRESS_35					0x00
#define CAP1206_M_PRESS_70					0x01
#define CAP1206_M_PRESS_105					0x02
#define CAP1206_M_PRESS_140					0x03
#define CAP1206_M_PRESS_175					0x04
#define CAP1206_M_PRESS_210					0x05
#define CAP1206_M_PRESS_245					0x06
#define CAP1206_M_PRESS_280					0x07
#define CAP1206_M_PRESS_315					0x08
#define CAP1206_M_PRESS_350					0x09
#define CAP1206_M_PRESS_385					0x0A
#define CAP1206_M_PRESS_420					0x0B
#define CAP1206_M_PRESS_455					0x0C
#define CAP1206_M_PRESS_490					0x0D
#define CAP1206_M_PRESS_525					0x0E
#define CAP1206_M_PRESS_560					0x0F

/*CAP1206_AVG_x*/
#define CAP1206_AVG_1						0x00
#define CAP1206_AVG_2						0x01
#define CAP1206_AVG_4						0x02
#define CAP1206_AVG_8						0x03 //default
#define CAP1206_AVG_16						0x04
#define CAP1206_AVG_32						0x05
#define CAP1206_AVG_64						0x06
#define CAP1206_AVG_128						0x07

/*CAP1206_SAMP_TIME_x*/
#define CAP1206_SAMP_TIME_320us				0x00
#define CAP1206_SAMP_TIME_640us				0x01
#define CAP1206_SAMP_TIME_1280us			0x02 //default
#define CAP1206_SAMP_TIME_2560us			0x03

/*CAP1206_CYCLE_TIME_x*/
#define CAP1206_CYCLE_TIME_35				0x00
#define CAP1206_CYCLE_TIME_70				0x01 //default
#define CAP1206_CYCLE_TIME_105				0x02
#define CAP1206_CYCLE_TIME_140				0x03

/*CAP1206_MUL_BLK_x*/
#define CAP1206_MUL_BLK_EN					0x01
#define CAP1206_MUL_BLK_DIS					0x00

/*CAP1206_CSx_CAL*/
#define CAP1206_CS1_CAL						0x01
#define CAP1206_CS2_CAL						0x02
#define CAP1206_CS3_CAL						0x04
#define CAP1206_CS4_CAL						0x08
#define CAP1206_CS5_CAL						0x10
#define CAP1206_CS6_CAL						0x20


#endif /* __APP_BUTTON_CAP1206_H */
