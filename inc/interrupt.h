
#include <stdint.h>

enum EXTI_Interrupt {
	ACCEL_INT1,
	LIPO_PG,
	LED1_INTB,
	LED2_INTB,
	ACCEL_INT2,
	LIPO_CHG,

	NUM_EXTI
};


void EXTI_Config(void);
void EXTI_Stop(void);
uint32_t EXTI_GetPinState(enum EXTI_Interrupt i);
void EXTI_SetCallback(enum EXTI_Interrupt i, void (*callback)(void *), void *ctx);
