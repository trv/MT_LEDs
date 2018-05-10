
#include "display.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

#include "i2c.h"
#include "led.h"

// TODO: set back to 2 when the second driver is connected
#define NUM_LED_DRIVERS	2
#define FRAME_SIZE		192
#define NUM_LEDS		(64 * NUM_LED_DRIVERS)

#define LED_ADDR		0xA0
#define LED1_PORT		GPIOA
#define LED1_SHDN_PIN	LL_GPIO_PIN_12
#define LED2_PORT		GPIOB
#define LED2_SHDN_PIN	LL_GPIO_PIN_13


static void gpioInit(GPIO_TypeDef *GPIOx, uint32_t sclPin, uint32_t sdaPin, uint32_t shdnPin);
static void i2cInit(I2C_TypeDef *I2Cx);
static void refresh(void);
static void writeFB(void);
static void animateLEDs(int x, int y, uint8_t *c);
static void getColor(uint8_t phase, uint8_t *c);

static void fillAll(uint8_t *c, const uint8_t *index, uint8_t size);
static void drawWave(uint8_t phase, uint8_t *c1, uint8_t *c2, const uint8_t *index, uint8_t size);


static GPIO_TypeDef * const LEDx_PORT[] = {LED1_PORT, LED2_PORT};
static I2C_TypeDef * const LEDx_I2C[] = {I2C1, I2C2};
static const uint32_t LEDx_SHDN_PIN[] = {LED1_SHDN_PIN, LED2_SHDN_PIN};

static struct LED l[NUM_LED_DRIVERS];
static uint8_t fb[NUM_LED_DRIVERS*FRAME_SIZE*2];	// room for double-buffering

static volatile uint8_t accelData[6];
static uint32_t alsData = 0;
#define NUM_WAVES	16
static uint8_t currentPhase[NUM_WAVES] = {0};
static const uint8_t waveSpeed = 2;
static const uint8_t phaseSpeed = 4;
//static const int panelPhase = 0x80;  // 1/4 of 256
static const int redPhase = 170;
static const int greenPhase = 95;
static const int bluePhase = 0;

static uint8_t colorPhase = 42;
static uint8_t colorSpeed = 95;
uint8_t refreshPending = 0;
uint8_t animateIndex = 1;

static enum ChargeColor chgColor;


static const uint8_t sinTable[] = {128, 131, 134, 137, 140, 143, 146, 149, 152, 155, 158, 162, 165, 167, 170, 173, 176, 179, 182, 185, 188, 190, 193, 196, 198, 201, 203, 206, 208, 211, 213, 215, 218, 220, 222, 224, 226, 228, 230, 232, 234, 235, 237, 238, 240, 241, 243, 244, 245, 246, 248, 249, 250, 250, 251, 252, 253, 253, 254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 254, 254, 254, 253, 253, 252, 251, 250, 250, 249, 248, 246, 245, 244, 243, 241, 240, 238, 237, 235, 234, 232, 230, 228, 226, 224, 222, 220, 218, 215, 213, 211, 208, 206, 203, 201, 198, 196, 193, 190, 188, 185, 182, 179, 176, 173, 170, 167, 165, 162, 158, 155, 152, 149, 146, 143, 140, 137, 134, 131, 128, 124, 121, 118, 115, 112, 109, 106, 103, 100, 97, 93, 90, 88, 85, 82, 79, 76, 73, 70, 67, 65, 62, 59, 57, 54, 52, 49, 47, 44, 42, 40, 37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 17, 15, 14, 12, 11, 10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9, 10, 11, 12, 14, 15, 17, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35, 37, 40, 42, 44, 47, 49, 52, 54, 57, 59, 62, 65, 67, 70, 73, 76, 79, 82, 85, 88, 90, 93, 97, 100, 103, 106, 109, 112, 115, 118, 121, 124};
static const uint8_t transitionTable[17] = {0,2,10,21,37,57,79,103,128,152,176,198,218,234,245,253,255};

// per-channel gamma correction
static const uint8_t gR[] = {0,0,0,0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,5,5,5,5,5,6,6,6,6,6,7,7,7,7,7,8,8,8,8,8,9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,15,15,15,16,16,16,16,17,17,17,18,18,18,19,19,19,20,20,20,21,21,21,22,22,23,23,23,24,24,25,25,25,26,26,27,27,27,28,28,29,29,30,30,31,31,31,32,32,33,33,34,34,35,35,36,36,37,38,38,39,39,40,40,41,41,42,43,43,44,44,45,46,46,47,48,48,49,50,50,51,52,52,53,54,54,55,56,57,57,58,59,60,60,61,62,63,64,65,65,66,67,68,69,70,71,72,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,90,91,92,93,94,95,96,98,99,100,101,102,104,105,106,108,109,110,112,113,114,116,117,118,120,121,123,124,126,127,129,130,132,133,135,137,138,140,142,143,145,147,148,150,152,154,156,157,159,161,163,165,167,169,171,173,175,177,179,181};
static const uint8_t gG[] = {0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,7,7,7,7,7,7,8,8,8,8,8,8,9,9,9,9,9,9,10,10,10,10,10,11,11,11,11,11,12,12,12,12,12,13,13,13,13,13,14,14,14,14,15,15,15,15,16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,20,20,20,21,21,21,21,22,22,22,23,23,23,24,24,24,25,25,25,26,26,27,27,27,28,28,28,29,29,30,30,30,31,31,32,32,32,33,33,34,34,35,35,36,36,37,37,38,38,38,39,40,40,41,41,42,42,43,43,44,44,45,45,46,47,47,48,48,49,50,50,51,51,52,53,53,54,55,55,56,57,58,58,59,60,60,61,62,63,63,64,65,66,67,67,68,69,70,71,71,72,73,74,75,76,77,78,79,80,81,81,82,83,84,85,86,87,88,90,91,92,93,94,95,96,97,98,99,101,102,103,104,105,107,108};
static const uint8_t gB[] = {0,0,0,1,1,1,1,1,2,2,2,2,2,3,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,9,9,9,10,10,10,10,11,11,11,12,12,12,13,13,13,14,14,14,15,15,15,16,16,16,17,17,17,18,18,19,19,19,20,20,21,21,21,22,22,23,23,24,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,32,32,33,33,34,35,35,36,36,37,37,38,39,39,40,40,41,42,42,43,44,44,45,46,46,47,48,48,49,50,51,51,52,53,54,54,55,56,57,58,58,59,60,61,62,63,64,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,88,89,90,91,92,93,95,96,97,98,100,101,102,103,105,106,107,109,110,111,113,114,116,117,119,120,122,123,125,126,128,129,131,133,134,136,138,139,141,143,144,146,148,150,152,153,155,157,159,161,163,165,167,169,171,173,175,177,179,181,184,186,188,190,193,195,197,199,202,204,207,209,212,214,217,219,222,224,227,230,232,235,238,241,243,246,249,252,255};

// LED position and channel index info
static const uint16_t iR[] = {176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,368,369,370,371,372,373,374,375,376,377,378,379,380,381,382,383,320,321,322,323,324,325,326,327,328,329,330,331,332,333,334,335,272,273,274,275,276,277,278,279,280,281,282,283,284,285,286,287,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239};
static const uint16_t iG[] = {160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,352,353,354,355,356,357,358,359,360,361,362,363,364,365,366,367,304,305,306,307,308,309,310,311,312,313,314,315,316,317,318,319,256,257,258,259,260,261,262,263,264,265,266,267,268,269,270,271,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223};
static const uint16_t iB[] = {144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,336,337,338,339,340,341,342,343,344,345,346,347,348,349,350,351,288,289,290,291,292,293,294,295,296,297,298,299,300,301,302,303,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207};

static const int8_t pX[] = {-127,-109,-91,-73,-55,-37,-19,-1,-127,-109,-91,-73,-55,-37,-19,-1,-127,-109,-91,-73,-55,-37,-19,-1,-127,-109,-91,-73,-55,-37,-19,-1,-127,-109,-91,-73,-55,-37,-19,-1,-127,-109,-91,-73,-55,-37,-19,-1,-127,-109,-91,-73,-55,-37,-19,-1,-127,-109,-91,-73,-55,-37,-19,-1,1,19,37,55,73,91,109,127,1,19,37,55,73,91,109,127,1,19,37,55,73,91,109,127,1,19,37,55,73,91,109,127,1,19,37,55,73,91,109,127,1,19,37,55,73,91,109,127,1,19,37,55,73,91,109,127,1,19,37,55,73,91,109,127};
static const int8_t pY[] = {63,63,63,63,63,63,63,63,45,45,45,45,45,45,45,45,27,27,27,27,27,27,27,27,9,9,9,9,9,9,9,9,-9,-9,-9,-9,-9,-9,-9,-9,-27,-27,-27,-27,-27,-27,-27,-27,-45,-45,-45,-45,-45,-45,-45,-45,-63,-63,-63,-63,-63,-63,-63,-63,63,63,63,63,63,63,63,63,45,45,45,45,45,45,45,45,27,27,27,27,27,27,27,27,9,9,9,9,9,9,9,9,-9,-9,-9,-9,-9,-9,-9,-9,-27,-27,-27,-27,-27,-27,-27,-27,-45,-45,-45,-45,-45,-45,-45,-45,-63,-63,-63,-63,-63,-63,-63,-63};

static const uint8_t border[44] = {32,40,48,56,57,58,59,60,61,62,63,120,121,122,123,124,125,126,127,119,111,103,95,87,79,71,70,69,68,67,66,65,64,7,6,5,4,3,2,1,0,8,16,24};
static const uint8_t center[128-44] = {33,34,42,41,49,50,51,43,35,36,37,45,44,52,53,54,55,47,46,38,39,96,97,105,104,112,113,114,115,107,106,98,99,100,101,109,108,116,117,118,110,102,94,86,78,77,76,84,85,93,92,91,90,82,83,75,74,73,72,80,81,89,88,31,30,22,23,15,14,13,12,20,21,29,28,27,19,11,10,9,17,18,26,25};
static const uint8_t charger[2] = {24,32}; // final values: {76,127};
static uint8_t chargerBackup[2][3];

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

	chgColor = ChargeColorNone;

	uint8_t c[3];
	getColor(colorPhase, c);
	fillAll(c, border, sizeof(border));
	getColor(colorPhase + 128, c);
	fillAll(c, center, sizeof(center));
	refreshPending = 1;
}

void display_Charger(enum ChargeColor color)
{
	if (chgColor == ChargeColorNone) {
		for (int i = 0; i < sizeof(charger); i++) {
			// save real color
			chargerBackup[i][0] = fb[iR[charger[i]]];
			chargerBackup[i][1] = fb[iG[charger[i]]];
			chargerBackup[i][2] = fb[iB[charger[i]]];
		}
	}

	if (color == ChargeColorNone) {
		// restore overwritten pixels
		for (int i = 0; i < sizeof(charger); i++) {
			// save real color
			fb[iR[charger[i]]] = chargerBackup[i][0];
			fb[iG[charger[i]]] = chargerBackup[i][1];
			fb[iB[charger[i]]] = chargerBackup[i][2];
		}
	}

	chgColor = color;

	refreshPending = 1;
}

void display_Next(void)
{
	animateIndex = (animateIndex + 1) % 12;
	for (int i = 0; i < NUM_WAVES; i++) {
		if (currentPhase[i] == 0) {
			currentPhase[i] = waveSpeed;
			break;
		}
	}
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
	// 1.6V = max brightness, ADC reference = 1.8V (for now)
	// max data = (2^18-1) * 1.6 / 1.8 = 233016
	// min data = (2^18-1) * 0.005 / 1.8 ~= 728
	// global brightness range = 1-255
	uint16_t globalBrightness = 1 + (alsData / 800);
	if (globalBrightness > 255) {
		// saturate to 8-bit
		globalBrightness = 255;
	}
	LED_SetBrightness(&l[0], globalBrightness);	// BLOCKING CALL
	LED_SetBrightness(&l[1], (globalBrightness+7)/8);	// BLOCKING CALL

	refresh();
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

static void drawWave(uint8_t phase, uint8_t *c1, uint8_t *c2, const uint8_t *index, uint8_t size)
{
	for (int i = 0; i < size; i++) {
		int j = index[i];
		int32_t activePhase = phase - (240*i)/size;
		if (activePhase >= 0 && activePhase <= 16) {
			uint8_t transition = transitionTable[activePhase];
			fb[iR[j]] = gR[(c1[0]*(255-transition) + c2[0]*transition) >> 8];
			fb[iG[j]] = gG[(c1[1]*(255-transition) + c2[1]*transition) >> 8];
			fb[iB[j]] = gB[(c1[2]*(255-transition) + c2[2]*transition) >> 8];
		}
	}
}

static void fillAll(uint8_t *c, const uint8_t *index, uint8_t size)
{
	for (int i = 0; i < size; i++) {
		int j = index[i];

		fb[iR[j]] = gR[c[0]];
		fb[iG[j]] = gG[c[1]];
		fb[iB[j]] = gB[c[2]];
	}
}

static void refresh(void)
{
	if (animateIndex < 12) {
		if (currentPhase[0] == 0) {
			if (refreshPending) {
				writeFB();
			}
		} else {

			uint8_t c1[3];
			uint8_t c2[3];

			for (int i = 0; i < NUM_WAVES; i++) {
				if (currentPhase[i] == 0) { break; }
				getColor(colorPhase + colorSpeed*i, c1);
				getColor(colorPhase + colorSpeed*(i+1), c2);
				drawWave(currentPhase[i], c1, c2, border, sizeof(border));

				getColor(colorPhase + colorSpeed*i + 128, c1);
				getColor(colorPhase + colorSpeed*(i+1) + 128, c2);
				drawWave(currentPhase[i], c1, c2, center, sizeof(center));

				currentPhase[i] += waveSpeed;
				if (currentPhase[i] < waveSpeed) {
					// shift down the rest of things
					for (int j = i; j+1 < NUM_WAVES; j++) {
						currentPhase[j] = currentPhase[j+1];
					}
					i--;
					colorPhase += colorSpeed;
				}
			}

			writeFB();

		}
	} else if (animateIndex == 6) {
		uint8_t c[3];
		for (int i = 0; i < NUM_LEDS; i++) {
			animateLEDs(pX[i], pY[i], c);
			fb[iR[i]] = gR[c[0]];
			fb[iG[i]] = gG[c[1]];
			fb[iB[i]] = gB[c[2]];
		}
		writeFB();
		currentPhase[0] += phaseSpeed;
	} else if (animateIndex == 7) {

	}
}

static void writeFB(void)
{
	if (chgColor != ChargeColorNone) {
		for (int i = 0; i < sizeof(charger); i++) {
			if (!refreshPending) {
				// save real color
				chargerBackup[i][0] = fb[iR[charger[i]]];
				chargerBackup[i][1] = fb[iG[charger[i]]];
				chargerBackup[i][2] = fb[iB[charger[i]]];
			}

			// overwrite with charger indication
			fb[iR[charger[i]]] = gR[(chgColor == ChargeColorGreen) ? 0 : 255];
			fb[iG[charger[i]]] = gG[(chgColor == ChargeColorRed) ? 0 : 255];
			fb[iB[charger[i]]] = gB[0];
		}
	}

	LED_Update(&l[0], fb);
	LED_Update(&l[1], &fb[FRAME_SIZE]);

	refreshPending = 0;
}

static void animateLEDs(int x, int y, uint8_t *c)
{
	// grab the latest accelData TODO: make atomic and/or double-buffer accelData
	int8_t Ax = accelData[1];
	int8_t Ay = accelData[3];
	//int8_t Az = accelData[5];

	int32_t offset = ((x * Ax) + (y * Ay))/64;
	if (offset > 127) {
		offset = 127;
	} else if (offset < -127) {
		offset = -127;
	}
	getColor(currentPhase[0] + offset, c);
}

static void getColor(uint8_t phase, uint8_t *c)
{
	c[0] = sinTable[(uint8_t)(phase + redPhase)];
	c[1] = sinTable[(uint8_t)(phase + greenPhase)];
	c[2] = sinTable[(uint8_t)(phase + bluePhase)];
}
