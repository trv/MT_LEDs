#include <stdlib.h>

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

void SystemClock_Config(void){

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
    
    LL_Init1msTick(SystemCoreClock);
}

int main(void){

    /* Configure the system clock */
    SystemClock_Config();

    /* Enable clock for SYSCFG */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    // Configure PC13 as WKUP2
    LL_PWR_SetWakeUpPinPolarityLow(LL_PWR_WAKEUP_PIN2);
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN2);

    // set STANDBY as low power mode
    LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
    LL_LPM_EnableDeepSleep();

    LL_PWR_ClearFlag_SB();
    LL_PWR_ClearFlag_WU2();
    LL_PWR_DisableInternWU();

    /* Let's pick a pin and toggle it */

    /* Use a structure for this (usually for bulk init), you can also use LL functions */   
    LL_GPIO_InitTypeDef GPIO_InitStruct;
    
    /* Enable the GPIO clock for GPIOA*/
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);


    /* Set up port A parameters */
    LL_GPIO_StructInit(&GPIO_InitStruct);                   // init the struct with some sensible defaults 
    GPIO_InitStruct.Pin = LL_GPIO_PIN_13;                    // GPIO pin 5; on Nucleo there is an LED
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;         // output speed
    GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;             // set as output 
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;   // make it a push pull
    LL_GPIO_Init(GPIOB, &GPIO_InitStruct);                  // initialize PORT A
 
    LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_13);
    LL_mDelay(500);
    LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_13);
    LL_mDelay(10);

    // go to stop mode
    __WFI();


    /* Toggle forever */
    while(1){
        LL_mDelay(250);
        LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_13);
    }

    return 0;
}
