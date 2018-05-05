

changes for real board:

1. remove internal pull-ups on I2C
2. swap `ACCEL_INT2` and `LIPO_PG` back to the right pins (user button on dev kit is connected to PC13, so use that to simulate battery charger for now). 