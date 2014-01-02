#include "bootSlaveI2C.h"
#include "DrvSvu.h"
#include "mv_types.h"
#include "stdio.h"
#include "DrvI2cMaster.h"
#include <DrvGpioDefines.h>
#include "DrvCpr.h"
#include <brdMv0153Defines.h>
#include <brdMv0155DCard.h>
#include <DrvTimer.h>


int PowerUp156()
{
	int result;

	// Use same driver as for MV0155
    result = brdMv0155DcStartupVoltages(); 
	
	// if result is 0, power-up worked
	return result;
}


void BootMv156(unsigned char* mvcmd_img, unsigned int len)
{
   unsigned int i, idx;
   u64 enableClocks;

   enableClocks = DEV_ICB  |
                  DEV_GPIO |
                  DEV_TIM  |
                  DEV_APB_CTRL | DEV_IIC1;

   //Switch the I2C1 pins into I2C mode 0
   DrvGpioMode(MV0153_I2C1_SCL_PIN, D_GPIO_MODE_0 );
   DrvGpioMode(MV0153_I2C1_SDA_PIN, D_GPIO_MODE_0 );

   DrvCprSysDeviceAction(ENABLE_CLKS,enableClocks);
   
   DrvIicEn            (IIC1_BASE_ADR, 0);          // disable module
   DrvIicSetTar       (IIC1_BASE_ADR, 0x63);       // set target address
   DrvIicCon           (IIC1_BASE_ADR, 1, 0, 2, 1); // configure as master, do not use 10 bit adr, fast speed, restart enable
   DrvIicSetSpeedFast(IIC1_BASE_ADR, 0xB8, 0xB8); // set speed registers
   DrvIicEn            (IIC1_BASE_ADR, 1);          // enable module
   SleepMs(1000);
   
   //Sending Image
   for (i=0 ; i < len; ++i)
   {
    //Wait while FIFO full
     while (( i > 9 )&& !(GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL));
    //Put the new data on FIFO
     idx = (i & 0xFFFFFFFC) | ((i & 0x3)^0x3);
     SET_REG_WORD(IIC1_BASE_ADR + IIC_DATA_CMD, mvcmd_img[idx] & 0xFF);     
   }
  //wait till FIFO empties
   while (!(GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_STATUS) & IIC_STATUS_TX_FIFO_EMPTY)) ;
  //wait for the master to go idle
   while ( (GET_REG_WORD_VAL(IIC1_BASE_ADR + IIC_STATUS) & IIC_STATUS_ACTIVITY_MASTER)) ;
}
