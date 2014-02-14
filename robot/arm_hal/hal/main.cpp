#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"

extern "C" {
#include "lib/usb/usbd_cdc_core.h"
#include "lib/usb/usbd_usr.h"
#include "lib/usb/usb_conf.h"
#include "lib/usb/usbd_desc.h"
}

 __ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev __ALIGN_END ;

extern u8 m_buffer1[];

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Forward declarations
      void Startup();
      void TimerInit();
      void UARTInit();
      
      int USBGetChar(u32){ return -1; }
      s32 USBPeekChar(u32 offset){ return -1; }
      u32 USBGetNumBytesToRead(){ return 0; }
      int USBPutChar(int c){ return c; }
      
      void CameraStartFrame(CameraID cameraID, u8* frame, CameraMode mode,
          CameraUpdateMode updateMode, u16 exposure, bool enableLight)
      {
      }
      
      bool CameraIsEndOfFrame(CameraID cameraID){ return false; }
      
			#warning fix this
      //TimeStamp GetTimeStamp(void){ return (TimeStamp)0; }
      
      int GetRobotID(){ return 0; }
      void UpdateDisplay(){ }
      bool RadioSendMessage(Anki::Cozmo::Messages::ID, const void*){ return true; }
      
      ReturnCode Init(){ return 0; }
      ReturnCode Step(){ return 0; }
      void Destroy(){ }
      
      void MotorSetPower(MotorID motor, f32 power){ }
      void MotorResetPosition(MotorID motor){ }
      f32 MotorGetSpeed(MotorID motor){ return 0; }
      f32 MotorGetPosition(MotorID motor){ return 0; }
      
      const CameraInfo* GetHeadCamInfo(){ return 0; }
      const CameraInfo* GetMatCamInfo(){ return 0; }
      
      Messages::ID RadioGetNextMessage(u8* buffer){ return (Messages::ID)0; }
    }
  }
}

extern bool isEOF;
extern void StartFrame();

#define DATA_SIZE 100
u8 g_dataTX[DATA_SIZE];
volatile u8 g_dataRX[DATA_SIZE];


int main(void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Initialize the hardware
  Startup();
  TimerInit();
  UARTInit();
  
  UARTPutString("UART Initialized\r\n");
  
  USBD_Init(&USB_OTG_dev,
            USB_OTG_FS_CORE_ID,
            &USR_desc, 
            &USBD_CDC_cb, 
            &USR_cb);
  
  //volatile u8* ram = (volatile u8*)0xC0000000;
  
  UARTPutString("Initializing Robot\r\n");
  Anki::Cozmo::Robot::Init();
  
  while (1)
  {
    Anki::Cozmo::Robot::step_LongExecution();
  }
  
  /*GPIO_InitTypeDef gpio;
  
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
  
  gpio.GPIO_Pin = GPIO_Pin_13;
  gpio.GPIO_Mode = GPIO_Mode_OUT;
  gpio.GPIO_Speed = GPIO_Speed_2MHz;
  gpio.GPIO_OType = GPIO_OType_PP;
  gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
  
  GPIO_Init(GPIOG, &gpio); */
  
  /*FrontCameraInit();
  
  u32 startTime = GetMicroCounter();
  while (1)
  {
    //while (!isEOF) ;
    
    //if (isEOF)
    {
      UARTPutChar(0xbe);
      UARTPutChar(0xef);
      UARTPutChar(0xf0);
      UARTPutChar(0xff);
      UARTPutChar(0xbd);
      
      for (int y = 0; y < 240; y++)
      {
        for (int x = 0; x < 320; x++)
        {
          UARTPutChar(m_buffer1[y * 320*2 + x*2]);
        }
      }
      
      StartFrame();
      //MicroWait(250000);
    }
    
    //MicroWait(2000);
  }*/
  
  
  // Initialize some SPI data
  /*for (int i = 0; i < DATA_SIZE; i++)
  {
    g_dataTX[i] = 0xFF - i;
    g_dataRX[i] = 0;
  }
  
  GPIO_PIN_SOURCE(SPI_SCK, GPIOG, 13);
  GPIO_PIN_SOURCE(SPI_MISO, GPIOG, 12);
  GPIO_PIN_SOURCE(SPI_MOSI, GPIOG, 14);

  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI6, ENABLE);
  
  // Configure the pins for SPI in AF mode
  GPIO_PinAFConfig(GPIO_SPI_SCK, SOURCE_SPI_SCK, GPIO_AF_SPI6);
  GPIO_PinAFConfig(GPIO_SPI_MISO, SOURCE_SPI_MISO, GPIO_AF_SPI6);
  GPIO_PinAFConfig(GPIO_SPI_MOSI, SOURCE_SPI_MOSI, GPIO_AF_SPI6);
  
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = PIN_SPI_SCK | PIN_SPI_MISO | PIN_SPI_MOSI;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
  GPIO_Init(GPIO_SPI_SCK, &GPIO_InitStructure);  // GPIOG
  
  // Initialize SPI in slave mode
  SPI_I2S_DeInit(SPI6);
  SPI_InitTypeDef SPI_InitStructure;
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI6, &SPI_InitStructure);
  
  /*SPI_Cmd(SPI6, ENABLE);
  
  while (1)
  {
    while (!(SPI6->SR & SPI_FLAG_TXE))
      ;
    SPI6->DR = 0xAA;
    
    SPI6->DR;
  } */
  
  // Initialize DMA for SPI
  // SPI6_TX is DMA2_Stream5 Channel 1
  // SPI6_RX is DMA2_Stream6 Channel 1
  /*DMA_DeInit(DMA2_Stream5);
  DMA_DeInit(DMA2_Stream6);
  
  DMA_InitTypeDef DMA_InitStructure;
  DMA_InitStructure.DMA_BufferSize = DATA_SIZE;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&(SPI6->DR);
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  // Configure TX DMA
  DMA_InitStructure.DMA_Channel = DMA_Channel_1;
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
  DMA_InitStructure.DMA_Memory0BaseAddr = (u32)g_dataTX;
  DMA_Init(DMA2_Stream5, &DMA_InitStructure);
  // Configure RX DMA
  DMA_InitStructure.DMA_Channel = DMA_Channel_1;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_Memory0BaseAddr = (u32)g_dataRX;
  DMA_Init(DMA2_Stream6, &DMA_InitStructure);
  
  // Enable DMA
  DMA_Cmd(DMA2_Stream5, ENABLE);
  DMA_Cmd(DMA2_Stream6, ENABLE);
  
  // Enable DMA SPI requests
  SPI_I2S_DMACmd(SPI6, SPI_I2S_DMAReq_Tx, ENABLE);
  SPI_I2S_DMACmd(SPI6, SPI_I2S_DMAReq_Rx, ENABLE);
  
  SPI_Cmd(SPI6, ENABLE);
  
  // Wait for transfers to complete
  while (!DMA_GetFlagStatus(DMA2_Stream5, DMA_FLAG_TCIF5))
    ;
  while (!DMA_GetFlagStatus(DMA2_Stream6, DMA_FLAG_TCIF6))
    ;
  
  // Clear DMA Transfer Complete Flags
  DMA_ClearFlag(DMA2_Stream5, DMA_FLAG_TCIF5);
  DMA_ClearFlag(DMA2_Stream6, DMA_FLAG_TCIF6);
  
  while (1)
    ;*/
}

extern "C"
u32 XXX_HACK_FOR_PETE()
{
  return Anki::Cozmo::HAL::GetMicroCounter();
}

extern "C"
void __aeabi_assert(const char* s1, const char* s2, int s3)
{
}

