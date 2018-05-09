
#include "power.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

uint32_t Power_Init(void)
{
    /* Enable clock for SYSCFG */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    uint32_t status = PWR->SR1;

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
    }

    // LIPO_PG interrupt is always active
    // PA2: WKUP4 - Charger PG interrupt (with pull-up)
    LL_PWR_EnableGPIOPullUp(LL_PWR_GPIO_A, LL_PWR_GPIO_BIT_2);
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


