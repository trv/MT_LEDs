

enum EXTI_Interrupt {
	ACCEL_INT1,
	ACCEL_INT2,		// TODO: swap order with LIPO_PG
	LED1_INTB,
	LED2_INTB,
	LIPO_PG,
	LIPO_CHG,

	NUM_EXTI
};


void EXTI_Config(void);
void EXTI_Stop(void);
void EXTI_SetCallback(enum EXTI_Interrupt i, void (*callback)(void *), void *ctx);
