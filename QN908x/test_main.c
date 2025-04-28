#include "hal.h"

#include "LED.h"    // ! temporary solution - remove later.

static void _toggleGreen(void)
{
    Led2Toggle();
}

int main(void)
{
    unsigned int i = 0;

    HAL_status_t hwStatus = HAL_init_hardware();
    if (hwStatus != HAL_STATUS_OK) {
        HAL_ASSERT(0);
    }

    while(1) {
        i++;
        if (i == 1000000) {
            _toggleGreen();
            i = 0;
        }
    }
    return 0;
}


/* End of FILE */