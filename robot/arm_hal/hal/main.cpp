#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "spiData.h"

//#define SEND_IMAGE_ONLY

#ifdef SEND_IMAGE_ONLY
OFFCHIP u8 buffer[320*240 + 5]; // +5 for beeffoodfd
#endif

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
      void IMUInit();

      TimeStamp_t GetTimeStamp(void){ return (TimeStamp_t)0; }

      int UARTGetFreeSpace();
      
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
  
  IMUInit();
	
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
    
    MotorSetPower(MOTOR_HEAD, 0.3f);
    Wait();
    MotorSetPower(MOTOR_HEAD, -0.3f);
    Wait();
    MotorSetPower(MOTOR_HEAD, -0.0f);
    
    MicroWait(500000); */
  }
  
#else
  
  Anki::Cozmo::Robot::Init();
  
#ifndef SEND_IMAGE_ONLY
  while (Anki::Cozmo::Robot::step_LongExecution() == Anki::RESULT_OK)
  {
  }
#else
  while(true)
  {
    buffer[0] = 0xbe;
    buffer[1] = 0xef;
    buffer[2] = 0xf0;
    buffer[3] = 0xff;
    buffer[4] = 0xbd;
    
    CameraGetFrame(&buffer[5], CAMERA_MODE_QVGA, 0.25f, false);
    
    UARTPutChar(buffer[0]);
    UARTPutChar(buffer[1]);
    UARTPutChar(buffer[2]);
    UARTPutChar(buffer[3]);
    UARTPutChar(buffer[4]);
    
    for (int y = 0; y < 240; y++)
    {
      for (int x = 0; x < 320; x++)
      {
        buffer[y*320 + x + 5] = (buffer[y*320 + x + 5] * ((x & 255) ^ y)) >> 8;
        UARTPutChar((x & 255) ^ y);
      }
    }
    
    //MicroWait(2000000);
    
    
    // 
    /*if (!UARTPutBuffer(buffer, 320 * 240 + 5))
    {
    }*/
  }
#endif // #ifdef SEND_IMAGE_ONLY
  
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

