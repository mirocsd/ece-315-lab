#include "pushbutton.h"

void PUSHBUTTON_begin(XGpio *InstancePtr, u32 GPIO_Address) {
    XGpio_Initialize(InstancePtr, GPIO_Address);
    // set direction to input
    XGpio_SetDataDirection(InstancePtr, PUSHBUTTON_CHANNEL, 0x1);
}

u32 PUSHBUTTON_getButtonState(XGpio *InstancePtr) {
    return XGpio_DiscreteRead(InstancePtr, PUSHBUTTON_CHANNEL);
}