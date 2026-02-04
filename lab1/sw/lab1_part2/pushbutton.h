#ifndef PUSHBUTTON_H
#define PUSHBUTTON_H
#include <xgpio.h>
#include <xparameters.h>

#define PUSHBUTTON_BASEADDR XPAR_XGPIO_0_BASEADDR
#define PUSHBUTTON_CHANNEL 1 // change

void PUSHBUTTON_begin(XGpio *InstancePtr, u32 GPIO_Address);
u32 PUSHBUTTON_getButtonState(XGpio *InstancePtr);
#endif