

changes for real board:

1. remove internal pull-ups on I2C
2. swap `ACCEL_INT2` and `LIPO_PG` back to the right pins (user button on dev kit is connected to PC13, so use that to simulate battery charger for now). 


TODO:
ALS driver
battery status/charger driver


Interrupts:

`PA0 `: `ACCEL_INT1`		(`EXTI0_IRQHandler`)  
`PA2 `: `LIPO_PG`			(`EXTI2_IRQHandler`)  
`PA11`: `LED1_INTB`			(`EXTI15_10_IRQHandler`)  
`PB12`: `LED2_INTB`			(`EXTI15_10_IRQHandler`)  
`PC13`: `ACCEL_INT2`		(`EXTI15_10_IRQHandler`)  
`PA15`: `LIPO_CHG`			(`EXTI15_10_IRQHandler`)  

ADC channels:

pin | channel | net
- | - | -
PA0 | ADC1_IN5 | DUMMY_CHANNEL (see errata 2.5.1)
PA1 | ADC1_IN6 | VBAT_MEAS
PA3 | ADC1_IN8 | ALS1
PA4 | ADC1_IN9 | ALS2
PA5 | ADC1_IN10 | ALS3
PA6 | ADC1_IN11 | ALS4


