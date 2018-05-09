
#include <stdint.h>

enum BatteryStatus {
	BatteryUnknown,
	BatteryLow,
	BatteryNormal,
	BatteryCharging,
	BatteryFull
};

void Battery_Init(void);

enum BatteryStatus Battery_GetStatus(void);

void Battery_UpdateVoltage(uint32_t adcSample);