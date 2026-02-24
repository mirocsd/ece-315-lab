#include <xgpio.h>
#include "rgb_led.h"

void RGB_LED_begin(XGpio *InstancePtr, u32 GPIO_Address) {
    XGpio_Initialize(InstancePtr, GPIO_Address);
    // set direction to output
    XGpio_SetDataDirection(InstancePtr, 2, 0x00);
}