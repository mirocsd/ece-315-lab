#include "rgb_led.h"

void RGB_LED_begin(RGB_LED *InstancePtr, u32 GPIO_Address) {
    XGpio_Initialize(&InstancePtr->GPIO_inst, GPIO_Address);
    // set direction to output
    XGpio_SetDataDirection(&InstancePtr->GPIO_inst, 2, 0x00);
}