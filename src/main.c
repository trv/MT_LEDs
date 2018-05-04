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

uint32_t Power_Config(void)
{
    uint32_t status = PWR->SR1;

    /* Enable clock for SYSCFG */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    // PC13: WKUP2 (accel int 2) - also button on dev board
    LL_PWR_SetWakeUpPinPolarityLow(LL_PWR_WAKEUP_PIN2);
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN2);

    // PA0: Accel interrupt 1
    LL_PWR_EnableWakeUpPin(LL_PWR_WAKEUP_PIN1);
    
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

void accel_write(uint8_t subAddr, uint8_t *data, size_t len);
void accel_read(uint8_t subAddr, uint8_t *data, size_t len);

static volatile uint8_t who_am_i = 0;
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

void accel_config(void)
{
    uint8_t accel_config[] = {
    		0x4F,		// CTRL_REG1 = 50Hz, low power mode, XYZ on
			0x84,		// CTRL_REG2 = normal high-pass filter, 1Hz cutoff, enabled only for CLICK
			0x10,		// CTRL_REG3 = data ready interrupt on INT1
			0x00,		// CTRL_REG4
			0x00, 		// CTRL_REG5
			0x80,		// CTRL_REG6 = click interrupt on INT2, interrupt active high
    };

    accel_write(0x20, accel_config, 6);

    uint8_t click_config[] = {
    		0x3F,		// CLICK_CFG = enable single and double tap on XYZ
			0x00,		// read-only CLICK_SRC
			0x08,		// CLICK_THS = threshold
			0x08,		// TIME_LIMIT
			0x08,		// TIME_LATENCY
			0x08,		// TIME_WINDOW
    };

    accel_write(0x38, click_config, 6);
}

void accel_write(uint8_t subAddr, uint8_t *data, size_t len)
{
    LL_I2C_ClearFlag_STOP(I2C3);
    LL_I2C_HandleTransfer(I2C3, 0x30, LL_I2C_ADDRSLAVE_7BIT, 1+len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_WRITE);
    while (!LL_I2C_IsActiveFlag_TXE(I2C3));
    LL_I2C_TransmitData8(I2C3, 0x80 | subAddr); 	// set MSB to enable auto-increment
    for (size_t i = 0; i < len; i++) {
        while (!LL_I2C_IsActiveFlag_TXE(I2C3));
        LL_I2C_TransmitData8(I2C3, data[i]);
    }
    while (/*!LL_I2C_IsActiveFlag_TC(I2C3) && !LL_I2C_IsActiveFlag_NACK(I2C3) &&*/ !LL_I2C_IsActiveFlag_STOP(I2C3));
    LL_I2C_ClearFlag_STOP(I2C3);
    LL_I2C_ClearFlag_NACK(I2C3);
}

void accel_read(uint8_t subAddr, uint8_t *data, size_t len)
{
    LL_I2C_ClearFlag_STOP(I2C3);
    LL_I2C_HandleTransfer(I2C3, 0x30, LL_I2C_ADDRSLAVE_7BIT, 1, LL_I2C_MODE_SOFTEND, LL_I2C_GENERATE_START_WRITE);
    LL_I2C_TransmitData8(I2C3, 0x80 | subAddr);	// set MSB to enable auto-increment
    while (!LL_I2C_IsActiveFlag_TC(I2C3));
    LL_I2C_HandleTransfer(I2C3, 0x30, LL_I2C_ADDRSLAVE_7BIT, len, LL_I2C_MODE_AUTOEND, LL_I2C_GENERATE_START_READ);
    for (size_t i = 0; i < len; i++) {
    	while (!LL_I2C_IsActiveFlag_RXNE(I2C3) && !LL_I2C_IsActiveFlag_NACK(I2C3) && !LL_I2C_IsActiveFlag_STOP(I2C3));
    	data[i] = LL_I2C_ReceiveData8(I2C3);
    }

    while (/*!LL_I2C_IsActiveFlag_TC(I2C3) && !LL_I2C_IsActiveFlag_NACK(I2C3) &&*/ !LL_I2C_IsActiveFlag_STOP(I2C3));
    LL_I2C_ClearFlag_STOP(I2C3);
    LL_I2C_ClearFlag_NACK(I2C3);
}


int main(void){

    /* Configure the system clock */
    SystemClock_Config();

    uint32_t powerStatus = Power_Config();

    LED_Config();

    I2C3_Config();

    if (!(powerStatus & PWR_SR1_SBF)) {
        accel_config();
    }
 
    LL_mDelay(10);
    uint8_t ctrlData[8] = {0};
    accel_read(0x20, ctrlData, 8);

	//accel_read(0x28, ctrlData, 6);	// read values
	LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_13);
	LL_mDelay(100);
	LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_13);
	LL_mDelay(10);

	// go to stop mode
    __WFI();

    /* loop forever */
    while(1);

    return 0;
}
