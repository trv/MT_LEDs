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

    i2c_initNB(I2C3);
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
    i2cConfig.Timing = 0x20A54E50; //0x00B11024;
    //0x00D00E28;  /* (Rise time = 120ns, Fall time = 25ns) */
    
    LL_I2C_Init(I2C1, &i2cConfig);

    i2c_initNB(I2C1);
}

static volatile uint8_t accelData[6];
static volatile uint8_t shutdown = 0;
static volatile uint8_t cycle = 0;
static volatile uint8_t animate = 0;

static uint32_t lastClick = 0x80000000u;
static const uint32_t doubleTapTime = 250;
static uint8_t clickSrc = 0;
static uint8_t clickFlag = 0;

void clickHandler(void *ctx)
{
    if (clickSrc & 0x40) {
    	// interrupt active
    	if ((tick - lastClick) < doubleTapTime) {
			accel_config_asleep();
			// go to stop mode
			shutdown = 1;
    	} else {
    		lastClick = tick;
    		cycle = 1;
    	}
    }
}

void accel_int2_handler(void *ctx)
{
	// set clickFlag to read the clickSrc later if I2C is busy
    clickFlag = i2c_readNB(I2C3, ACCEL_ADDR, 0x39, &clickSrc, 1, clickHandler, NULL);
}

void animateHandler(void *ctx)
{
    animate++;
    if (clickFlag) {
    	clickFlag = 0;
    	i2c_readNB(I2C3, ACCEL_ADDR, 0x39, &clickSrc, 1, clickHandler, NULL);
    }
}

void accel_int1_handler(void *ctx)
{
    i2c_readNB(I2C3, ACCEL_ADDR, 0x80 | 0x28, accelData, 6, animateHandler, NULL);
}

static struct LED l;

uint8_t animateIndex = 6;
uint8_t animateColor[][3] = {
		{64,0,0},
		{48,48,0},
		{0, 64, 0},
		{0, 48, 48},
		{0, 0, 64},
		{48, 0, 48}
};

void solidColors(int x, int y, uint8_t *c)
{
	for (int i=0; i < 3; i++) {
		c[i] = animateColor[animateIndex][i];
	}
}


uint8_t currentPhase = 0;
const uint8_t phaseSpeed = 4;
int panelPhase = 0x80;  // 1/4 of 256
int redPhase = 0;
int greenPhase = 85;
int bluePhase = 170;

uint8_t sinTable[] = {
  128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244, 245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 93, 90, 88, 85, 82, 79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40, 37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11, 10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9, 10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35, 37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76, 79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124
};

void animateLEDs(int x, int y, uint8_t *c)
{
	// grab the latest accel data TODO: make atomic
	//int8_t Ax = accelData[1];
	int8_t Ay = accelData[3];
	int8_t Az = accelData[5];

	// for now, just use the z-axis to set the gradient
	//int32_t offset = ((2*x - 7) * Az)/2;
	int32_t offset = -((2*x - 7) * Az + (2*y - 7) * Ay)/4;
	if (offset > 127) {
		offset = 127;
	} else if (offset < -127) {
		offset = -127;
	}
	c[0] = sinTable[(uint8_t)(currentPhase + offset + redPhase)]/2;
	c[1] = sinTable[(uint8_t)(currentPhase + offset + greenPhase)]/2;
	c[2] = sinTable[(uint8_t)(currentPhase + offset + bluePhase)]/2;
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

    uint32_t toggleTick = tick;

    while (1) {
    	if ( (tick - toggleTick) >= 100) {
    		toggleTick += 100;
    		LL_GPIO_TogglePin(GPIOB, LL_GPIO_PIN_13);
    	}

        if (shutdown) {
        	while (1) {
        		EXTI_Stop();
                Power_Shutdown();
        	}
        }

        if (animate >= 1) {
        	animate = 0;

        	if (cycle && (tick - lastClick) > doubleTapTime) {
            	cycle = 0;
            	animateIndex = (animateIndex + 1) % 7;
            	if (animateIndex < 6) {
            		LED_Update(&l, solidColors);
            	}
            }

        	if (animateIndex == 6) {
        		LED_Update(&l, animateLEDs);
        		currentPhase += phaseSpeed;
        	}
        }
        __WFI();
    }

    return 0;
}

void SysTick_Handler(void) {
	tick++;
}
