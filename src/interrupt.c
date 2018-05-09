
#include "interrupt.h"

#include "stm32l4xx.h"
#include "stm32l4xx_conf.h"

// GPIO Config
#define ACCEL_INT1_PIN		LL_GPIO_PIN_0
#define ACCEL_INT2_PIN		LL_GPIO_PIN_13
#define LIPO_PG_PIN			LL_GPIO_PIN_2
#define LIPO_CHG_PIN		LL_GPIO_PIN_15
#define LED1_PIN			LL_GPIO_PIN_11
#define LED2_PIN			LL_GPIO_PIN_12

#define ACCEL_INT1_PORT		GPIOA
#define ACCEL_INT2_PORT		GPIOC
#define LIPO_PG_PORT		GPIOA
#define LIPO_CHG_PORT		GPIOA
#define LED1_PORT			GPIOA
#define LED2_PORT 			GPIOB

// SYSCFG EXTI Config
#define ACCEL_INT1_SYSCFG_EXTI_PORT		LL_SYSCFG_EXTI_PORTA
#define ACCEL_INT2_SYSCFG_EXTI_PORT		LL_SYSCFG_EXTI_PORTC
#define LIPO_PG_SYSCFG_EXTI_PORT		LL_SYSCFG_EXTI_PORTA
#define LIPO_CHG_SYSCFG_EXTI_PORT		LL_SYSCFG_EXTI_PORTA
#define LED1_SYSCFG_EXTI_PORT			LL_SYSCFG_EXTI_PORTA
#define LED2_SYSCFG_EXTI_PORT 			LL_SYSCFG_EXTI_PORTB

#define ACCEL_INT1_SYSCFG_EXTI_LINE		LL_SYSCFG_EXTI_LINE0
#define ACCEL_INT2_SYSCFG_EXTI_LINE		LL_SYSCFG_EXTI_LINE13
#define LIPO_PG_SYSCFG_EXTI_LINE		LL_SYSCFG_EXTI_LINE2
#define LIPO_CHG_SYSCFG_EXTI_LINE		LL_SYSCFG_EXTI_LINE15
#define LED1_SYSCFG_EXTI_LINE			LL_SYSCFG_EXTI_LINE11
#define LED2_SYSCFG_EXTI_LINE			LL_SYSCFG_EXTI_LINE12

// EXTI Config
#define ACCEL_INT1_EXTI_LINE	LL_EXTI_LINE_0
#define ACCEL_INT2_EXTI_LINE	LL_EXTI_LINE_13
#define LIPO_PG_EXTI_LINE		LL_EXTI_LINE_2
#define LIPO_CHG_EXTI_LINE		LL_EXTI_LINE_15
#define LED1_EXTI_LINE			LL_EXTI_LINE_11
#define LED2_EXTI_LINE			LL_EXTI_LINE_12

static const uint32_t SYSCFG_EXTI_PORT[] = {
		[ACCEL_INT1] = ACCEL_INT1_SYSCFG_EXTI_PORT,
		[ACCEL_INT2] = ACCEL_INT2_SYSCFG_EXTI_PORT,
		[LIPO_PG]    = LIPO_PG_SYSCFG_EXTI_PORT,
		[LIPO_CHG]   = LIPO_CHG_SYSCFG_EXTI_PORT,
		[LED1_INTB]  = LED1_SYSCFG_EXTI_PORT,
		[LED2_INTB]  = LED2_SYSCFG_EXTI_PORT
};

static const uint32_t SYSCFG_EXTI_LINE[] = {
		[ACCEL_INT1] = ACCEL_INT1_SYSCFG_EXTI_LINE,
		[ACCEL_INT2] = ACCEL_INT2_SYSCFG_EXTI_LINE,
		[LIPO_PG]    = LIPO_PG_SYSCFG_EXTI_LINE,
		[LIPO_CHG]   = LIPO_CHG_SYSCFG_EXTI_LINE,
		[LED1_INTB]  = LED1_SYSCFG_EXTI_LINE,
		[LED2_INTB]  = LED2_SYSCFG_EXTI_LINE
};

static const uint32_t EXTI_LINE[] = {
		[ACCEL_INT1] = ACCEL_INT1_EXTI_LINE,
		[ACCEL_INT2] = ACCEL_INT2_EXTI_LINE,
		[LIPO_PG]    = LIPO_PG_EXTI_LINE,
		[LIPO_CHG]   = LIPO_CHG_EXTI_LINE,
		[LED1_INTB]  = LED1_EXTI_LINE,
		[LED2_INTB]  = LED2_EXTI_LINE
};

static const uint8_t EXTI_TRIGGER[] = {
		[ACCEL_INT1] = LL_EXTI_TRIGGER_RISING,
		[ACCEL_INT2] = LL_EXTI_TRIGGER_RISING,
		[LIPO_PG]    = LL_EXTI_TRIGGER_RISING_FALLING,
		[LIPO_CHG]   = LL_EXTI_TRIGGER_RISING_FALLING,
		[LED1_INTB]  = LL_EXTI_TRIGGER_FALLING,
		[LED2_INTB]  = LL_EXTI_TRIGGER_FALLING
};

static const uint32_t GPIO_PIN[] = {
		[ACCEL_INT1] = ACCEL_INT1_PIN,
		[ACCEL_INT2] = ACCEL_INT2_PIN,
		[LIPO_PG]    = LIPO_PG_PIN,
		[LIPO_CHG]   = LIPO_CHG_PIN,
		[LED1_INTB]  = LED1_PIN,
		[LED2_INTB]  = LED2_PIN
};

static GPIO_TypeDef * const GPIO_PORT[] = {
		[ACCEL_INT1] = ACCEL_INT1_PORT,
		[ACCEL_INT2] = ACCEL_INT2_PORT,
		[LIPO_PG]    = LIPO_PG_PORT,
		[LIPO_CHG]   = LIPO_CHG_PORT,
		[LED1_INTB]  = LED1_PORT,
		[LED2_INTB]  = LED2_PORT
};

static const uint32_t GPIO_PULL[] = {
		[ACCEL_INT1] = LL_GPIO_PULL_NO,
		[ACCEL_INT2] = LL_GPIO_PULL_NO,
		[LIPO_PG]    = LL_GPIO_PULL_UP,
		[LIPO_CHG]   = LL_GPIO_PULL_UP,
		[LED1_INTB]  = LL_GPIO_PULL_UP,		// todo: remove pull up
		[LED2_INTB]  = LL_GPIO_PULL_UP		// todo: remove pull up
};

static void (*cb[NUM_EXTI])(void *);
static void *cbCtx[NUM_EXTI];

void EXTI_Config(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    LL_GPIO_InitTypeDef gpioConfig;
    LL_GPIO_StructInit(&gpioConfig);

    gpioConfig.Speed = LL_GPIO_SPEED_LOW;
    gpioConfig.Mode = LL_GPIO_MODE_INPUT;

    for (int i=0; i < NUM_EXTI; i++) {
		gpioConfig.Pin = GPIO_PIN[i];
	    gpioConfig.Pull = GPIO_PULL[i];
		LL_GPIO_Init(GPIO_PORT[i], &gpioConfig);
    }

    LL_EXTI_InitTypeDef extiConfig;
    LL_EXTI_StructInit(&extiConfig);

    extiConfig.LineCommand = ENABLE;

    for (int i=0; i < NUM_EXTI; i++) {
    	LL_SYSCFG_SetEXTISource(SYSCFG_EXTI_PORT[i], SYSCFG_EXTI_LINE[i]);

    	extiConfig.Line_0_31 = EXTI_LINE[i];
    	extiConfig.Trigger = EXTI_TRIGGER[i];
    	LL_EXTI_Init(&extiConfig);
    }

	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	NVIC_SetPriority(EXTI0_IRQn, 3);
	NVIC_EnableIRQ(EXTI0_IRQn);

	NVIC_ClearPendingIRQ(EXTI2_IRQn);
	NVIC_SetPriority(EXTI2_IRQn, 3);
	NVIC_EnableIRQ(EXTI2_IRQn);

	NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
	NVIC_SetPriority(EXTI15_10_IRQn, 3);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI_Stop(void)
{
	NVIC_DisableIRQ(EXTI0_IRQn);
	NVIC_DisableIRQ(EXTI2_IRQn);
	NVIC_DisableIRQ(EXTI15_10_IRQn);

	NVIC_ClearPendingIRQ(EXTI0_IRQn);
	NVIC_ClearPendingIRQ(EXTI2_IRQn);
	NVIC_ClearPendingIRQ(EXTI15_10_IRQn);

}

void EXTI_SetCallback(enum EXTI_Interrupt i, void (*callback)(void *), void *ctx)
{
	cb[i] = callback;
	cbCtx[i] = ctx;
}

void EXTI0_IRQHandler(void)
{
	LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_0);
	if (cb[0]) {
		cb[0](cbCtx[0]);
	}
}

void EXTI2_IRQHandler(void)
{
	LL_EXTI_ClearFlag_0_31(LL_EXTI_LINE_2);
	if (cb[1]) {
		cb[1](cbCtx[1]);
	}
}

void EXTI15_10_IRQHandler(void)
{
	for (int i=2; i < NUM_EXTI; i++) {
		if (LL_EXTI_IsActiveFlag_0_31(EXTI_LINE[i])) {
			LL_EXTI_ClearFlag_0_31(EXTI_LINE[i]);
			if (cb[i]) {
				cb[i](cbCtx[i]);
			}
		}
	}
}
