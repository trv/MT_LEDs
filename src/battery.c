
#include "battery.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "interrupt.h"

#define LIPO_PORT		GPIOA
#define LIPO_PG_PIN		LL_GPIO_PIN_2
#define LIPO_CHG_PIN	LL_GPIO_PIN_15

#define SHUTDOWN_mV		3000
#define LOWBATT_mV		3300

static enum BatteryStatus status;
static uint32_t battery_mV;

static void (*cb)(void *, enum BatteryStatus status, uint32_t battery_mV);
static void *cbCtx;

static void updateCB(void *ctx);
static void updateNormal(void);

void Battery_Init(void)
{
    EXTI_SetCallback(LIPO_PG, updateCB, NULL);
    EXTI_SetCallback(LIPO_CHG, updateCB, NULL);

    status = BatteryUnknown;
    battery_mV = 0;
    // check state of chg/pg pins
    updateCB(NULL);
}

void Battery_SetCallback(void (*callback)(void *, enum BatteryStatus status, uint32_t battery_mV), void *ctx)
{
	cbCtx = ctx;
	cb = callback;
}


enum BatteryStatus Battery_GetStatus(void)
{
	return status;
}

uint32_t Battery_GetLevel_mV(void)
{
	return battery_mV;
}

void Battery_UpdateVoltage(uint32_t adcSample)
{
	// battery divider is 2:1, so voltage is 3x reading
	// adcSample * 1800mV * 3x / 2^12
	// resolution is 1.32mV / count
	battery_mV = (adcSample*1800*3 + 2048) >> 12;
	if (status <= BatteryNormal) {
		updateNormal();
	}
}

static void updateNormal(void)
{
	enum BatteryStatus newStatus = status;

	if (battery_mV < SHUTDOWN_mV) {
		newStatus = BatteryCritical;
	} else if (battery_mV < LOWBATT_mV) {
		newStatus = BatteryLow;
	} else {
		newStatus = BatteryNormal;
	}

	if (newStatus != status) {
		status = newStatus;
		if (cb) {
			cb(cbCtx, status, battery_mV);
		}
	}
}

static void updateCB(void *ctx)
{
	uint32_t pg = !LL_GPIO_IsInputPinSet(LIPO_PORT, LIPO_PG_PIN);
	uint32_t chg = !LL_GPIO_IsInputPinSet(LIPO_PORT, LIPO_CHG_PIN);

	enum BatteryStatus newStatus = status;

	if (pg) {
		if (chg) {
			newStatus = BatteryCharging;
		} else {
			newStatus = BatteryFull;
		}
		if (newStatus != status) {
			status = newStatus;
			if (cb) {
				cb(cbCtx, status, battery_mV);
			}
		}
	} else {
		updateNormal();
	}

}

