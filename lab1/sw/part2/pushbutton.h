#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H
#include "xparameters.h"

#define PUSHBUTTON_BASEADDR XPAR_GPIO_PUSHBUTTON_BASEADDR
#define PUSHBUTTON_CHANNEL 1 // change

void PUSHBUTTON_begin(XGpio *InstancePtr, u32 GPIO_Address);
u32 PUSHBUTTON_getButtonState(XGpio *InstancePtr);
#endif