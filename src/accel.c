
#include "accel.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

void accel_config_asleep(void)
{
    uint8_t click_config[] = {
    		0x3F,		// CLICK_CFG = enable single and double tap on XYZ (0x3F)
			0x00,		// read-only CLICK_SRC
			0x08,		// CLICK_THS = threshold
			0x08,		// TIME_LIMIT
			0x08,		// TIME_LATENCY
			0x08,		// TIME_WINDOW
    };
    accel_write(0x38, click_config, 6);

    uint8_t accel_config[] = {
    		0x4F,		// CTRL_REG1 = 50Hz, low power mode, XYZ on
			0x84,		// CTRL_REG2 = normal high-pass filter, 1Hz cutoff, enabled only for CLICK
			0x00,		// CTRL_REG3 = no interrupt on INT1
			0x00,		// CTRL_REG4
			0x00, 		// CTRL_REG5
			0x80,		// CTRL_REG6 = click interrupt on INT2, interrupt active high
    };
    accel_write(0x20, accel_config, 6);
}

void accel_config_awake(void)
{
    uint8_t click_config[] = {
    		0x3F,		// CLICK_CFG = enable single and double tap on XYZ (0x3F)
			0x00,		// read-only CLICK_SRC
			0x08,		// CLICK_THS = threshold
			0x08,		// TIME_LIMIT
			0x08,		// TIME_LATENCY
			0x08,		// TIME_WINDOW
    };
    accel_write(0x38, click_config, 6);

    uint8_t accel_config[] = {
    		0x4F,		// CTRL_REG1 = 50Hz, low power mode, XYZ on
			0x84,		// CTRL_REG2 = normal high-pass filter, 1Hz cutoff, enabled only for CLICK
			0x10,		// CTRL_REG3 = data ready interrupt on INT1
			0x00,		// CTRL_REG4
			0x00, 		// CTRL_REG5
			0x80,		// CTRL_REG6 = click interrupt on INT2, interrupt active high
    };
    accel_write(0x20, accel_config, 6);
}

void accel_write(uint8_t subAddr, uint8_t *data, uint8_t len)
{
    LL_I2C_ClearFlag_STOP(I2C3);
    LL_I2C_HandleTransfer(I2C3, 0x30, LL_I2C_ADDRSLAVE_7BIT, 1+len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
    while (!LL_I2C_IsActiveFlag_TXE(I2C3));
    LL_I2C_TransmitData8(I2C3, 0x80 | subAddr); 	// set MSB to enable auto-increment
    for (size_t i = 0; i < len; i++) {
        while (!LL_I2C_IsActiveFlag_TXE(I2C3));
        LL_I2C_TransmitData8(I2C3, data[i]);
    }
    while (/*!LL_I2C_IsActiveFlag_TC(I2C3) && !LL_I2C_IsActiveFlag_NACK(I2C3) &&*/ !LL_I2C_IsActiveFlag_STOP(I2C3));
    LL_I2C_ClearFlag_STOP(I2C3);
    LL_I2C_ClearFlag_NACK(I2C3);
}

void accel_read(uint8_t subAddr, uint8_t *data, uint8_t len)
{
    LL_I2C_ClearFlag_STOP(I2C3);
    LL_I2C_HandleTransfer(I2C3, 0x30, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
    LL_I2C_TransmitData8(I2C3, 0x80 | subAddr);	// set MSB to enable auto-increment
    while (!LL_I2C_IsActiveFlag_TC(I2C3));
    LL_I2C_HandleTransfer(I2C3, 0x30, LL_I2C_ADDRSLAVE_7BIT, len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
    for (size_t i = 0; i < len; i++) {
    	while (!LL_I2C_IsActiveFlag_RXNE(I2C3) && !LL_I2C_IsActiveFlag_NACK(I2C3) && !LL_I2C_IsActiveFlag_STOP(I2C3));
    	data[i] = LL_I2C_ReceiveData8(I2C3);
    }

    while (/*!LL_I2C_IsActiveFlag_TC(I2C3) && !LL_I2C_IsActiveFlag_NACK(I2C3) &&*/ !LL_I2C_IsActiveFlag_STOP(I2C3));
    LL_I2C_ClearFlag_STOP(I2C3);
    LL_I2C_ClearFlag_NACK(I2C3);
}
