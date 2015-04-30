#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"
#include "diag.h"

#define U_ID_0 (*(uint32_t*) 0x1FFF7A10)
#define U_ID_1 (*(uint32_t*) 0x1FFF7A14)
#define U_ID_2 (*(uint32_t*) 0x1FFF7A18)

Anki::ReturnCode Anki::Cozmo::Robot::step_MainExecution()
{
  return 0;
}

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
      
      int UARTGetFreeSpace();
      
      //const CameraInfo* GetHeadCamInfo(){ return 0; }
      
      //Messages::ID RadioGetNextMessage(u8* buffer){ return (Messages::ID)0; }
      //bool RadioIsConnected(){ return false; }
    }
  }
}

void Reboot()
{
  printf("Rebooting processor");
  Anki::Cozmo::HAL::MicroWait(500000); // wait 0.5 seconds
  NVIC_SystemReset();
}

s32 GetCommand()
{
    s32 c;
    printf("  Select option: ");
    do
    {
      c = Anki::Cozmo::HAL::UARTGetChar();
    }
    while(c == -1);
    Anki::Cozmo::HAL::UARTPutChar(c);
    printf("\r\n");
    return c;
}


void AudioMenu()
{
  using namespace Anki::Cozmo::HAL;
  using namespace Anki::Cozmo::DIAG_HAL;
  // Print Menu Options
  printf("\r\nAudio Menu\r\n\r\n");
  
  printf("\t(%d) - Return to main menu\r\n", EscAudio);
  printf("\t(%d) - Play beep\r\n", PlaySineLow);
  printf("\t(%d) - Play frequency\r\n", PlaySineHigh);
  printf("\t(%d) - Play sweep\r\n", PlaySweep);
  printf("\r\n");
  
  while (1)
  {
    char c = GetCommand();
    switch(c - 0x30)
    {
      case EscAudio:
        Reboot();
        break;
         
      case PlaySineLow:
        printf("Playing 200 Hz Tone ... Press any key to continue\r\n");
        AudioInit();    
        do
        {
          SineLow();
        }
        while( UARTGetChar() == -1 );
        break;
             
      case PlaySineHigh:
        printf("Playing 1000 Hz Tone... Press any key to continue\r\n");
        AudioInit();    
        do
        {
          SineHigh();
        }
        while( UARTGetChar() == -1 );
        break;
               
      case PlaySweep:
        printf("Playing sweep... Press any key to continue\r\n"); 
        AudioInit();  
        do
        {
          Sweep();
        }
        while( UARTGetChar() == -1 );
        break;

      default:
        printf("Invalid Option\r\n");
    }
  }
}


void CameraMenu()
{
  using namespace Anki::Cozmo::HAL;
  // Print Menu Options
  printf("\r\n*Not supported yet.\r\n");
}


void IMUMenu()
{
  using namespace Anki::Cozmo::HAL;
  // Print Menu Options
  printf("\r\nIMU Menu\r\n\r\n");
  printf("\t(%d) - Return to main menu\r\n", EscIMU);
  printf("\t(%d) - Stream data\r\n", StreamData);
  printf("\t(%d) - Run drive motors and stream data\r\n", StreamDance);
  printf("\t(%d) - *Calibrate\r\n", Calibrate);
  printf("\r\n");
  
  IMU_DataStructure IMUData;
  
  while (1)
  {
    char c = GetCommand();
    switch(c - 0x30)
    {
      case EscIMU:
        Reboot();
        break;
         
      case StreamData:       
        IMUInit();
        printf("Acceleration X Y Z (mm/s/s), Rate X Y Z (rad/s)\r\n");
        do
        {
          IMUReadData(IMUData);
          printf("%f %f %f %f %f %f\r\n", IMUData.acc_x, IMUData.acc_y, IMUData.acc_z, IMUData.rate_x, IMUData.rate_y, IMUData.rate_z);
          MicroWait(100000); // 10 Hz
        }
        while( UARTGetChar() == -1 );
        break;
        
      case StreamDance:
        IMUInit();
        printf("Acceleration X Y Z (mm/s/s), Rate X Y Z (rad/s)\r\n");
        f32 t = 0.0f;
        do
        {
          if(t < 0.5f)
          {
            MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);
            MotorSetPower(MOTOR_RIGHT_WHEEL, 0.0f);
          }
          else if (t < 1.5f)
          {
            MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);
            MotorSetPower(MOTOR_RIGHT_WHEEL, 0.4f);
          }
          else if (t < 2.0f)
          {
            MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);
            MotorSetPower(MOTOR_RIGHT_WHEEL, 0.0f);
          }
          else if (t < 3.0f)
          {
            MotorSetPower(MOTOR_LEFT_WHEEL, 0.4f);
            MotorSetPower(MOTOR_RIGHT_WHEEL, 0.0f);
          }
          else
          {
            MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);
            MotorSetPower(MOTOR_RIGHT_WHEEL, 0.0f);
          }
          IMUReadData(IMUData);
          printf("%f %f %f %f %f %f\r\n", IMUData.acc_x, IMUData.acc_y, IMUData.acc_z, IMUData.rate_x, IMUData.rate_y, IMUData.rate_z);
          MicroWait(10000); // 100 Hz
          t += 0.01;
        }
        while( UARTGetChar() == -1 );
        break;  
      
      case PrintAverages:
        IMUInit();
        f32 ax,ay,az,gx,gy,gz;
        ax = 0;
        ay = 0;
        az = 0;
        gx = 0;
        gy = 0;
        gz = 0;
        for(int i = 0; i<1000; i++)
        {
          IMUReadData(IMUData);
          ax += IMUData.acc_x;
          ay += IMUData.acc_y;
          az += IMUData.acc_z;
          gx += IMUData.rate_x;
          gy += IMUData.rate_y;
          gz += IMUData.rate_z;
          MicroWait(10000);
        }
        printf("%f %f %f %f %f %f\r\n", 0.001*ax, 0.001*ay, 0.001*az, 0.001*gx, 0.001*gy, 0.001*gz);
        break;

      case Calibrate:
        IMUInit();
        MotorResetPosition(MOTOR_HEAD);
        MotorSetPower(MOTOR_HEAD, 0.5f);
        MicroWait(1000000); 
        MotorResetPosition(MOTOR_HEAD);
        f32 x,y,z;
        x = 0;
        y = 0;
        z = 0;
        printf("%f %f\r\n", MotorGetSpeed(MOTOR_HEAD), MotorGetPosition(MOTOR_HEAD));
        for(int i = 0; i<1000; i++)
        {
          IMUReadData(IMUData);
          x += IMUData.acc_x;
          y += IMUData.acc_y;
          z += IMUData.acc_z;
          MicroWait(10000);
        }
        printf("%f %f %f\r\n", 0.001*x, 0.001*y, 0.001*z);
        MotorSetPower(MOTOR_HEAD, -0.5f);
        MicroWait(1000000); 
        x = 0;
        y = 0;
        z = 0;
        printf("%f %f\r\n", MotorGetSpeed(MOTOR_HEAD), MotorGetPosition(MOTOR_HEAD));
        for(int i = 0; i<1000; i++)
        {
          IMUReadData(IMUData);
          x += IMUData.acc_x;
          y += IMUData.acc_y;
          z += IMUData.acc_z;
          MicroWait(10000);
        }
        printf("%f %f %f\r\n", 0.001*x, 0.001*y, 0.001*z);
        
        MotorSetPower(MOTOR_HEAD, 0.0f);
        
     
        printf("*Not supported yet.\r\n");
        break;

      default:
        printf("Invalid Option\r\n");
    }
  }
}


void LEDMenu()
{
  using namespace Anki::Cozmo::HAL;
  using namespace Anki::Cozmo::DIAG_HAL;
  // Print Menu Options
  printf("\r\nLED Menu\r\n\r\n");
  
  printf("\t(%d) - Return to main menu\r\n", EscLED);
  printf("\t(%d) - Blink eyes\r\n", BlinkEyes);
  printf("\t(%d) - Blink status LED\r\n", BlinkStatus);
  printf("\t(%d) - Blink IR LEDs\r\n", BlinkIR);
  printf("\r\n");
    
  while (1)
  {
    char c = GetCommand();
    switch(c - 0x30)
    {
      case EscLED:
        Reboot();
        break;
                 
      case BlinkEyes:
        printf("Blinking eyes... Press any key to continue\r\n"); 
        LEDInit();
        do
        {
          LEDBlinkEye();
        }
        while( UARTGetChar() == -1 );
        break;
               
      case BlinkStatus:
        printf("Blinking status LED... Press any key to continue\r\n"); 
        LEDInit();
        do
        {
          LEDBlinkStatus();
        }
        while( UARTGetChar() == -1 );
        break;
        
      case BlinkIR:
        printf("Blinking IR LEDs... Press any key to continue\r\n"); 
        LEDInit();
        do
        {
          LEDBlinkIR();
        }
        while( UARTGetChar() == -1 );
        break;
        
      default:
        printf("Invalid Option\r\n");
        break;
    }
  }
}


void MotorMenu()
{
  using namespace Anki::Cozmo::HAL;
  // Print Menu Options
  printf("\r\nMotor Menu\r\n\r\n");
  
  printf("\t(%d) - Return to main menu\r\n", EscMotors);
  printf("\t(%d) - Left drive motor forward\r\n", LeftForward);
  printf("\t(%d) - Left drive motor reverse\r\n", LeftReverse);
  printf("\t(%d) - Right drive motor forward\r\n", RightForward);
  printf("\t(%d) - Right drive motor reverse\r\n", RightReverse);
  printf("\t(%d) - Head motor up\r\n", HeadUp);
  printf("\t(%d) - Head motor down\r\n", HeadDown);
  printf("\t(%d) - Lift motor up\r\n", LiftUp);
  printf("\t(%d) - Lift motor down\r\n", LiftDown);
  printf("\r\n");
    
  while (1)
  {
    char c = GetCommand();
    switch(c - 0x30)
    {
      case EscMotors:
        Reboot();
        break;
                 
      case LeftForward:
        printf("Left drive motor forward... Press any key to continue\r\n");
        printf("Speed / Position\r\n");
        MotorSetPower(MOTOR_LEFT_WHEEL, 1.0f);
        MotorResetPosition(MOTOR_LEFT_WHEEL);
        while( UARTGetChar() == -1 )
        {
          printf("%f %f\r\n", MotorGetSpeed(MOTOR_LEFT_WHEEL), MotorGetPosition(MOTOR_LEFT_WHEEL));
          MicroWait(100000);
        }
        MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);
        break;
               
      case LeftReverse:
        printf("Left drive motor reverse... Press any key to continue\r\n");
        printf("Speed / Position\r\n");        
        MotorSetPower(MOTOR_LEFT_WHEEL, -1.0f);
        MotorResetPosition(MOTOR_LEFT_WHEEL);
        while( UARTGetChar() == -1 )
        {
          printf("%f %f\r\n", MotorGetSpeed(MOTOR_LEFT_WHEEL), MotorGetPosition(MOTOR_LEFT_WHEEL));
          MicroWait(100000);
        }
        MotorSetPower(MOTOR_LEFT_WHEEL, 0.0f);
        break;
                         
      case RightForward:
        printf("Right drive motor forward... Press any key to continue\r\n"); 
        printf("Speed / Position\r\n");
        MotorSetPower(MOTOR_RIGHT_WHEEL, 1.0f);
        MotorResetPosition(MOTOR_RIGHT_WHEEL);
        while( UARTGetChar() == -1 )
        {
          printf("%f %f\r\n", MotorGetSpeed(MOTOR_RIGHT_WHEEL), MotorGetPosition(MOTOR_RIGHT_WHEEL));
          MicroWait(100000);
        }
        MotorSetPower(MOTOR_RIGHT_WHEEL, 0.0f);
        break;
               
      case RightReverse:
        printf("Right drive motor reverse... Press any key to continue\r\n");
        printf("Speed / Position\r\n");        
        MotorResetPosition(MOTOR_RIGHT_WHEEL);
        MotorSetPower(MOTOR_RIGHT_WHEEL, -1.0f);
        while( UARTGetChar() == -1 )
        {
          printf("%f %f\r\n", MotorGetSpeed(MOTOR_RIGHT_WHEEL), MotorGetPosition(MOTOR_RIGHT_WHEEL));
          MicroWait(100000);
        }
        MotorSetPower(MOTOR_RIGHT_WHEEL, 0.0f);
        break;
        
      case HeadUp:
        printf("Head motor up... Press any key to continue\r\n"); 
        printf("Speed / Position\r\n");
        MotorResetPosition(MOTOR_HEAD);
        MotorSetPower(MOTOR_HEAD, 1.0f);
        while( UARTGetChar() == -1 )
        {
          printf("%f %f\r\n", MotorGetSpeed(MOTOR_HEAD), MotorGetPosition(MOTOR_HEAD));
          MicroWait(100000);
        }
        MotorSetPower(MOTOR_HEAD, 0.0f);
        break;
               
      case HeadDown:
        printf("Head motor down... Press any key to continue\r\n"); 
        printf("Speed / Position\r\n");
        MotorResetPosition(MOTOR_HEAD);
        MotorSetPower(MOTOR_HEAD, -1.0f);
        while( UARTGetChar() == -1 )
        {
          printf("%f %f\r\n", MotorGetSpeed(MOTOR_HEAD), MotorGetPosition(MOTOR_HEAD));
          MicroWait(100000);
        }
        MotorSetPower(MOTOR_HEAD, 0.0f);
        break;
                         
      case LiftUp:
        printf("Lift motor up... Press any key to continue\r\n"); 
        printf("Speed / Position\r\n");
        MotorResetPosition(MOTOR_LIFT);
        MotorSetPower(MOTOR_LIFT, 1.0f);
        while( UARTGetChar() == -1 )
        {
          printf("%f %f\r\n", MotorGetSpeed(MOTOR_LIFT), MotorGetPosition(MOTOR_LIFT));
          MicroWait(100000);
        }
        MotorSetPower(MOTOR_LIFT, 0.0f);
        break;
               
      case LiftDown:
        printf("Lift motor down... Press any key to continue\r\n"); 
        printf("Speed / Position\r\n");
        MotorResetPosition(MOTOR_LIFT);
        MotorSetPower(MOTOR_LIFT, -1.0f);
        while( UARTGetChar() == -1 )
        {
          printf("%f %f\r\n", MotorGetSpeed(MOTOR_LIFT), MotorGetPosition(MOTOR_LIFT));
          MicroWait(100000);
        }
        MotorSetPower(MOTOR_LIFT, 0.0f);
        break;
        
      case RobotDance:
        printf("Oscillating all motors... Press any key to continue\r\n");
        while( UARTGetChar() == -1 )
        {
          MotorSetPower(MOTOR_LEFT_WHEEL, 1.0f);
          MotorSetPower(MOTOR_RIGHT_WHEEL, 1.0f);
          MotorSetPower(MOTOR_HEAD, 1.0f);
          MotorSetPower(MOTOR_LIFT, 1.0f);
          MicroWait(1000000);
          MotorSetPower(MOTOR_LEFT_WHEEL, -1.0f);
          MotorSetPower(MOTOR_RIGHT_WHEEL, -1.0f);
          MotorSetPower(MOTOR_HEAD, -1.0f);
          MotorSetPower(MOTOR_LIFT, -1.0f);
          MicroWait(1000000);
        }
        MotorSetPower(MOTOR_LIFT, 0.0f);
        break;
        
      default:
        printf("Invalid Option\r\n");
        break;
    }
  }
}

void PowerMenu()
{
  using namespace Anki::Cozmo::HAL;
  // Print Menu Options
  printf("\r\n*Not supported yet.\r\n");
}

void SelfTestMenu()
{
  using namespace Anki::Cozmo::HAL;
  // Print Menu Options
  printf("\r\n*Not supported yet.\r\n");
}

void MainMenu()
{
  using namespace Anki::Cozmo::HAL;
  // Print Menu Options
  printf("\r\nMain Menu - Please select from the following options:\r\n\r\n");

  printf("\t(%d) - Reboot\r\n", Esc);
  printf("\t(%d) - Audio\r\n", Audio);
  printf("\t(%d) - *Camera\r\n", Camera);
  printf("\t(%d) - IMU\r\n", IMU);
  printf("\t(%d) - LEDs\r\n", LEDs);
  printf("\t(%d) - Motors\r\n", Motors);
  printf("\t(%d) - *Power\r\n", Power);
  printf("\t(%d) - *Self Test\r\n", SelfTest);
  printf("\r\n");
  
  while(1)
  {
    char c = GetCommand();
    switch(c - 0x30)
    {
      case Esc:
        Reboot();
        break;
        
      case Audio:
        AudioMenu();
        break;
         
      case Camera:
        CameraMenu();
        break;
             
      case IMU:
        IMUMenu();
        break;
               
      case LEDs:
        LEDMenu();
        break;
               
      case Motors:
        MotorMenu();
        break;
        
      case Power:
        PowerMenu();
        break;
        
      case SelfTest:
        SelfTestMenu();
        break;
        
      default:
        printf("Invalid Option\r\n");
    }
  }
}

int main(void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Initialize the hardware
  Startup();
  TimerInit();
  UARTInit();
  SPIInit();
  
  printf("\f");
  printf("################################################################################\r\n\r\n");
  printf("Welcome to Cozmo Diagnostic Tool!\r\n");
  printf("ID: %x-%x-%x\r\n\r\n", U_ID_2, U_ID_1, U_ID_0);
  printf("################################################################################\r\n");
  // Run Menu
  while(1)
  {
    MainMenu();
  }
  return 0;

  s32 c;
  while(1)
  {
    c=UARTGetChar(1000);
    UARTPutChar(c);
    UARTPutChar(c);
  }
  
  //AudioInit();
  //MakeNoise();
  
  return 0;
  
  FrontCameraInit();
  
  SPIInit();
  UARTPutString("SPI!\r\n");

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