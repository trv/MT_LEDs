#include <stdint.h>

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

void i2c_write(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t subAddr, uint8_t *data, uint8_t len);
void i2c_read(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t subAddr, volatile uint8_t *data, uint8_t len);

void i2c_initNB(I2C_TypeDef *I2Cx);
void i2c_writeNB(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t subAddr, uint8_t *data, uint8_t len);
