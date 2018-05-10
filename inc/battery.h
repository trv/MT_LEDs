
#include <stdint.h>

enum BatteryStatus {
	BatteryUnknown,
	BatteryCritical,
	BatteryLow,
	BatteryNormal,
	BatteryCharging,
	BatteryFull
};

void Battery_Init(void);
void Battery_SetCallback(void (*callback)(void *, enum BatteryStatus status, uint32_t battery_mV), void *ctx);

enum BatteryStatus Battery_GetStatus(void);
uint32_t Battery_GetLevel_mV(void);

void Battery_UpdateVoltage(uint32_t adcSample);
