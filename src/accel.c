
#include "accel.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "i2c.h"

#define I2Cx		I2C3

#define INT1_PIN	LL_GPIO_PIN_0
#define INT1_PORT	GPIOA

// TODO: change back to PC13
#define INT2_PIN	LL_GPIO_PIN_2
#define INT2_PORT	GPIOA

void accel_Init(void)
{
	// config INT1/INT2 pins
}

void accel_config_asleep(void)
{
    uint8_t click_config[] = {
    		0x2A,		// CLICK_CFG = enable double tap on XYZ
			0x00,		// read-only CLICK_SRC
			0x0C,		// CLICK_THS = threshold
			0x08,		// TIME_LIMIT
			0x04,		// TIME_LATENCY
			0x08,		// TIME_WINDOW
    };
    i2c_write(I2Cx, ACCEL_ADDR, 0x80 | 0x38, click_config, 6);

    uint8_t accel_config[] = {
    		0x4F,		// CTRL_REG1 = 50Hz, low power mode, XYZ on
			0x84,		// CTRL_REG2 = normal high-pass filter, 1Hz cutoff, enabled only for CLICK
			0x00,		// CTRL_REG3 = no interrupt on INT1
			0x00,		// CTRL_REG4
			0x00, 		// CTRL_REG5
			0x80,		// CTRL_REG6 = click interrupt on INT2, interrupt active high
    };
    i2c_write(I2Cx, ACCEL_ADDR, 0x80 | 0x20, accel_config, 6);
}

void accel_config_awake(void)
{
    uint8_t click_config[] = {
    		0x15,		// CLICK_CFG = enable single tap on XYZ
			0x00,		// read-only CLICK_SRC
			0x10,		// CLICK_THS = threshold
			0x08,		// TIME_LIMIT
			0x01,		// TIME_LATENCY
			0x08,		// TIME_WINDOW
    };
    i2c_write(I2Cx, ACCEL_ADDR, 0x80 | 0x38, click_config, 6);

    uint8_t accel_config[] = {
    		0x4F,		// CTRL_REG1 = 50Hz, low power mode, XYZ on
			0x84,		// CTRL_REG2 = normal high-pass filter, 1Hz cutoff, enabled only for CLICK
			0x10,		// CTRL_REG3 = data ready interrupt on INT1
			0x00,		// CTRL_REG4
			0x00, 		// CTRL_REG5
			0x80,		// CTRL_REG6 = click interrupt on INT2, interrupt active high
    };
    i2c_write(I2Cx, ACCEL_ADDR, 0x80 | 0x20, accel_config, 6);
}


