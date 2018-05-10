
#include "power.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#define EN_5V_PORT	GPIOA
#define EN_5V_PIN	LL_GPIO_PIN_8

uint32_t Power_Init(void)
{
    /* Enable clock for SYSCFG */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    uint32_t status = PWR->SR1;

    // turn on 5V regulator

    LL_GPIO_ResetOutputPin(EN_5V_PORT, EN_5V_PIN);

    LL_GPIO_InitTypeDef gpioConfig;
    LL_GPIO_StructInit(&gpioConfig);
    gpioConfig.Pin = EN_5V_PIN;
    gpioConfig.Speed = LL_GPIO_SPEED_LOW;
    gpioConfig.Mode = LL_GPIO_MODE_OUTPUT;
    gpioConfig.Pull = LL_GPIO_PULL_NO;
    gpioConfig.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(EN_5V_PORT, &gpioConfig);
    LL_GPIO_SetOutputPin(EN_5V_PORT, EN_5V_PIN);

    return status;
}

void Power_Shutdown(enum ShutdownReason reason)
{
    // only enable double-tap interrupt for normal turn-off
    if (reason == ShutdownReason_TurnOff || reason == ShutdownReason_Lockup) {
        // PC13: WKUP2 - accel int 2 - double-tap
        LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN2);

        // PA0: WKUP1 - accel int 1 - data ready - not used for wakeup
        //LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);
    } else if (reason == ShutdownReason_LowBattery) {
    	// TODO: put accel in shutdown mode to save more power
    }

    // LIPO_PG interrupt is always active
    // PA2: WKUP4 - Charger PG interrupt (with pull-up)
    LL_PWR_EnableGPIOPullUp(LL_PWR_GPIO_A, LL_PWR_GPIO_BIT_2);
    LL_PWR_EnablePUPDCfg();
    LL_PWR_SetWakeUpPinPolarityLow(LL_PWR_WAKEUP_PIN4);
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN4);

    // set STANDBY as low power mode
    LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
    LL_LPM_EnableDeepSleep();

    LL_PWR_DisableInternWU();

    LL_PWR_ClearFlag_SB();
    LL_PWR_ClearFlag_WU1();
    LL_PWR_ClearFlag_WU2();
    LL_PWR_ClearFlag_WU4();

    __WFI();
}


