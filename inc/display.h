
#include <stdint.h>

void display_Init(void);

void display_Next(void);

void display_Update(volatile uint8_t *accel, volatile uint32_t *als);
