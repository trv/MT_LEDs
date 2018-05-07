#include <stdlib.h>

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "interrupt.h"
#include "i2c.h"
#include "accel.h"
#include "led.h"
#include "display.h"

volatile uint32_t tick = 0;

void SystemClock_Config(void){

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

uint32_t Power_Config(void)
{
    /* Enable clock for SYSCFG */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    uint32_t status = PWR->SR1;

    return status;
}

void Power_Shutdown(void)
{
    // PC13: WKUP2 (accel int 2) - also button on dev board
    //LL_PWR_SetWakeUpPinPolarityLow(LL_PWR_WAKEUP_PIN2);
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN2);

    // PA0: Accel interrupt 1
    //LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);
    
    // PA2: Charger PG interrupt
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

void StatusLED_Config(void)
{
    /* Use a structure for this (usually for bulk init), you can also use LL functions */   
    LL_GPIO_InitTypeDef GPIO_InitStruct;
    
    /* Enable the GPIO clock for GPIO */
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);

    /* Set up port parameters */
    LL_GPIO_StructInit(&GPIO_InitStruct);
    GPIO_InitStruct.Pin = LL_GPIO_PIN_13;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

// called from accel_Poll() on main thread
void clickHandler(void *ctx, enum ClickType type)
{
	switch (type) {
	case ClickSingle:
		display_Next();	//cycle = 1;
		break;
	case ClickDouble:
		accel_config_asleep();
		// go to stop mode, loop to ensure no pending interrupts
    	while (1) {
    		EXTI_Stop();
            Power_Shutdown();
    	}
    	break;
	}
}

// called from accel_Poll() on main thread
void animateHandler(void *ctx, volatile uint8_t *data)
{
	display_Update(data);
}

int main(void)
{
    uint8_t clickSrc = 0;

    SystemClock_Config();
    uint32_t powerStatus = Power_Config();
    StatusLED_Config();
    EXTI_Config();

    accel_Init();
    accel_setClickHandler(clickHandler, NULL);
    accel_setDataHandler(animateHandler, NULL);

    if (powerStatus & LL_PWR_SR1_WUF4) {
        i2c_read(I2C3, ACCEL_ADDR, 0x39, &clickSrc, 1);
    }

    display_Init();

    accel_config_awake();

    uint32_t toggleTick = tick;

    while (1) {
    	if ( (tick - toggleTick) >= 100) {
    		toggleTick += 100;
    		LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_13);
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
