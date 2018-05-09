
#include <stdint.h>

enum ShutdownReason {
    ShutdownReason_LowBattery,
    ShutdownReason_TurnOff,
    ShutdownReason_Lockup,
};

uint32_t Power_Init(void);

void Power_Shutdown(enum ShutdownReason reason);

