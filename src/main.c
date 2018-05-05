#include <stdlib.h>

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "accel.h"

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
    
    LL_Init1msTick(SystemCoreClock);
}

uint32_t Power_Config(void)
{
    /* Enable clock for SYSCFG */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    uint32_t status = PWR->SR1;

    // PC13: WKUP2 (accel int 2) - also button on dev board
    LL_PWR_SetWakeUpPinPolarityLow(LL_PWR_WAKEUP_PIN2);
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN2);

    // PA0: Accel interrupt 1
    //LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);
    
    // PA2: Charger PG interrupt
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN4);

    // set STANDBY as low power mode
    LL_PWR_SetPowerMode(LL_PWR_MODE_STANDBY);
    LL_LPM_EnableDeepSleep();

    LL_PWR_ClearFlag_SB();
    LL_PWR_ClearFlag_WU1();
    LL_PWR_ClearFlag_WU2();
    LL_PWR_ClearFlag_WU4();
    LL_PWR_DisableInternWU();

    return status;
}

void LED_Config(void)
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

void I2C3_Config(void)
{
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
    
    LL_I2C_Init(I2C3, &i2cConfig);
}

void I2C1_Config(void)
{
    LL_I2C_InitTypeDef i2cConfig;
    LL_GPIO_InitTypeDef gpioConfig;

    // I2C3: SCL = PA9, SDA = PA10
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);

    LL_SYSCFG_EnableFastModePlus(LL_SYSCFG_I2C_FASTMODEPLUS_I2C1);

    LL_GPIO_StructInit(&gpioConfig);
    gpioConfig.Pin = LL_GPIO_PIN_9;
    gpioConfig.Speed = LL_GPIO_SPEED_HIGH;
    gpioConfig.Mode = LL_GPIO_MODE_ALTERNATE;
    gpioConfig.Pull = LL_GPIO_PULL_UP;
    gpioConfig.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpioConfig.Alternate = LL_GPIO_AF_4;

    // do we need internal pull--ups?

    LL_GPIO_Init(GPIOA, &gpioConfig);

    gpioConfig.Pin = LL_GPIO_PIN_10;
    LL_GPIO_Init(GPIOA, &gpioConfig);

    LL_I2C_StructInit(&i2cConfig);
    i2cConfig.Timing = 0x00B11024;
    //0x00D00E28;  /* (Rise time = 120ns, Fall time = 25ns) */
    
    LL_I2C_Init(I2C3, &i2cConfig);
}

int main(void)
{
    uint8_t clickSrc;

    /* Configure the system clock */
    SystemClock_Config();

    uint32_t powerStatus = Power_Config();

    LED_Config();

    I2C3_Config();

    if (powerStatus & LL_PWR_SR1_WUF4) {
        accel_read(0x39, &clickSrc, 1);
    }

    accel_config_awake();

    while (1) {
        LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_13);
        LL_mDelay(100);
        LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_13);
        LL_mDelay(100);
        accel_read(0x39, &clickSrc, 1);
        if (clickSrc & 0x40) {
        	// interrupt active
            accel_config_asleep();
            // go to stop mode
            LL_mDelay(100);
            Power_Config();
            __WFI();
        }
    }

    /* loop forever */
    while(1);

    return 0;
}
