/*
 * app_button_CY8CMBR3xxx.c
 *
 *  Created on: Jul 21, 2021
 *      Author: laian
 */

#include "app_button_CY8CMBR3xxx.h"
#include "app_common.h"

uint8_t m_cy8cmbr3xxxConfigData[128] = {
		0xE4u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x7Fu, 0x7Fu, 0x80u, 0x7Fu,
		0x7Fu, 0x80u, 0x80u, 0x80u, 0x00u, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x03u, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x80u,
		0x05u, 0x00u, 0x00u, 0x02u, 0x00u, 0x02u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x01u, 0x01u,
		0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x10u, 0x00u, 0x01u, 0x68u,
		0x00u, 0x37u, 0x06u, 0x00u, 0x00u, 0x0Au, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
		0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x78u, 0x58u
};

static i2c_cmd_handle_t cmd;

static void CY8CMBR3xxx_task(void *arg);

bool app_cy8cmbr3xxx_init() {
	static uint8_t data_wr[2] = {0};
	static uint8_t data_rd[2] = {0};

	uint8_t u8FamilyID = 0, u8DevRev = 0;
	uint16_t u16DevId = 0;

	data_wr[0] = 0x00;
	data_wr[1] = 0;
	app_cy8cmbr3xxx_write(data_wr, 2);
	delay_ms(10);

	data_wr[0] = 0x00;
	data_wr[1] = 0;
	app_cy8cmbr3xxx_write(data_wr, 2);
	delay_ms(10);

	data_wr[0] = MBR_REG_FAMILY_ID;
	app_cy8cmbr3xxx_write(data_wr, 1);
	if (app_cy8cmbr3xxx_read(data_rd , 1)) {
		u8FamilyID = data_rd[0];
	}
	delay_ms(10);

	data_wr[0] = MBR_REG_DEVICE_ID;
	app_cy8cmbr3xxx_write(data_wr, 1);
	if (app_cy8cmbr3xxx_read(data_rd, 2)) {
		u16DevId = ((uint16_t)data_rd[1] << 8) + data_rd[0];
	}
	delay_ms(10);

	data_wr[0] = MBR_REG_DEVICE_REV;
	app_cy8cmbr3xxx_write(data_wr, 1);
	if (app_cy8cmbr3xxx_read(data_rd , 1)) {
		u8DevRev = data_rd[0];
	}
	delay_ms(10);

	LREP_WARNING(__FUNCTION__, "Family ID %d, Device ID %d, Device Rev %d", u8FamilyID, u16DevId, u8DevRev);

	return (u8FamilyID == MBR_FAMILY_ID && u16DevId == MBR3108_DEVICE_ID);
}

bool app_cy8cmbr3xxx_config() {
	LREP(__FUNCTION__, "CY8CMBR3116_config");
	delay_ms(10);
	int i=0;
	uint8_t data_wr[200] = {0};
	data_wr[0] = 0x00;
	data_wr[1] = 0;
	app_cy8cmbr3xxx_write(data_wr, 2);
	LREP(__FUNCTION__, "Step 1 ok");

	delay_ms(10);
	app_cy8cmbr3xxx_write(data_wr, 2);
	LREP(__FUNCTION__, "Step 2 ok");

	delay_ms(10);
	data_wr[0]=MBR_REG_REGMAP_ORIGIN;
	for(i=0;i<128;i++) data_wr[i+1]= m_cy8cmbr3xxxConfigData[i];
	if (!app_cy8cmbr3xxx_write(data_wr, 129)) return false;
	LREP(__FUNCTION__, "Step 3 ok");

	delay_ms(10);
	data_wr[0] = MBR_REG_CTRL_CMD;
	data_wr[1] = MBR_CMD_SAVE_CHECK_CRC;
	if (!app_cy8cmbr3xxx_write(data_wr, 2)) return false;
	LREP(__FUNCTION__, "Step 4 ok");

	delay_ms(200);
	data_wr[0] = MBR_REG_CTRL_CMD;
	data_wr[1] = MBR_CMD_SW_RESET;
	if (!app_cy8cmbr3xxx_write(data_wr, 2)) return false;
	LREP(__FUNCTION__, "Step 5 ok");

	delay_ms(200);
	LREP(__FUNCTION__, "CY8CMBR3116_config END");
	return true;
}

bool app_cy8cmbr3xxx_write(const uint8_t *pData, int size) {
	int ret = ESP_OK;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (MBR_ADDRESS << 1) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write(cmd, pData, size, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret == ESP_OK;
}

bool app_cy8cmbr3xxx_read(uint8_t *pData, int size) {
	int ret = ESP_OK;
	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, MBR_ADDRESS << 1 | READ_BIT, ACK_CHECK_EN);
	if (size > 1)
		i2c_master_read(cmd, pData, size - 1, ACK_VAL);
	i2c_master_read_byte(cmd, pData + size - 1, NACK_VAL);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret == ESP_OK;
}

void CY8CMBR3xxx_task(void *arg) {
	int ret;
	uint8_t data_wr[20] = {0};
	uint8_t data_rd[20] = {0};

	while(1) {
		data_wr[0] = 0xAE;
		ret = app_cy8cmbr3xxx_write(data_wr, 1);

		data_wr[0] = 0xAA;
		ret = app_cy8cmbr3xxx_write(data_wr, 1);

		ret = app_cy8cmbr3xxx_read(data_rd, 2);
		if (ret == ESP_ERR_TIMEOUT) {
			LREP_ERROR(__FUNCTION__, "I2C Timeout");
		} else if (ret == ESP_OK) {
			if(data_rd[0]==0x08) ESP_LOGW(__FUNCTION__, "BTN1") ;
			if(data_rd[0]==0x10) ESP_LOGW(__FUNCTION__, "BTN2");
			if(data_rd[0]==0x20) ESP_LOGW(__FUNCTION__, "BTN3");
			if(data_rd[0]==0x40) ESP_LOGW(__FUNCTION__, "BTN4");
		} else {
			LREP_WARNING(__FUNCTION__, "%s: No ack, sensor not connected...skip...", esp_err_to_name(ret));
		}
		vTaskDelay(10 / portTICK_RATE_MS);
	}
}


