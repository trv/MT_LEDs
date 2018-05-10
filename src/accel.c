
#include "accel.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "i2c.h"
#include "interrupt.h"
#include "power.h"

#define I2Cx		I2C3

extern volatile uint32_t tick;

static const uint32_t doubleTapTime = 250;
static volatile uint32_t lastClick = 0x80000000u;
static volatile uint8_t clickSrc = 0;
static volatile uint8_t clickReadPendingFlag = 0;
static volatile uint8_t singleClickPending = 0;
static volatile uint8_t doubleClickPending = 0;
static volatile uint8_t dataPending = 0;

static volatile uint8_t accelData[6];

static clickHandlerCB *clickCB = NULL;
static void *clickCtx = NULL;
static dataHandlerCB *dataCB = NULL;
static void *dataCtx = NULL;

void accel_int1_handler(void *ctx);
void accel_int2_handler(void *ctx);

void accel_Init(void)
{
	// config I2C

    LL_I2C_InitTypeDef i2cConfig;
    LL_GPIO_InitTypeDef gpioConfig;

    // I2C3: SCL = PA7, SDA = PB4
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C3);

    LL_GPIO_StructInit(&gpioConfig);
    gpioConfig.Pin = LL_GPIO_PIN_7;
    gpioConfig.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpioConfig.Mode = LL_GPIO_MODE_ALTERNATE;
    gpioConfig.Pull = LL_GPIO_PULL_UP;
    gpioConfig.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpioConfig.Alternate = LL_GPIO_AF_4;

    // do we need internal pull--ups?

    LL_GPIO_Init(GPIOA, &gpioConfig);

    gpioConfig.Pin = LL_GPIO_PIN_4;
    LL_GPIO_Init(GPIOB, &gpioConfig);

    LL_I2C_StructInit(&i2cConfig);
    i2cConfig.Timing = 0x30A54E69;     // 300ns rise, 300ns fall, 400kHz speed:
    //0x00D00E28;  /* (Rise time = 120ns, Fall time = 25ns) */

    LL_I2C_Init(I2Cx, &i2cConfig);

    i2c_initNB(I2Cx);

	// config INT1/INT2 interrupts

    EXTI_SetCallback(ACCEL_INT1, accel_int1_handler, NULL);
    EXTI_SetCallback(ACCEL_INT2, accel_int2_handler, NULL);
}

void accel_setClickHandler(clickHandlerCB cb, void *ctx)
{
	clickCB = cb;
	clickCtx = ctx;
}

void accel_setDataHandler(dataHandlerCB cb, void *ctx)
{
	dataCB = cb;
	dataCtx = ctx;
}

void accel_Poll(void)
{
    // just in case we missed an edge
    if (EXTI_GetPinState(ACCEL_INT1)) {
        accel_int1_handler(NULL);
    }

	if (dataPending) {
		dataPending = 0;
		dataCB(dataCtx, accelData);
	}

	if (singleClickPending && (tick - lastClick) > doubleTapTime) {
		singleClickPending = 0;
		if (clickCB) { clickCB(clickCtx, ClickSingle); }
	} else if (doubleClickPending) {
		doubleClickPending = 0;
		if (clickCB) { clickCB(clickCtx, ClickDouble); }
	}

}

void accel_config_shutdown(void)
{
    uint8_t accel_config[] = {
            0x00,       // CTRL_REG1 = shutdown mode
    };
    i2c_write(I2Cx, ACCEL_ADDR, 0x80 | 0x20, accel_config, 1);
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
			0x18,		// CLICK_THS = threshold
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

    // read accel data to reset interrupt pin
    i2c_read(I2C3, ACCEL_ADDR, 0x80 | 0x28, accelData, 6);
}

void clickReadHandler(void *ctx)
{
    if (clickSrc & 0x40) {
    	// interrupt active
    	if ((tick - lastClick) < doubleTapTime) {
            if (singleClickPending) {
                singleClickPending = 0;
                doubleClickPending = 1;
            } else if (doubleClickPending) {
                // emergency shutdown
                Power_Shutdown(ShutdownReason_Lockup);
            }
    	} else {
    		singleClickPending = 1;
    	}
        lastClick = tick;
    }
}

void accel_int2_handler(void *ctx)
{
	// set clickReadPendingFlag to read the clickSrc later if I2C is busy
    clickReadPendingFlag = i2c_readNB(I2C3, ACCEL_ADDR, 0x39, &clickSrc, 1, clickReadHandler, NULL);
}

void dataReadHandler(void *ctx)
{
	dataPending = 1;
	if (clickReadPendingFlag) {
    	clickReadPendingFlag = 0;
    	i2c_readNB(I2C3, ACCEL_ADDR, 0x39, &clickSrc, 1, clickReadHandler, NULL);
    }
}

void accel_int1_handler(void *ctx)
{
    i2c_readNB(I2C3, ACCEL_ADDR, 0x80 | 0x28, accelData, 6, dataReadHandler, NULL);
}

