#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "spiData.h"

//#define SEND_IMAGE_ONLY_TEST_BASESTATION

#ifdef SEND_IMAGE_ONLY_TEST_BASESTATION
OFFCHIP u8 buffer[320*240 + 5]; // +5 for beeffoodfd
#endif

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      extern u8 g_halInitComplete;
      
      // Forward declarations
      void Startup();
      void SPIInit();
      void TimerInit();
      void UARTInit();
      void FrontCameraInit();
      void IMUInit();
      void LightsInit();
      void SharpInit();

      void PrintCrap();

      //TimeStamp_t GetTimeStamp(void){ return (TimeStamp_t)0; }
      TimeStamp_t t_;
      TimeStamp_t GetTimeStamp(void){ return t_; }
      void SetTimeStamp(TimeStamp_t t) {t_ = t;}

      int UARTGetFreeSpace();
      
      //const CameraInfo* GetHeadCamInfo(){ return 0; }
      
      //Messages::ID RadioGetNextMessage(u8* buffer){ return (Messages::ID)0; }
      //bool RadioIsConnected(){ return false; }
      static IDCard m_idCard;
      IDCard* GetIDCard() { return &m_idCard; }

      static void GetId()
      {
        // XXX: Replace with flash identification block
        printf("My ID: %08x", *(int*)(0x1FFF7A10));
        m_idCard.esn = 2;
        if (*(int*)(0x1FFF7A10) == 0x00250031)
          m_idCard.esn = 1;
      }
    }
  }
}

// Belongs in motortest.cpp
static void Wait()
{
  using namespace Anki::Cozmo::HAL;
  
  u32 start = GetMicroCounter();
  while ((GetMicroCounter() - start) < 500000)
  {}
  printf("\n");
  for (int i = 0; i < 4; i++)
  {
    printf("%.6f, %.6f | ",
      MotorGetPosition((MotorID)i),
      MotorGetSpeed((MotorID)i));
  }
  printf("\n");
  PrintCrap();
}

// Belongs in powerontest.cpp
static void MemTest()
{
  using namespace Anki::Cozmo::HAL;
  // Memory test  
  UARTPutString("Testing 64MB...");
  MicroWait(1000);
  for (int* pp = (int*)0xC0000000; pp < (int*)0xC4000000; pp++)
    *pp = ((int)pp)*11917;
  for (int* pp = (int*)0xC0000000; pp < (int*)0xC4000000; pp++)
    if (*pp != ((int)pp)*11917)
      UARTPutString("error");  
  UARTPutString("Done\r\n");
}

int main(void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Timer, than Startup, must be called FIRST in main() to do hardware sanity check
  TimerInit();
  Startup();
  
  // Initialize the hardware
  LightsInit();
  UARTInit();
  printf("UART..");
  GetId();
  
  FrontCameraInit();
  printf("camera..");
  
  IMUInit();  // The IMU must be configured before spineport  
  printf("IMU..");
  SPIInit();  
  printf("spine..");
  
  SharpInit();
  printf("sharp..");
  
#if 0 
  // Prox sensor testing
  ProximityValues prox;
  while(1)
  {
    GetProximity(&prox);
    MicroWait(5000);
  }
#endif
  
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
    
    MotorSetPower(MOTOR_LIFT, 0.6f);
    Wait();
    MotorSetPower(MOTOR_LIFT, -0.6f);
    Wait();
    MotorSetPower(MOTOR_LIFT, 0.0f);

    MotorSetPower(MOTOR_HEAD, 0.5f);
    Wait();
    MotorSetPower(MOTOR_HEAD, -0.5f);
    Wait();
    MotorSetPower(MOTOR_HEAD, -0.0f);

    MicroWait(500000);
  }
  
#else
  
#ifndef SEND_IMAGE_ONLY_TEST_BASESTATION
  Anki::Cozmo::Robot::Init();
  g_halInitComplete = true;
  printf("init complete!\r\n");
   
  while (Anki::Cozmo::Robot::step_LongExecution() == Anki::RESULT_OK)
  {
  }
#else
  while(true)
  {
    CameraGetFrame(buffer, Anki::Vision::CAMERA_RES_QVGA, false);
    
    if (UARTGetFreeSpace() < (1024 * 1024 * 4) - (320*240+5))
      continue;
    
    UARTPutChar(0xbe);
    UARTPutChar(0xef);
    UARTPutChar(0xf0);
    UARTPutChar(0xff);
    UARTPutChar(0xbd);
    
    for (int y = 0; y < 240; y++)
    {
      for (int x = 0; x < 320; x++)
      {
        //buffer[y*320 + x ] = (buffer[y*320 + x] * ((x & 255) ^ y)) >> 8;
        UARTPutChar(buffer[y*320 + x]); // + buffer[y*320 + x+320] + buffer[y*320 + x+1] + buffer[y*320 + x+321])/4);
      }
    }
  }
#endif // #ifdef SEND_IMAGE_ONLY_TEST_BASESTATION  
#endif
}

extern "C"
void __aeabi_assert(const char* s1, const char* s2, int s3)
{
}

