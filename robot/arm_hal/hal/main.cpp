#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"

extern u8 m_buffer1[];

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      extern GlobalDataToHead m_dataToHead;
      
      // Forward declarations
      void Startup();
      void SPIInit();
      void TimerInit();
      void UARTInit();
      
      int USBGetChar(u32){ return -1; }
      s32 USBPeekChar(u32 offset){ return -1; }
      u32 USBGetNumBytesToRead(){ return 0; }
      int USBPutChar(int c){ return c; }
      void USBSendBuffer(const u8* buffer, const u32 size){ }
      
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
      
      const CameraInfo* GetHeadCamInfo(){ return 0; }
      const CameraInfo* GetMatCamInfo(){ return 0; }
      
      Messages::ID RadioGetNextMessage(u8* buffer){ return (Messages::ID)0; }
      bool RadioIsConnected(){ return false; }
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
  
  UARTPutString("UART!\r\n");
  
  SPIInit();
  
  UARTPutString("SPI!\r\n");
  
  FrontCameraInit();
  
#if 0
  // Motor testing...
  while (1)
  {
    MotorSetPower(MOTOR_LEFT_WHEEL, 0.3f);
    MicroWait(500000);
    MotorSetPower(MOTOR_LEFT_WHEEL, -0.3f);
    MicroWait(500000);
    MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);
    
    MotorSetPower(MOTOR_RIGHT_WHEEL, 0.3f);
    MicroWait(500000);
    MotorSetPower(MOTOR_RIGHT_WHEEL, -0.3f);
    MicroWait(500000);
    MotorSetPower(MOTOR_RIGHT_WHEEL, 0.0f);
    
    MotorSetPower(MOTOR_LIFT, 0.3f);
    MicroWait(500000);
    MotorSetPower(MOTOR_LIFT, -0.3f);
    MicroWait(500000);
    MotorSetPower(MOTOR_LIFT, 0.0f);
    
    MotorSetPower(MOTOR_HEAD, 0.3f);
    MicroWait(500000);
    MotorSetPower(MOTOR_HEAD, -0.3f);
    MicroWait(500000);
    MotorSetPower(MOTOR_HEAD, -0.0f);
    MicroWait(500000);
  }
  
#else
  
  Anki::Cozmo::Robot::Init();
  
  while (Anki::Cozmo::Robot::step_LongExecution() == EXIT_SUCCESS)
  {
  }
  
#endif
  
  
  /*u32 startTime = GetMicroCounter();
  
  for (int i = 0; i < 320*240; i++)
    m_buffer1[i] = 0;
  
  while (1)
  {
    //while (!isEOF) ;
    
    if (isEOF)
    {
      UARTPutChar(0xbe);
      UARTPutChar(0xef);
      UARTPutChar(0xf0);
      UARTPutChar(0xff);
      UARTPutChar(0xbd);
      
      for (int y = 0; y < 240; y += 1)
      {
        for (int x = 0; x < 320; x += 1)
        {
          UARTPutChar(m_buffer1[y * 320*2 + x*2]);
        }
      }
      
      StartFrame();
    }
  } */
  
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

