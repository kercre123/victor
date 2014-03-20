#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "spiData.h"

//OFFCHIP u8 buffer[320*240];

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
      void FrontCameraInit();
      
      int USBGetChar(u32){ return -1; }
      s32 USBPeekChar(u32 offset){ return -1; }
      u32 USBGetNumBytesToRead(){ return 0; }
      int USBPutChar(int c){ return c; }
      
      TimeStamp_t GetTimeStamp(void){ return (TimeStamp_t)0; }
      
      int GetRobotID(){ return 0; }
      void UpdateDisplay(){ }
      //bool RadioSendMessage(Anki::Cozmo::Messages::ID, const void*, u32){ return true; }
      
      ReturnCode Init(){ return 0; }
      ReturnCode Step(){ return 0; }
      void Destroy(){ }
      
      //const CameraInfo* GetHeadCamInfo(){ return 0; }
      
      //Messages::ID RadioGetNextMessage(u8* buffer){ return (Messages::ID)0; }
      //bool RadioIsConnected(){ return false; }
    }
  }
}

void Wait()
{
  using namespace Anki::Cozmo::HAL;
  
  u32 start = GetMicroCounter();
  while ((GetMicroCounter() - start) < 500000)
  {
    /*printf("%.6f, %.6f  | %.6f, %.6f\n",
      MotorGetPosition(MOTOR_LEFT_WHEEL),
      MotorGetSpeed(MOTOR_LEFT_WHEEL),
      MotorGetPosition(MOTOR_RIGHT_WHEEL),
      MotorGetSpeed(MOTOR_RIGHT_WHEEL));*/
  }
}

int main(void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Initialize the hardware
  Startup();
  TimerInit();
  UARTInit();
  
  UARTPutString("UART!\r\n");
  
  FrontCameraInit();
  
  SPIInit();
  UARTPutString("SPI!\r\n");
  
#if 0
  // Motor testing...
  while (1)
  {
    MotorSetPower(MOTOR_LEFT_WHEEL, 0.6f);
    Wait();
    MotorSetPower(MOTOR_LEFT_WHEEL, -0.6f);
    Wait();
    MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);
    
    MotorSetPower(MOTOR_RIGHT_WHEEL, 0.6f);
    Wait();
    MotorSetPower(MOTOR_RIGHT_WHEEL, -0.6f);
    Wait();
    MotorSetPower(MOTOR_RIGHT_WHEEL, 0.0f);
    
    /*MotorSetPower(MOTOR_LIFT, 0.3f);
    Wait();
    MotorSetPower(MOTOR_LIFT, -0.3f);
    Wait();
    MotorSetPower(MOTOR_LIFT, 0.0f);
    
    /*MotorSetPower(MOTOR_HEAD, 0.3f);
    Wait();
    MotorSetPower(MOTOR_HEAD, -0.3f);
    Wait();
    MotorSetPower(MOTOR_HEAD, -0.0f);
    
    MicroWait(500000); */
  }
  
#else
  
  Anki::Cozmo::Robot::Init();
  
  while (Anki::Cozmo::Robot::step_LongExecution() == EXIT_SUCCESS)
  //while(true)
  {
    /*
    CameraGetFrame(buffer, CAMERA_MODE_QVGA, 0.25f, false);
    
    UARTPutChar(0xbe);
    UARTPutChar(0xef);
    UARTPutChar(0xf0);
    UARTPutChar(0xff);
    UARTPutChar(0xbd);
    
    // Revert to the old method if the  buffer is full
    //if (!UARTPutBuffer(buffer, 320 * 240))
    {
      for (int y = 0; y < 240; y++)
      {
        for (int x = 0; x < 320; x++)
        {
          UARTPutChar(buffer[y * 320 + x]);
        }
      }
    }
   */ 
  }
#endif
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

