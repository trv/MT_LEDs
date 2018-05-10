
#include <stdint.h>

enum ChargeColor {
	ChargeColorNone,
	ChargeColorRed,
	ChargeColorYellow,
	ChargeColorGreen,
};

void display_Init(void);

void display_Charger(enum ChargeColor color);

void display_Next(void);

void display_Update(volatile uint8_t *accel, volatile uint32_t *als);
