#include <xgpio.h>

typedef struct SSD {
    XGpio GPIO_inst;
    u32 GPIO_addr;
 } SSD;

 void SSD_begin(SSD *InstancePtr, u32 GPIO_Address);
 void SSD_setSSD(SSD *InstancePtr, u8 ssd_value);