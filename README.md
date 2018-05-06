

changes for real board:

1. remove internal pull-ups on I2C
2. swap `ACCEL_INT2` and `LIPO_PG` back to the right pins (user button on dev kit is connected to PC13, so use that to simulate battery charger for now). 



Interrupts:

`PA0 `: `ACCEL_INT1`		(`EXTI0_IRQHandler`)
`PA2 `: `LIPO_PG`			(`EXTI2_IRQHandler`)
`PA11`: `LED1_INTB`			(`EXTI15_10_IRQHandler`)
`PB12`: `LED2_INTB`			(`EXTI15_10_IRQHandler`)
`PC13`: `ACCEL_INT2`		(`EXTI15_10_IRQHandler`)
`PA15`: `LIPO_CHG`			(`EXTI15_10_IRQHandler`)