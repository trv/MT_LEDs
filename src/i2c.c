
#include "i2c.h"

struct transfer {
	void (*cb)(void *);
	void *ctx;
	uint8_t len;
	uint8_t i;
	volatile uint8_t *data;
};

static struct transfer xfer[3];

static const uint32_t I2Cx_EV_IRQn[] = {I2C1_EV_IRQn, I2C2_EV_IRQn, I2C3_EV_IRQn};
static const uint32_t I2Cx_ER_IRQn[] = {I2C1_ER_IRQn, I2C2_ER_IRQn, I2C3_ER_IRQn};

static uint8_t getIndex(I2C_TypeDef *I2Cx);


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
	uint8_t i = getIndex(I2Cx);

	NVIC_ClearPendingIRQ(I2Cx_EV_IRQn[i]);
	NVIC_SetPriority(I2Cx_EV_IRQn[i], 3);
	NVIC_EnableIRQ(I2Cx_EV_IRQn[i]);

	NVIC_ClearPendingIRQ(I2Cx_ER_IRQn[i]);
	NVIC_SetPriority(I2Cx_ER_IRQn[i], 3);
	NVIC_EnableIRQ(I2Cx_ER_IRQn[i]);
}

static uint8_t getIndex(I2C_TypeDef *I2Cx)
{
	if (I2Cx == I2C1) {
		return 0;
	} else if (I2Cx == I2C2) {
		return 1;
	} else if (I2Cx == I2C3) {
		return 2;
	} else {
		return -1;
	}
}

int i2c_writeNB(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t subAddr, uint8_t *data, uint8_t len, void (cb(void *)), void *ctx)
{
	if (LL_I2C_IsActiveFlag_BUSY(I2Cx)) { return 1; }	// ongoing transfer in progress

	uint8_t i = getIndex(I2Cx);

	xfer[i].cb = cb;
	xfer[i].ctx = ctx;
	xfer[i].len = len;
	xfer[i].i = 0;
	xfer[i].data = data;

    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_ClearFlag_NACK(I2Cx);

    LL_I2C_HandleTransfer(I2Cx, devAddr, LL_I2C_ADDRSLAVE_7BIT, 1+len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
    LL_I2C_TransmitData8(I2Cx, subAddr);

    LL_I2C_EnableIT_TX(I2Cx);
    LL_I2C_EnableIT_NACK(I2Cx);
    LL_I2C_EnableIT_STOP(I2Cx);
	//LL_I2C_EnableIT_TC(I2Cx);	// not needed with AUTOEND

    return 0;
}

int i2c_readNB(I2C_TypeDef *I2Cx, uint8_t devAddr, uint8_t subAddr, volatile uint8_t *data, uint8_t len, void (cb(void *)), void *ctx)
{
	if (LL_I2C_IsActiveFlag_BUSY(I2Cx)) { return 1; }	// ongoing transfer in progress

	uint8_t i = getIndex(I2Cx);

	xfer[i].cb = cb;
	xfer[i].ctx = ctx;
	xfer[i].len = len;
	xfer[i].i = 0;
	xfer[i].data = data;

    LL_I2C_ClearFlag_STOP(I2Cx);
    LL_I2C_ClearFlag_NACK(I2Cx);

    LL_I2C_HandleTransfer(I2Cx, devAddr, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
    LL_I2C_TransmitData8(I2Cx, subAddr);

    LL_I2C_EnableIT_TC(I2Cx);
    LL_I2C_EnableIT_NACK(I2Cx);
    LL_I2C_EnableIT_STOP(I2Cx);

    return 0;
}

void I2C_EV_IRQHandler(I2C_TypeDef *I2Cx, struct transfer *t)
{
	uint32_t flags = I2Cx->ISR;

	if ((flags & I2C_ISR_TXE) && (I2Cx->CR1 & I2C_CR1_TXIE)) {
		if (t->i < t->len) {
		    LL_I2C_TransmitData8(I2Cx, t->data[t->i++]);
		} else {
			t->len = 0;
		    LL_I2C_DisableIT_TX(I2Cx);
		}
	} else if (flags & I2C_ISR_STOPF) {
		// transfer complete?
		LL_I2C_DisableIT_STOP(I2Cx);
	    LL_I2C_ClearFlag_STOP(I2Cx);
	    if (t->cb) { t->cb(t->ctx); }
	} else if (flags & I2C_ISR_TC) {
	    LL_I2C_HandleTransfer(I2Cx, 0x30, LL_I2C_ADDRSLAVE_7BIT, t->len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
	    LL_I2C_DisableIT_TC(I2Cx);
	    LL_I2C_EnableIT_RX(I2Cx);
	} else if (flags & I2C_ISR_RXNE) {
		if (t->i < t->len) {
			t->data[t->i++] = LL_I2C_ReceiveData8(I2Cx);
		} else {
			t->len = 0;
		    LL_I2C_DisableIT_RX(I2Cx);
		}
	}

}

void I2C_ER_IRQHandler(I2C_TypeDef *I2Cx, struct transfer *t)
{
	uint32_t flags = I2C1->ISR;
    LL_I2C_DisableIT_TX(I2Cx);
    LL_I2C_DisableIT_NACK(I2Cx);
    LL_I2C_DisableIT_STOP(I2Cx);

    (void) flags;
    if (t->cb) { t->cb(t->ctx); }
}

void I2C1_EV_IRQHandler(void)
{
	I2C_EV_IRQHandler(I2C1, &xfer[0]);
}

void I2C1_ER_IRQHandler(void)
{
	I2C_ER_IRQHandler(I2C1, &xfer[0]);
}

void I2C2_EV_IRQHandler(void)
{
	I2C_EV_IRQHandler(I2C2, &xfer[1]);
}

void I2C2_ER_IRQHandler(void)
{
	I2C_ER_IRQHandler(I2C2, &xfer[1]);
}

void I2C3_EV_IRQHandler(void)
{
	I2C_EV_IRQHandler(I2C3, &xfer[2]);
}

void I2C3_ER_IRQHandler(void)
{
	I2C_ER_IRQHandler(I2C3, &xfer[2]);
}


