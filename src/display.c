
#include "display.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "i2c.h"
#include "led.h"

// TODO: set back to 2 when the second driver is connected
#define NUM_LED_DRIVERS	1
#define FRAME_SIZE		192
#define NUM_LEDS		(64 * NUM_LED_DRIVERS)

#define LED_ADDR		0xA0
#define LED1_PORT		GPIOA
#define LED1_SHDN_PIN	LL_GPIO_PIN_12
#define LED2_PORT		GPIOB
#define LED2_SHDN_PIN	LL_GPIO_PIN_13


static void gpioInit(GPIO_TypeDef *GPIOx, uint32_t sclPin, uint32_t sdaPin, uint32_t shdnPin);
static void i2cInit(I2C_TypeDef *I2Cx);

static void animateLEDs(int x, int y, uint8_t *c);

static GPIO_TypeDef * const LEDx_PORT[] = {LED1_PORT, LED2_PORT};
static I2C_TypeDef * const LEDx_I2C[] = {I2C1, I2C2};
static const uint32_t LEDx_SHDN_PIN[] = {LED1_SHDN_PIN, LED2_SHDN_PIN};

static struct LED l[NUM_LED_DRIVERS];
static uint8_t fb[NUM_LED_DRIVERS*FRAME_SIZE*2];	// room for double-buffering

static volatile uint8_t accelData[6];
static uint32_t alsData = 0;
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


static const uint8_t sinTable[] = {128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244, 245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 93, 90, 88, 85, 82, 79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40, 37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11, 10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9, 10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35, 37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76, 79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124};
static const uint8_t transitionTable[16] = {0,2,10,21,37,57,79,103,128,152,176,198,218,234,245,253};

// per-channel gamma correction
static const uint8_t gR[] = {0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,5,5,5,5,5,6,6,6,6,6,7,7,7,7,7,8,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,15,15,15,16,16,16,16,17,17,17,18,18,18,19,19,19,20,20,20,21,21,21,22,22,23,23,23,24,24,25,25,25,26,26,27,27,27,28,28,29,29,30,30,31,31,31,32,32,33,33,34,34,35,35,36,36,37,38,38,39,39,40,40,41,41,42,43,43,44,44,45,46,46,47,48,48,49,50,50,51,52,52,53,54,54,55,56,57,57,58,59,60,60,61,62,63,64,65,65,66,67,68,69,70,71,72,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,90,91,92,93,94,95,96,98,99,100,101,102,104,105,106,108,109,110,112,113,114,116,117,118,120,121,123,124,126,127,129,130,132,133,135,137,138,140,142,143,145,147,148,150,152,154,156,157,159,161,163,165,167,169,171,173,175,177,179,181};
static const uint8_t gG[] = {0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,8,8,8,8,8,8,9,9,9,9,9,9,10,10,10,10,10,11,11,11,11,11,12,12,12,12,12,13,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,20,20,20,21,21,21,21,22,22,22,23,23,23,24,24,24,25,25,25,26,26,27,27,27,28,28,28,29,29,30,30,30,31,31,32,32,32,33,33,34,34,35,35,36,36,37,37,38,38,38,39,40,40,41,41,42,42,43,43,44,44,45,45,46,47,47,48,48,49,50,50,51,51,52,53,53,54,55,55,56,57,58,58,59,60,60,61,62,63,63,64,65,66,67,67,68,69,70,71,71,72,73,74,75,76,77,78,79,80,81,81,82,83,84,85,86,87,88,90,91,92,93,94,95,96,97,98,99,101,102,103,104,105,107,108};
static const uint8_t gB[] = {0,0,0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,10,10,10,10,11,11,11,12,12,12,13,13,13,14,14,14,15,15,15,16,16,16,17,17,17,18,18,19,19,19,20,20,21,21,21,22,22,23,23,24,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,32,32,33,33,34,35,35,36,36,37,37,38,39,39,40,40,41,42,42,43,44,44,45,46,46,47,48,48,49,50,51,51,52,53,54,54,55,56,57,58,58,59,60,61,62,63,64,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,88,89,90,91,92,93,95,96,97,98,100,101,102,103,105,106,107,109,110,111,113,114,116,117,119,120,122,123,125,126,128,129,131,133,134,136,138,139,141,143,144,146,148,150,152,153,155,157,159,161,163,165,167,169,171,173,175,177,179,181,184,186,188,190,193,195,197,199,202,204,207,209,212,214,217,219,222,224,227,230,232,235,238,241,243,246,249,252,255};

// LED position and channel index info
static const uint8_t iR[] = {176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};
static const uint8_t iG[] = {160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
static const uint8_t iB[] = {144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

static const int8_t pX[] = {-126,-90,-54,-18,18,54,90,126,-126,-90,-54,-18,18,54,90,126,-126,-90,-54,-18,18,54,90,126,-126,-90,-54,-18,18,54,90,126,-126,-90,-54,-18,18,54,90,126,-126,-90,-54,-18,18,54,90,126,-126,-90,-54,-18,18,54,90,126,-126,-90,-54,-18,18,54,90,126};
static const int8_t pY[] = {126,126,126,126,126,126,126,126,90,90,90,90,90,90,90,90,54,54,54,54,54,54,54,54,18,18,18,18,18,18,18,18,-18,-18,-18,-18,-18,-18,-18,-18,-54,-54,-54,-54,-54,-54,-54,-54,-90,-90,-90,-90,-90,-90,-90,-90,-126,-126,-126,-126,-126,-126,-126,-126};

static const uint8_t border[28] = {3,2,1,0,8,16,24,32,40,48,56,57,58,59,60,61,62,63,55,47,39,31,23,15,7,6,5,4};
static const uint8_t center[64-28] = {11,19,18,10,9,17,25,26,27,35,34,33,41,49,50,42,43,51,52,44,45,53,54,46,38,37,36,28,29,30,22,14,13,21,20,12};


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
	animateIndex = (animateIndex + 1) % 8;
	currentPhase = 0;
	refreshPending = 1;
}

void display_Update(volatile uint8_t *accel, volatile uint32_t *als)
{
	for (int i=0; i < 6; i++) {
		accelData[i] = accel[i];
	}

	uint32_t sum = 0;
	for (int i=0; i < 4; i++) {
		sum += als[i];
	}
	// ALS data is 12-bit, 4 samples (+2 bits) = 14-bit data.
	// multiply by 16 to avoid truncation in IIR filter (+4 bits = 18-bit data)
	sum = sum << 4;

	// display updates at 50Hz, 1-(15/16)^50 = 96% of the new value after 1 second
	alsData = (alsData * 15 + sum + 8) >> 4;

	// alsData is 18 bits, convert to global brightness setting
	// 1.6V = max brightness, ADC reference = 3.3V (for now)
	// max data = (2^18-1) * 1.6 / 3.3 = 127100
	// min data = (2^18-1) * 0.01 / 3.3 ~= 800
	// global brightness range = 1-128
	uint8_t globalBrightness = 1 + (alsData / 1000);
	LED_SetBrightness(&l[0], globalBrightness);	// BLOCKING CALL

	if (animateIndex < 6) {
		if (refreshPending) {
			refreshPending = 0;
			for (int i = 0; i < NUM_LEDS; i++) {
				fb[iR[i]] = animateColor[animateIndex][0];
				fb[iG[i]] = animateColor[animateIndex][1];
				fb[iB[i]] = animateColor[animateIndex][2];
			}
			LED_Update(&l[0], fb);
		}
	} else if (animateIndex == 6) {
		uint8_t c[3];
		for (int i = 0; i < NUM_LEDS; i++) {
			animateLEDs(pX[i], pY[i], c);
			fb[iR[i]] = gR[c[0]];
			fb[iG[i]] = gG[c[1]];
			fb[iB[i]] = gB[c[2]];
		}
		LED_Update(&l[0], fb);
		currentPhase += phaseSpeed;
	} else if (animateIndex == 7) {
		for (int i = 0; i < (7*4); i++) {
			int j = border[i];
			int32_t phase = currentPhase - (256*i)/28;
			if (phase < 0) { phase = 0; }
			if (phase > 15) { phase = 15; }
			fb[iR[j]] = gR[transitionTable[phase]];
			fb[iG[j]] = 0;
			fb[iB[j]] = 0;
		}
		for (int i = 0; i < 36; i++) {
			int j = center[i];
			int32_t phase = currentPhase - (256*i)/36;
			if (phase < 0) { phase = 0; }
			if (phase > 15) { phase = 15; }
			fb[iR[j]] = 0;
			fb[iG[j]] = 0;
			fb[iB[j]] = gB[transitionTable[phase]];
		}
		LED_Update(&l[0], fb);
		currentPhase++;
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

	int32_t offset = ((-x * Az) + (y * Ay))/64;
	if (offset > 127) {
		offset = 127;
	} else if (offset < -127) {
		offset = -127;
	}
	c[0] = sinTable[(uint8_t)(currentPhase + offset + redPhase)]/2;
	c[1] = sinTable[(uint8_t)(currentPhase + offset + greenPhase)]/2;
	c[2] = sinTable[(uint8_t)(currentPhase + offset + bluePhase)]/2;
}
