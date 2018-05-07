
#include "display.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "i2c.h"
#include "led.h"

// TODO: set back to 2 when the second driver is connected
#define NUM_LED_DRIVERS	1

#define LED_ADDR		0xA0
#define LED1_PORT		GPIOA
#define LED1_SHDN_PIN	LL_GPIO_PIN_12
#define LED2_PORT		GPIOB
#define LED2_SHDN_PIN	LL_GPIO_PIN_13


static void gpioInit(GPIO_TypeDef *GPIOx, uint32_t sclPin, uint32_t sdaPin, uint32_t shdnPin);
static void i2cInit(I2C_TypeDef *I2Cx);

static void animateLEDs(int x, int y, uint8_t *c);
static void solidColors(int x, int y, uint8_t *c);

static GPIO_TypeDef * const LEDx_PORT[] = {LED1_PORT, LED2_PORT};
static I2C_TypeDef * const LEDx_I2C[] = {I2C1, I2C2};
static const uint32_t LEDx_SHDN_PIN[] = {LED1_SHDN_PIN, LED2_SHDN_PIN};

static struct LED l[NUM_LED_DRIVERS];

static volatile uint8_t accelData[6];
static uint8_t currentPhase = 0;
static const uint8_t phaseSpeed = 4;
//static const int panelPhase = 0x80;  // 1/4 of 256
static const int redPhase = 0;
static const int greenPhase = 85;
static const int bluePhase = 170;

uint8_t refreshPending = 0;
uint8_t animateIndex = 6;
static const uint8_t animateColor[][3] = {
		{64,  0,  0},
		{48, 48,  0},
		{ 0, 64,  0},
		{ 0, 48, 48},
		{ 0,  0, 64},
		{48,  0, 48}
};


static const uint8_t sinTable[] = {
  128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244, 245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 93, 90, 88, 85, 82, 79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40, 37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11, 10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9, 10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35, 37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76, 79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124
};

void display_Init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);

    // I2C1: SCL = PA9, SDA = PA10, SHDN = PA12
	gpioInit(LED1_PORT, LL_GPIO_PIN_9, LL_GPIO_PIN_10, LED1_SHDN_PIN);
    LL_SYSCFG_EnableFastModePlus(LL_SYSCFG_I2C_FASTMODEPLUS_I2C1);
	i2cInit(I2C1);

	// I2C2: SCL = PB10, SDA = PB11, SHDN = PB13
	gpioInit(LED2_PORT, LL_GPIO_PIN_10, LL_GPIO_PIN_11, LED2_SHDN_PIN);
    LL_SYSCFG_EnableFastModePlus(LL_SYSCFG_I2C_FASTMODEPLUS_I2C2);
	i2cInit(I2C2);

	for (int i=0; i < NUM_LED_DRIVERS; i++) {
		LED_Init(&l[i], LEDx_I2C[i], LED_ADDR, LEDx_PORT[i], LEDx_SHDN_PIN[i]);
	}

}

void display_Next(void)
{
	animateIndex = (animateIndex + 1) % 7;
	refreshPending = 1;
}

void display_Update(volatile uint8_t *data)
{
	for (int i=0; i < 6; i++) {
		accelData[i] = data[i];
	}

	if (animateIndex < 6) {
		if (refreshPending) {
			refreshPending = 0;
			LED_Update(&l[0], solidColors);
		}
	} else {
		LED_Update(&l[0], animateLEDs);
		currentPhase += phaseSpeed;
	}
}

static void gpioInit(GPIO_TypeDef *GPIOx, uint32_t sclPin, uint32_t sdaPin, uint32_t shdnPin)
{
    LL_GPIO_InitTypeDef gpioConfig;

    // configure SCL/SDA
    LL_GPIO_StructInit(&gpioConfig);
    gpioConfig.Speed = LL_GPIO_SPEED_HIGH;
    gpioConfig.Mode = LL_GPIO_MODE_ALTERNATE;
    gpioConfig.Pull = LL_GPIO_PULL_UP;			    // do we need internal pull--ups?
    gpioConfig.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpioConfig.Alternate = LL_GPIO_AF_4;

    gpioConfig.Pin = sclPin;
    LL_GPIO_Init(GPIOx, &gpioConfig);

    gpioConfig.Pin = sdaPin;
    LL_GPIO_Init(GPIOx, &gpioConfig);

    // configure SHDN pin
    LL_GPIO_ResetOutputPin(GPIOx, shdnPin);

    gpioConfig.Pin = shdnPin;
    gpioConfig.Speed = LL_GPIO_SPEED_LOW;
    gpioConfig.Mode = LL_GPIO_MODE_OUTPUT;
    gpioConfig.Pull = LL_GPIO_PULL_NO;
    gpioConfig.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpioConfig.Alternate = LL_GPIO_AF_0;
    LL_GPIO_Init(GPIOx, &gpioConfig);
}


static void i2cInit(I2C_TypeDef *I2Cx)
{
    LL_I2C_InitTypeDef i2cConfig;

    LL_I2C_StructInit(&i2cConfig);
    i2cConfig.Timing = 0x20A54E50; //0x00B11024;
    //0x00D00E28;  /* (Rise time = 120ns, Fall time = 25ns) */

    LL_I2C_Init(I2Cx, &i2cConfig);

    i2c_initNB(I2Cx);
}






static void animateLEDs(int x, int y, uint8_t *c)
{
	// grab the latest accelData TODO: make atomic and/or double-buffer accelData
	//int8_t Ax = accelData[1];
	int8_t Ay = accelData[3];
	int8_t Az = accelData[5];

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

static void solidColors(int x, int y, uint8_t *c)
{
	for (int i=0; i < 3; i++) {
		c[i] = animateColor[animateIndex][i];
	}
}

