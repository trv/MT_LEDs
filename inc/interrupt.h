

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
void EXTI_SetCallback(enum EXTI_Interrupt i, void (*callback)(void *), void *ctx);
