
#include "led.h"

#include <string.h>

#include "i2c.h"

#define FRAME_SIZE		192

// ORDER: RGB, RGB, RGB
static const uint8_t lookup[192] = {
		0xB0, 0xA0, 0x90,
		0xB1, 0xA1, 0x91,
		0xB2, 0xA2, 0x92,
		0xB3, 0xA3, 0x93,
		0xB4, 0xA4, 0x94,
		0xB5, 0xA5, 0x95,
		0xB6, 0xA6, 0x96,
		0xB7, 0xA7, 0x97,
		0xB8, 0xA8, 0x98,
		0xB9, 0xA9, 0x99,
		0xBA, 0xAA, 0x9A,
		0xBB, 0xAB, 0x9B,
		0xBC, 0xAC, 0x9C,
		0xBD, 0xAD, 0x9D,
		0xBE, 0xAE, 0x9E,
		0xBF, 0xAF, 0x9F,

		0x80, 0x70, 0x60,
		0x81, 0x71, 0x61,
		0x82, 0x72, 0x62,
		0x83, 0x73, 0x63,
		0x84, 0x74, 0x64,
		0x85, 0x75, 0x65,
		0x86, 0x76, 0x66,
		0x87, 0x77, 0x67,
		0x88, 0x78, 0x68,
		0x89, 0x79, 0x69,
		0x8A, 0x7A, 0x6A,
		0x8B, 0x7B, 0x6B,
		0x8C, 0x7C, 0x6C,
		0x8D, 0x7D, 0x6D,
		0x8E, 0x7E, 0x6E,
		0x8F, 0x7F, 0x6F,

		0x50, 0x40, 0x30,
		0x51, 0x41, 0x31,
		0x52, 0x42, 0x32,
		0x53, 0x43, 0x33,
		0x54, 0x44, 0x34,
		0x55, 0x45, 0x35,
		0x56, 0x46, 0x36,
		0x57, 0x47, 0x37,
		0x58, 0x48, 0x38,
		0x59, 0x49, 0x39,
		0x5A, 0x4A, 0x3A,
		0x5B, 0x4B, 0x3B,
		0x5C, 0x4C, 0x3C,
		0x5D, 0x4D, 0x3D,
		0x5E, 0x4E, 0x3E,
		0x5F, 0x4F, 0x3F,

		0x20, 0x10, 0x00,
		0x21, 0x11, 0x01,
		0x22, 0x12, 0x02,
		0x23, 0x13, 0x03,
		0x24, 0x14, 0x04,
		0x25, 0x15, 0x05,
		0x26, 0x16, 0x06,
		0x27, 0x17, 0x07,
		0x28, 0x18, 0x08,
		0x29, 0x19, 0x09,
		0x2A, 0x1A, 0x0A,
		0x2B, 0x1B, 0x0B,
		0x2C, 0x1C, 0x0C,
		0x2D, 0x1D, 0x0D,
		0x2E, 0x1E, 0x0E,
		0x2F, 0x1F, 0x0F,
};

//static void set(uint8_t *fb, int x, int y, int r, int g, int b);

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

/*
static void set(uint8_t *fb, int x, int y, int r, int g, int b)
{
	int index = 3 * x + 24 * y;
	fb[lookup[index]] = r;
	fb[lookup[index + 1]] = g;
	fb[lookup[index + 2]] = b;
}
*/

