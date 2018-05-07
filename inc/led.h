
#include <stdint.h>

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

struct LED {
	I2C_TypeDef *I2Cx;
	GPIO_TypeDef *GPIOx;	// for SHDN pin
	uint32_t SHDN_Pin;		// for SHDN pin
	uint8_t devAddr;		// I2C device address
	uint8_t displayOffset;	// offset into frame buffer
	uint8_t fb[192*2];		// double-buffered frame data
};

void LED_Init(
		struct LED *l,
		I2C_TypeDef *I2Cx,
		uint8_t devAddr,
		GPIO_TypeDef *GPIOx,
		uint32_t SHDN_Pin);

void LED_Update(
		struct LED *l,
		void (f(int x, int y, uint8_t *color)));
