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
      void SPIInit();
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
      void CameraSetIsEndOfFrame(CameraID, bool){ }
      
      typedef u32 TimeStamp;
      TimeStamp GetTimeStamp(void){ return (TimeStamp)0; }
      
      int GetRobotID(){ return 0; }
      void UpdateDisplay(){ }
      bool RadioSendMessage(Anki::Cozmo::Messages::ID, const void*, u32){ return true; }
      
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

int main(void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Initialize the hardware
  Startup();
  TimerInit();
  UARTInit();
  
  UARTPutString("UART Initialized\r\n");
  
  SPIInit();
  
  UARTPutString("Initializing Robot\r\n");
  Anki::Cozmo::Robot::Init();
  
  while (1)
  {
    Anki::Cozmo::Robot::step_LongExecution();
  }
  
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

