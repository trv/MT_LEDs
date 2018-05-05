
#include <stdint.h>

void accel_config_awake(void);
void accel_config_asleep(void);

void accel_write(uint8_t subAddr, uint8_t *data, uint8_t len);
void accel_read(uint8_t subAddr, uint8_t *data, uint8_t len);
