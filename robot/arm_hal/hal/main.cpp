#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "spiData.h"

//#define SEND_IMAGE_ONLY_TEST_BASESTATION

//#define DO_MOTOR_TESTING

//#define DO_BAUDRATE_TESTING

//#define DO_LED_TESTING

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
      void SetLED(LEDId led_id, u32 color);
      void SharpInit();

      void PrintCrap();

      //TimeStamp_t GetTimeStamp(void){ return (TimeStamp_t)0; }
      TimeStamp_t t_;
      TimeStamp_t GetTimeStamp(void){ return t_; }
      void SetTimeStamp(TimeStamp_t t) {t_ = t;}

      int UARTGetFreeSpace();

      const CameraInfo* GetHeadCamInfo(void)
      {
        static CameraInfo s_headCamInfo = {
          HEAD_CAM_CALIB_FOCAL_LENGTH_X,
          HEAD_CAM_CALIB_FOCAL_LENGTH_Y,
          HEAD_CAM_CALIB_CENTER_X,
          HEAD_CAM_CALIB_CENTER_Y,
          0.f,
          HEAD_CAM_CALIB_HEIGHT,
          HEAD_CAM_CALIB_WIDTH
        };

        return &s_headCamInfo;
      }

      // XXX: This needs to work in a new way with 3.0+
      bool WifiHasClient() {
        return false;
      }
      // XXX: This needs to work in a new way with 3.0+
      void GetProximity(ProximityValues *prox) { }

      //Messages::ID RadioGetNextMessage(u8* buffer){ return (Messages::ID)0; }
      //bool RadioIsConnected(){ return false; }
      static IDCard m_idCard;
      IDCard* GetIDCard() { return &m_idCard; }

      static void GetId()
      {
        // XXX: Replace with flash identification block
        printf("My ID: %08x", *(int*)(0x1FFF7A10));
        //m_idCard.esn = 2;
        //if (*(int*)(0x1FFF7A10) == 0x00250031)
        m_idCard.esn = 1;
      }
    }
  }
}

#ifdef DO_MOTOR_TESTING
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
#endif

#if defined(DO_MEM_TEST)
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
#endif

int main(void)
{
  using namespace Anki::Cozmo::HAL;
  using namespace Anki::Cozmo;

  // Timer, than Startup, must be called FIRST in main() to do hardware sanity check
  TimerInit();
  Startup();

  // Initialize the hardware
  //LightsInit();
  UARTInit();
  printf("UART..");
  GetId();

#if defined(DO_BAUDRATE_TESTING)
  while(1) UARTPutChar(0xaa);
#endif

  IMUInit();  // The IMU must be configured before spineport
  printf("IMU..");
  SPIInit();
  printf("spine..");

#if defined(DO_PROX_SENSOR_TESTING)
  // Prox sensor testing
  ProximityValues prox;
  while(1)
  {
    GetProximity(&prox);
    MicroWait(5000);
  }
#elif defined(DO_MOTOR_TESTING)
  // Motor testing...
  while (1)
  {
    MotorSetPower(MOTOR_LEFT_WHEEL, 0.3f);
    Wait();
    MotorSetPower(MOTOR_LEFT_WHEEL, -0.3f);
    Wait();
    MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);

    MotorSetPower(MOTOR_RIGHT_WHEEL, 0.3f);
    Wait();
    MotorSetPower(MOTOR_RIGHT_WHEEL, -0.3f);
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

#elif defined(DO_LED_TESTING)
  while (1) {
    for (int i=0; i<8; ++i) {
      SetLED((LEDId)i, LED_RED);
      MicroWait(250000);
      SetLED((LEDId)i, 0);
    }
    for (int i=0; i<8; ++i) {
      SetLED((LEDId)i, LED_GREEN);
      MicroWait(250000);
      SetLED((LEDId)i, 0);
    }
    for (int i=0; i<8; ++i) {
      SetLED((LEDId)i, LED_BLUE);
      MicroWait(250000);
      SetLED((LEDId)i, 0);
    }
  }

#else

  Anki::Cozmo::Robot::Init();
  g_halInitComplete = true;
  printf("init complete!\r\n");

  while (1) // XXX: Anki::Cozmo::Robot::step_LongExecution() == Anki::RESULT_OK)
  {
  }
#endif
}

extern "C"
void __aeabi_assert(const char* s1, const char* s2, int s3)
{
}
