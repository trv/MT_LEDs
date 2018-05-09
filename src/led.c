
#include "led.h"

#include <string.h>

#include "i2c.h"

void LED_Init(
		struct LED *l,
		I2C_TypeDef *I2Cx,
		uint8_t devAddr,
		GPIO_TypeDef *GPIOx,
		uint32_t SHDN_Pin)
{
	l->I2Cx = I2Cx;
	l->devAddr = devAddr;
	l->GPIOx = GPIOx;
	l->SHDN_Pin = SHDN_Pin;

	uint8_t cfg[32] = {0};

	// unlock and switch to page 3
	cfg[0] = 0xC5;
	i2c_write(l->I2Cx, l->devAddr, 0xFE, cfg, 1);
	cfg[0] = 0x03;
	i2c_write(l->I2Cx, l->devAddr, 0xFD, cfg, 1);

	// read reset register to reset everything
	i2c_read(l->I2Cx, l->devAddr, 0x11, cfg, 1);

	// set config and global current
	cfg[0] = 0x01;	// TODO: sync enable?
	cfg[1] = 0x01;	// global brightness
	i2c_write(l->I2Cx, l->devAddr, 0x00, cfg, 2);

	// unlock and switch to page 0
	cfg[0] = 0xC5;
	i2c_write(l->I2Cx, l->devAddr, 0xFE, cfg, 1);
	cfg[0] = 0x00;
	i2c_write(l->I2Cx, l->devAddr, 0xFD, cfg, 1);

	// enable all channels
	memset(cfg, 0xFF, sizeof(cfg));
	i2c_write(l->I2Cx, l->devAddr, 0x00, cfg, 24);

	// unlock and switch to page 1
	cfg[0] = 0xC5;
	i2c_write(l->I2Cx, l->devAddr, 0xFE, cfg, 1);
	cfg[0] = 0x01;
	i2c_write(l->I2Cx, l->devAddr, 0xFD, cfg, 1);

	// release shutdown pin
	LL_GPIO_SetOutputPin(l->GPIOx, l->SHDN_Pin);
}

void LED_Update(
		struct LED *l,
		uint8_t *fb)
{
	i2c_writeNB(l->I2Cx, l->devAddr, 0x00, fb, 192, NULL, NULL);
}

void LED_SetBrightness(
		struct LED *l,
		uint8_t brightness)
{
	// unlock and switch to page 3
	uint8_t cfg = 0xC5;
	i2c_write(l->I2Cx, l->devAddr, 0xFE, &cfg, 1);
	cfg = 0x03;
	i2c_write(l->I2Cx, l->devAddr, 0xFD, &cfg, 1);

	// set global current
	cfg = brightness;	// global brightness
	i2c_write(l->I2Cx, l->devAddr, 0x01, &cfg, 1);

	// unlock and switch to page 1
	cfg = 0xC5;
	i2c_write(l->I2Cx, l->devAddr, 0xFE, &cfg, 1);
	cfg = 0x01;
	i2c_write(l->I2Cx, l->devAddr, 0xFD, &cfg, 1);
}
