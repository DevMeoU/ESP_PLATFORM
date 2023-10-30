/*
 * app_button_CAP1206.c
 *
 *  Created on: Jul 16, 2021
 *      Author: laian
 */

#include "app_button_CAP1206.h"
#include "app_common.h"

static i2c_cmd_handle_t cmd;

static void CAP1206_task(void *arg);

bool app_cap1206_init(void) {
	uint8_t PID=0, MID=0, REV=0;

	app_cap1206_read_byte(CAP1206_REG_PID, &PID);
	app_cap1206_read_byte(CAP1206_REG_MID, &MID);
	app_cap1206_read_byte(CAP1206_REG_REV, &REV);
	LREP_WARNING(__FUNCTION__, "PID 0x%02x, MID 0x%02x, REV 0x%02x", PID, MID, REV);

	bool ret = (PID == 0x67 && MID == 0x5D);

	//	xTaskCreate(CAP1206_task, "CAP1206_task", 4096, 0, 3, NULL);
	return ret;
}

bool app_cap1206_config(void) {
	if (!app_cap1206_set_sensitive(CAP1206_DELTA_SENSE_128)) return false;
	if (!app_cap1206_set_input_enable(CAP1206_CS3_ENABLE | CAP1206_CS4_ENABLE | CAP1206_CS5_ENABLE | CAP1206_CS6_ENABLE)) return false;
	if (!app_cap1206_set_input_config(CAP1206_MAX_DUR_11200, CAP1206_RPT_RATE_35, CAP1206_M_PRESS_35)) return false;
	if (!app_cap1206_set_average_sample_config(CAP1206_AVG_8, CAP1206_SAMP_TIME_640us, CAP1206_CYCLE_TIME_35)) return false;
	if (!app_cap1206_set_input_threshold(3, 40)) return false;
	if (!app_cap1206_set_input_threshold(4, 40)) return false;
	if (!app_cap1206_set_input_threshold(5, 40)) return false;
	if (!app_cap1206_set_input_threshold(6, 40)) return false;
	if (!app_cap1206_set_multiple_touch_config(CAP1206_MUL_BLK_DIS)) return false;

	return true;
}

bool app_cap1206_set_sensitive(uint8_t sensitive) {
	uint8_t SensitiveControl = 0, DeltaSense = 0, BaseShift = 0;
	if (!app_cap1206_read_byte(CAP1206_REG_SENSITIVE, &SensitiveControl)) return false;
	DeltaSense = SensitiveControl >> 4;
	BaseShift = SensitiveControl & 0x0F;
	LREP_WARNING(__FUNCTION__, "Sensitive 0x%02x, DeltaSense 0x%02X", SensitiveControl, DeltaSense);

	if (DeltaSense != sensitive) {
		DeltaSense = sensitive; SensitiveControl = (DeltaSense <<4) | BaseShift;
		if (!app_cap1206_write_byte(CAP1206_REG_SENSITIVE, SensitiveControl)) return false;
	}
	return true;
}

bool app_cap1206_set_input_enable(uint8_t inputEnable) {
	if (!app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_ENABLE, inputEnable)) return false;
	return true;
}

bool app_cap1206_set_input_config(uint8_t max_dur, uint8_t rpt_rate, uint8_t m_press) { /*CAP1206_MAX_DUR_xxx, CAP1206_RPT_RATE_xx, CAP1206_M_PRESS_xx*/
	if (!app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_CONF, (max_dur << 4) | rpt_rate)) return false;
	if (!app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_CONF_2, m_press)) return false;
	return true;
}

bool app_cap1206_set_average_sample_config(uint8_t avg, uint8_t samp_time, uint8_t cycle_time) { /*CAP1206_AVG_x, CAP1206_SAMP_TIME_x, CAP1206_CYCLE_TIME_x*/
	uint8_t config = (avg << 4) | (samp_time << 2) | (cycle_time);
	if (!app_cap1206_write_byte(CAP1206_REG_AVERAGE_SAMPLE_CONFIG, config)) return false;
	return true;
}

bool app_cap1206_set_input_threshold(int channel, uint8_t threshold) {
	if (channel == 1 && app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_1_THRESHOLD, threshold)) return true;
	if (channel == 2 && app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_2_THRESHOLD, threshold)) return true;
	if (channel == 3 && app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_3_THRESHOLD, threshold)) return true;
	if (channel == 4 && app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_4_THRESHOLD, threshold)) return true;
	if (channel == 5 && app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_5_THRESHOLD, threshold)) return true;
	if (channel == 6 && app_cap1206_write_byte(CAP1206_REG_SENSOR_INPUT_6_THRESHOLD, threshold)) return true;
	return false;
}

bool app_cap1206_set_multiple_touch_config(int mult_blk_en) {
	if (!app_cap1206_write_byte(CAP1206_REG_MULTIPLE_TOUCH_CONFIG, mult_blk_en << 7)) return false;
	return true;
}

bool app_cap1206_set_calibration(uint8_t active) {
	if (!app_cap1206_write_byte(CAP1206_REG_CALIB_ACTIVE_STATUS, active)) return false;
	return true;
}

bool app_cap1206_write_byte(uint8_t reg, uint8_t data) {
	int ret = ESP_OK;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	{
		i2c_master_write_byte(cmd, (CAP1206_ADDRESS << 1) | WRITE_BIT, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
	}
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret != ESP_OK) LREP_ERROR(__FUNCTION__,"");
	return ret == ESP_OK;
}

bool app_cap1206_read_byte(uint8_t reg, uint8_t* pData) {
	if (app_cap1206_smbus_send_byte(reg) == false) return false;
	return app_cap1206_smbus_recv_byte(pData);
}

bool app_cap1206_smbus_send_byte(uint8_t reg) {
	int ret = ESP_OK;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	{
		i2c_master_write_byte(cmd, (CAP1206_ADDRESS << 1) | WRITE_BIT, ACK_CHECK_EN);
		i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
	}
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret != ESP_OK) LREP_ERROR(__FUNCTION__,"");
	return ret == ESP_OK;
}

bool app_cap1206_smbus_recv_byte(uint8_t* pData) {
	int ret = ESP_OK;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	{
		i2c_master_write_byte(cmd, (CAP1206_ADDRESS << 1) | READ_BIT, ACK_CHECK_EN);
		i2c_master_read_byte(cmd, pData, I2C_MASTER_NACK);
	}
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret != ESP_OK) LREP_ERROR(__FUNCTION__,"");
	return ret == ESP_OK;
}

void CAP1206_task(void *arg) {
	uint8_t u8R_MainControl = 0, u8R_GeneralStatus = 0, u8R_SensorInputStatus = 0;
	uint8_t PID=0, MID=0, REV=0;

	app_cap1206_read_byte(CAP1206_REG_PID, &PID);
	app_cap1206_read_byte(CAP1206_REG_MID, &MID);
	app_cap1206_read_byte(CAP1206_REG_REV, &REV);
	LREP(__FUNCTION__, "PID 0x%02x, MID 0x%02x, REV 0x%02x", PID, MID, REV);

	while (1) {
		if (!app_cap1206_read_byte(CAP1206_REG_MAIN_CONTROL, &u8R_MainControl)) LREP_ERROR(__FUNCTION__,"1");
		if (!app_cap1206_read_byte(CAP1206_REG_GENERAL_STATUS, &u8R_GeneralStatus)) LREP_ERROR(__FUNCTION__,"2");
		if (!app_cap1206_read_byte(CAP1206_REG_SENSOR_INPUT_STATUS, &u8R_SensorInputStatus)) LREP_ERROR(__FUNCTION__,"3");
		if (u8R_SensorInputStatus) {
			app_cap1206_write_byte(CAP1206_REG_MAIN_CONTROL, u8R_MainControl & 0xFE);
		}
		if (u8R_SensorInputStatus) {
			LREP(__FUNCTION__, "Main control 0x%02x, General status 0x%02x, Sensor Input 0x%02x", u8R_MainControl, u8R_GeneralStatus, u8R_SensorInputStatus);
		}
		else {
			LREP_WARNING(__FUNCTION__, "Main control 0x%02x, General status 0x%02x, Sensor Input 0x%02x", u8R_MainControl, u8R_GeneralStatus, u8R_SensorInputStatus);
		}

		delay_ms(10);
	}
}

