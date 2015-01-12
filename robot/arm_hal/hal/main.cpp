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

  // Disable for now
  //UARTInit();
  //printf("UART..");
  GetId();

  IMUInit();  // The IMU must be configured before spineport
  printf("IMU..");

  // Disable for now
  //SPIInit();
  //printf("spine..");

  Anki::Cozmo::Robot::Init();
  g_halInitComplete = true;
  printf("init complete!\r\n");

  while (true); // No long execution in cozmo 3
  {
    // Add main loop here...
  }
}

extern "C"
void __aeabi_assert(const char* s1, const char* s2, int s3)
{
}
