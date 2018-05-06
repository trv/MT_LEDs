
#include "i2c.h"

struct transfer {
	uint8_t len;
	uint8_t i;
	uint8_t *data;
};

struct transfer t;

void i2c_write(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t subAddr, uint8_t *data, uint8_t len)
{
    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_HandleTransfer(I2Cx, devAddr, LL_I2C_ADDRSLAVE_7BIT, 1+len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
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

void i2c_read(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t subAddr, volatile uint8_t *data, uint8_t len)
{
	if (LL_I2C_IsActiveFlag_BUSY(I2Cx)) { return; }	// ongoing transfer in progress

    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_HandleTransfer(I2Cx, devAddr, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
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

void i2c_initNB(I2C_TypeDef *I2Cx)
{
	NVIC_ClearPendingIRQ(I2C1_EV_IRQn);
	NVIC_SetPriority(I2C1_EV_IRQn, 3);
	NVIC_EnableIRQ(I2C1_EV_IRQn);

	NVIC_ClearPendingIRQ(I2C1_ER_IRQn);
	NVIC_SetPriority(I2C1_ER_IRQn, 3);
	NVIC_EnableIRQ(I2C1_ER_IRQn);
}

void i2c_writeNB(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t subAddr, uint8_t *data, uint8_t len)
{
	if (LL_I2C_IsActiveFlag_BUSY(I2Cx)) { return; }	// ongoing transfer in progress

	t.len = len;
	t.i = 0;
	t.data = data;

    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_ClearFlag_NACK(I2Cx);

    LL_I2C_HandleTransfer(I2Cx, devAddr, LL_I2C_ADDRSLAVE_7BIT, 1+len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
    LL_I2C_TransmitData8(I2Cx, subAddr);

    LL_I2C_EnableIT_TX(I2Cx);
    LL_I2C_EnableIT_NACK(I2Cx);
    LL_I2C_EnableIT_STOP(I2Cx);
	//LL_I2C_EnableIT_TC(I2Cx);	// not needed with AUTOEND

}



void I2C1_EV_IRQHandler(void)
{
	uint32_t flags = I2C1->ISR;

	if ((flags & I2C_ISR_TXE) && (I2C1->CR1 & I2C_CR1_TXIE)) {
		if (t.i < t.len) {
		    LL_I2C_TransmitData8(I2C1, t.data[t.i++]);
		} else {
			t.len = 0;
		    LL_I2C_DisableIT_TX(I2C1);
		}
	} else if (flags & I2C_ISR_STOPF) {
		// transfer complete?
		LL_I2C_DisableIT_STOP(I2C1);
	    LL_I2C_ClearFlag_STOP(I2C1);
	}

}

void I2C1_ER_IRQHandler(void)
{
	uint32_t flags = I2C1->ISR;
    LL_I2C_DisableIT_TX(I2C1);
    LL_I2C_DisableIT_NACK(I2C1);
    LL_I2C_DisableIT_STOP(I2C1);

    (void) flags;
}

