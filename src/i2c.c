
#include "i2c.h"


void i2c_write(I2C_TypeDef *I2Cx, uint8_t subAddr, uint8_t *data, uint8_t len)
{
    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_HandleTransfer(I2Cx, 0x30, LL_I2C_ADDRSLAVE_7BIT, 1+len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
    while (!LL_I2C_IsActiveFlag_TXE(I2Cx));
    LL_I2C_TransmitData8(I2Cx, subAddr);
    for (size_t i = 0; i < len; i++) {
        while (!LL_I2C_IsActiveFlag_TXE(I2Cx));
        LL_I2C_TransmitData8(I2Cx, data[i]);
    }
    while (/*!LL_I2C_IsActiveFlag_TC(I2Cx) && !LL_I2C_IsActiveFlag_NACK(I2Cx) &&*/ !LL_I2C_IsActiveFlag_STOP(I2Cx));
    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_ClearFlag_NACK(I2Cx);
}

void i2c_read(I2C_TypeDef *I2Cx, uint8_t subAddr, uint8_t *data, uint8_t len)
{
    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_HandleTransfer(I2Cx, 0x30, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
    LL_I2C_TransmitData8(I2Cx, subAddr);
    while (!LL_I2C_IsActiveFlag_TC(I2Cx));
    LL_I2C_HandleTransfer(I2Cx, 0x30, LL_I2C_ADDRSLAVE_7BIT, len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
    for (size_t i = 0; i < len; i++) {
    	while (!LL_I2C_IsActiveFlag_RXNE(I2Cx) && !LL_I2C_IsActiveFlag_NACK(I2Cx) && !LL_I2C_IsActiveFlag_STOP(I2Cx));
    	data[i] = LL_I2C_ReceiveData8(I2Cx);
    }

    while (/*!LL_I2C_IsActiveFlag_TC(I2Cx) && !LL_I2C_IsActiveFlag_NACK(I2Cx) &&*/ !LL_I2C_IsActiveFlag_STOP(I2Cx));
    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_ClearFlag_NACK(I2Cx);
}
