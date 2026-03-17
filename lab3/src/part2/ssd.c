#include "xil_io.h"
#include "xil_types.h"
#include "ssd.h"

void SSD_begin(SSD *InstancePtr, u32 GPIO_Address) {
    XGpio_Initialize(&InstancePtr->GPIO_inst, GPIO_Address);
    XGpio_SetDataDirection(&InstancePtr->GPIO_inst, 1, 0x00);
}

void SSD_setSSD(SSD *InstancePtr, u8 ssd_value) {
    XGpio_DiscreteWrite(&InstancePtr->GPIO_inst, 1, ssd_value);
}
