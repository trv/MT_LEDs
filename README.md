

changes for real board:

1. remove internal pull-ups on I2C
2. swap `ACCEL_INT2` and `LIPO_PG` back to the right pins (user button on dev kit is connected to PC13, so use that to simulate battery charger for now). 


TODO:
ALS driver
battery status/charger driver


### Interrupts:

pin | ISR | net
--- | --- | ---
PA0  | EXTI0_IRQHandler		| ACCEL_INT1	
PA2  | EXTI2_IRQHandler		| LIPO_PG		
PA11 | EXTI15_10_IRQHandler	| LED1_INTB	
PB12 | EXTI15_10_IRQHandler	| LED2_INTB	
PC13 | EXTI15_10_IRQHandler	| ACCEL_INT2	
PA15 | EXTI15_10_IRQHandler	| LIPO_CHG		

### ADC channels:

pin | channel | net
--- | --- | ---
PA0 | ADC1_IN5 | DUMMY_CHANNEL (see errata 2.5.1)
PA1 | ADC1_IN6 | VBAT_MEAS
PA3 | ADC1_IN8 | ALS1
PA4 | ADC1_IN9 | ALS2
PA5 | ADC1_IN10 | ALS3
PA6 | ADC1_IN11 | ALS4


