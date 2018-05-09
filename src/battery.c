
#include "battery.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "interrupt.h"

#define LIPO_PORT		GPIOA
#define LIPO_PG_PIN		LL_GPIO_PIN_2
#define LIPO_CHG_PIN	LL_GPIO_PIN_15

static enum BatteryStatus status;

static void updateCB(void *ctx);

void Battery_Init(void)
{
    EXTI_SetCallback(LIPO_PG, updateCB, NULL);
    EXTI_SetCallback(LIPO_CHG, updateCB, NULL);

    status = BatteryUnknown;
    // check state of chg/pg pins
    updateCB(NULL);
}

enum BatteryStatus Battery_GetStatus(void)
{
	return status;
}

void Battery_UpdateVoltage(uint32_t adcSample)
{

}

static void updateCB(void *ctx)
{

}

