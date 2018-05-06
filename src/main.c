#include <stdlib.h>

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "interrupt.h"
#include "i2c.h"
#include "accel.h"
#include "led.h"

volatile uint32_t tick = 0;

#define LED_ADDR		0xA0

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
	while ((tick - now) <= ms);
}

uint32_t Power_Config(void)
{
    /* Enable clock for SYSCFG */
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
    uint32_t status = PWR->SR1;

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

    // configure SHDN pin while we're here
    LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_12);

    gpioConfig.Pin = LL_GPIO_PIN_12;
    gpioConfig.Speed = LL_GPIO_SPEED_LOW;
    gpioConfig.Mode = LL_GPIO_MODE_OUTPUT;
    gpioConfig.Pull = LL_GPIO_PULL_NO;
    gpioConfig.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpioConfig.Alternate = LL_GPIO_AF_0;
    LL_GPIO_Init(GPIOA, &gpioConfig);


    LL_I2C_StructInit(&i2cConfig);
    i2cConfig.Timing = 0x30A54E69; //0x00B11024;
    //0x00D00E28;  /* (Rise time = 120ns, Fall time = 25ns) */
    
    LL_I2C_Init(I2C1, &i2cConfig);
}

static volatile uint8_t accelData[6];
static volatile uint8_t shutdown = 0;
static volatile uint8_t cycle = 0;
void accel_int1_handler(void *ctx)
{
    i2c_read(I2C3, ACCEL_ADDR, 0x80 | 0x28, accelData, 6);
}

void accel_int2_handler(void *ctx)
{
	static uint32_t lastClick = 0x80000000u;
	uint8_t clickSrc = 0;
    i2c_read(I2C3, ACCEL_ADDR, 0x39, &clickSrc, 1);
    if (clickSrc & 0x40) {
    	// interrupt active
    	if ((tick - lastClick) < 300) {
			accel_config_asleep();
			// go to stop mode
			shutdown = 1;
    	} else {
    		lastClick = tick;
    		cycle = 1;
    	}
    }
}

static struct LED l;

uint8_t animateIndex = 0;
uint8_t animateColor[][3] = {
		{64,0,0},
		{48,48,0},
		{0, 64, 0},
		{0, 48, 48},
		{0, 0, 64},
		{48, 0, 48}
};

void animate(int x, int y, uint8_t *c)
{
	for (int i=0; i < 3; i++) {
		c[i] = animateColor[animateIndex][i];
	}
}

int main(void)
{
    uint8_t clickSrc;

    /* Configure the system clock */
    SystemClock_Config();

    uint32_t powerStatus = Power_Config();

    LED_Config();

    I2C3_Config();
    I2C1_Config();

    EXTI_SetCallback(ACCEL_INT1, accel_int1_handler, NULL);
    EXTI_SetCallback(ACCEL_INT2, accel_int2_handler, NULL);

    EXTI_Config();

    if (powerStatus & LL_PWR_SR1_WUF4) {
        i2c_read(I2C3, ACCEL_ADDR, 0x39, &clickSrc, 1);
    }

    accel_config_awake();
    i2c_read(I2C3, ACCEL_ADDR, 0x80 | 0x28, accelData, 6);

    LED_Init(&l, I2C1, LED_ADDR, GPIOA, LL_GPIO_PIN_12);

    while (1) {
        LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_13);
        delay_ms(100);
        LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_13);
        if (shutdown) {
        	while (1) {
        		EXTI_Stop();
                Power_Config();
        		__WFI();
        	}
        }
        if (cycle != 0) {
        	cycle++;
        }
        if (cycle == 3) {
        	cycle = 0;
        	LED_Update(&l, animate);
        	animateIndex = (animateIndex + 1) % 6;
        }
        delay_ms(100);
    }

    /* loop forever */
    while(1);

    return 0;
}

void SysTick_Handler(void) {
	tick++;
}
