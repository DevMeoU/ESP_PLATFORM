/*
 * i2c.c
 *
 *  Created on: 18 Jan 2021
 *      Author: tongv
 */
#include "I2C.h"

/**
 * @brief i2c master initialization
 */
esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = true;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = true;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void i2c_master_deinit(void) {
	int i2c_master_port = I2C_MASTER_NUM;
	i2c_driver_delete(i2c_master_port);
}
