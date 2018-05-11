
#include <stdint.h>

#define ACCEL_ADDR		0x32

enum ClickType {
	ClickSingle,
	ClickDouble,
};

typedef void (clickHandlerCB)(void *ctx, enum ClickType type);
typedef void (dataHandlerCB)(void *ctx, volatile uint8_t *data);

void accel_Init(void);
void accel_setClickHandler(clickHandlerCB cb, void *ctx);
void accel_setDataHandler(dataHandlerCB cb, void *ctx);
void accel_Poll(void);

void accel_config_shutdown(void);
void accel_config_asleep(void);
void accel_config_awake(void);
