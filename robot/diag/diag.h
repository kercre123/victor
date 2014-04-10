enum MainMenu
{
  Esc,
  Audio,
  Camera,
  IMU,
  LEDs,
  Motors,
  Encoders,
  Power,
  SelfTest
};

enum AudioMenu
{
  EscAudio,
  PlaySineLow,
  PlaySineHigh,
  PlaySweep
};

enum IMUMenu
{
  EscIMU,
  StreamData,
  PrintAverages,
  StreamDance,
  Calibrate
};

enum LEDMenu
{
  EscLED,
  BlinkEyes,
  BlinkStatus,
  BlinkIR
};

enum PowerMenu
{
  EscPower,
  ReadVoltage,
  ReadCurrent
};

enum MotorsMenu
{
  EscMotors,
  LeftForward,
  LeftReverse,
  RightForward,
  RightReverse,
  HeadUp,
  HeadDown,
  LiftUp,
  LiftDown,
  RobotDance
};

enum SelfTestMenu
{
  EscSelfTest,
  RunSelfTest
};

namespace Anki
{
  namespace Cozmo
  {
    namespace DIAG_HAL
    {
      void AudioInit();
      void SineHigh();
      void SineLow();
      void Sweep();
      
      void LEDInit();
      void LEDBlinkEye();
      void LEDBlinkStatus();
      void LEDBlinkIR();
    }
  }
}