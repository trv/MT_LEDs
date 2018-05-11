#include <stdlib.h>

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "interrupt.h"
#include "adc.h"
#include "i2c.h"
#include "accel.h"
#include "led.h"
#include "display.h"
#include "power.h"
#include "battery.h"

volatile uint32_t tick = 0;
static uint32_t adcSamples[5];

void SystemClock_Config(void)
{
    // update flash latency
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_FLASH);
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, (FLASH_ACR_LATENCY_4WS));
    
    /* Clock init stuff */ 
    LL_UTILS_PLLInitTypeDef sUTILS_PLLInitStruct = {
        .PLLM = LL_RCC_PLLM_DIV_2, 
        .PLLN = 20,
        .PLLR = LL_RCC_PLLR_DIV_2
    };

    LL_UTILS_ClkInitTypeDef sUTILS_ClkInitStruct = {
        LL_RCC_SYSCLK_DIV_1, 
        LL_RCC_APB1_DIV_1, 
        LL_RCC_APB2_DIV_1
    };
    
    LL_PLL_ConfigSystemClock_HSI(&sUTILS_PLLInitStruct, &sUTILS_ClkInitStruct);
    
    /* Configure the SysTick to have interrupt in 1ms time base */
    SysTick->LOAD  = (uint32_t)((SystemCoreClock / 1000) - 1UL);  /* set reload register */
    SysTick->VAL   = 0UL;                                       /* Load the SysTick Counter Value */
    SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |
    		SysTick_CTRL_TICKINT_Msk |
			SysTick_CTRL_ENABLE_Msk;                   /* Enable the Systick Timer */

	NVIC_ClearPendingIRQ(SysTick_IRQn);
	NVIC_EnableIRQ(SysTick_IRQn);
}

void delay_ms(uint32_t ms)
{
	uint32_t now = tick;
	while ((tick - now) <= ms);		//todo: __WFI(); ?
}

// called from accel_Poll() on main thread
void clickHandler(void *ctx, enum ClickType type)
{
	switch (type) {
	case ClickSingle:
		display_Next();
		break;
	case ClickDouble:
		accel_config_asleep();
		// go to stop mode, loop to ensure no pending interrupts
    	while (1) {
    		EXTI_Stop();
            Power_Shutdown(ShutdownReason_TurnOff);
    	}
    	break;
	}
}

// called from accel_Poll() on main thread
void animateHandler(void *ctx, volatile uint8_t *accelData)
{
	display_Update(accelData, &adcSamples[1]);
}

// called from interrupt context OR from Battery_UpdateVoltage
void batteryUpdate(void *ctx, enum BatteryStatus status, uint32_t battery_mV)
{
    switch (status) {
    case BatteryCritical:
        accel_config_shutdown();
        // go to stop mode, loop to ensure no pending interrupts
        while (1) {
            EXTI_Stop();
            Power_Shutdown(ShutdownReason_LowBattery);
        }
        break;
    case BatteryLow:
        display_Charger(ChargeColorRed);
        break;
    case BatteryNormal:
    case BatteryUnknown:
        display_Charger(ChargeColorNone);
        break;
    case BatteryCharging:
        display_Charger(ChargeColorYellow);
        break;
    case BatteryFull:
        display_Charger(ChargeColorGreen);
        break;
    }
}

int main(void)
{
    uint8_t clickSrc = 0;

    SystemClock_Config();
    uint32_t powerStatus = Power_Init();
    delay_ms(100);
    EXTI_Config();

    display_Init();

    Battery_Init();
    Battery_SetCallback(batteryUpdate, NULL);
    batteryUpdate(NULL, Battery_GetStatus(), Battery_GetLevel_mV());

    adc_Init();

    accel_Init();
    accel_setClickHandler(clickHandler, NULL);
    accel_setDataHandler(animateHandler, NULL);

    if (powerStatus & LL_PWR_SR1_WUF2) {
        i2c_read(I2C3, ACCEL_ADDR, 0x39, &clickSrc, 1);
    }

    accel_config_awake();

    uint32_t sampleTick = tick;

    while (1) {
    	if ( (tick - sampleTick) >= 100) {
    		sampleTick += 100;
            adc_Sample(adcSamples);
            Battery_UpdateVoltage(adcSamples[0]);
    	}

    	// handle single/double click events & data event
    	accel_Poll();

        __WFI();
    }

    return 0;
}

void SysTick_Handler(void) {
	tick++;
}
