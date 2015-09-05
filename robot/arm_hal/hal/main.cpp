#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"
#include "clad/types/imageTypes.h"

#include "messages.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // True when main exec should run, false when it is ready to run
      extern volatile u8 g_runMainExec;
     
      // Forward declarations
      void Startup();
      void SPIInit();
      void TimerInit();
      void UARTInit();
      void FrontCameraInit();
      void IMUInit();
      void LightsInit();
      void OLEDInit(); 
      void AudioInit();
      void PrintCrap();
            
      //TimeStamp_t GetTimeStamp(void){ return (TimeStamp_t)0; }
      TimeStamp_t t_;
      TimeStamp_t GetTimeStamp(void){ return t_; }
      void SetTimeStamp(TimeStamp_t t) {t_ = t;}

      // ============ Stubs ==============
      // TODO: Move and implement these in some other file
      void FaceMove(s32 x, s32 y) {};
      void FaceBlink() {};
      
      // ======== End of Stubs ==========

      
      int UARTGetFreeSpace();

      static IDCard m_idCard;
      IDCard* GetIDCard() { return &m_idCard; }

      // XXX
      u8* CamGetRaw();
      int CamGetReadyRow();

      static void GetId()
      {
        // XXX: Replace with flash identification block
        printf("My ID: %08x", *(int*)(0x1FFF7A10));
        m_idCard.esn = 2;
        if (*(int*)(0x1FFF7A10) == 0x00250031)
          m_idCard.esn = 1;
      }
      
      volatile ImageSendMode imageSendMode_ = ISM_STREAM;
      volatile ImageResolution captureResolution_ = CVGA;
      void SetImageSendMode(const ImageSendMode mode, const ImageResolution res)
      {
        imageSendMode_ = mode;
        captureResolution_ = res;  // TODO: Currently ignored
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

// Yield to main execution - must be called every 1ms
void Yield()
{
  using namespace Anki::Cozmo::HAL;
  if (g_runMainExec)
  {
    Anki::Cozmo::Robot::step_MainExecution();
    g_runMainExec = 0;
  }
}

int JPEGStart(u8* out, int width, int height, int quality);
int JPEGCompress(u8* out, u8* in);
int JPEGEnd(u8* out);
void JPEGInit();

// This function streams JPEG video in LongExecution
void StreamJPEG()
{
  using namespace Anki::Cozmo;

  const int FRAMESKIP = 0;  // Skip every other frame
  const int WIDTH = 400, HEIGHT = 296, QUALITY = 50;

  // Stack-allocate enough space for two whole image chunks, to handle overflow
  u8 buffer[IMAGE_CHUNK_SIZE*2];
  // Point message buffer +2 bytes ahead, to round 14 byte header up to 16 bytes for buffer alignment
  Anki::Cozmo::ImageChunk* m = (Anki::Cozmo::ImageChunk*)(buffer + 2);

  // Initialize the encoder
  JPEGStart(m->data, WIDTH, HEIGHT, QUALITY);

  m->resolution    = CVGA;
  m->imageEncoding = IE_JPEGMinimizedGray;
  m->imageId = 0;

  while (1)
  {
    if (HAL::imageSendMode_ != ISM_OFF) {
      
      // Skip frames (to prevent choking the Espressif)
      for (int i = 0; i < FRAMESKIP; i++)
      {
        while (HAL::CamGetReadyRow() != 0)
          Yield();
        while (HAL::CamGetReadyRow() == 0)
          Yield();
      }

      // Synchronize the timestamp with camera - wait for first row to arrive
      while (HAL::CamGetReadyRow() != 0)
        Yield();

      // Setup image header
      m->frameTimeStamp = HAL::GetTimeStamp() - 33;   // XXX: 30 FPS
      m->imageId++;
      m->chunkId = 0;

      // Convert JPEG while writing it out
      int datalen = 0;
      for (int row = 0; row < HEIGHT; row += 8)
      {
        // Wait for data to be valid before compressing it
        while (HAL::CamGetReadyRow() != row)
          ;
        datalen += JPEGCompress(m->data + datalen, HAL::CamGetRaw());
        
        // Can only safely yield AFTER streaming image is read from buffer
        Yield();
        
        // At EOF, finish frame
        int eof = (row == HEIGHT-8);
        if (eof)
          datalen += JPEGEnd(m->data + datalen);

        // Write out any full chunks, or at EOF, anything left
        while (datalen >= IMAGE_CHUNK_SIZE || (eof && datalen))
        {
          // Leave imageChunkCount at 255 until the final chunk
          m->imageChunkCount = (eof && datalen <= IMAGE_CHUNK_SIZE) ? m->chunkId+1 : 255;
          m->data_length = MIN(datalen, IMAGE_CHUNK_SIZE);

          // On the first chunk, write the quality into the image (cheesy hack)
          if (0 == m->chunkId)
            m->data[0] = QUALITY;

          Anki::Cozmo::RobotInterface::SendMessage(*m, false);

          // Copy anything left at end to front of buffer
          datalen -= m->data_length;
          if (datalen)
            memcpy(m->data, m->data + IMAGE_CHUNK_SIZE, datalen);
          m->chunkId++;
        }
      }
      
      if (HAL::imageSendMode_ == ISM_SINGLE_SHOT) {
        HAL::imageSendMode_ = ISM_OFF;
      }
    } else {
      Yield();
    }
  }
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
  OLEDInit();
  printf("oled..");
  AudioInit();
  printf("audio..");

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

  Anki::Cozmo::Robot::Init();
  //printf("init complete!\r\n");

  // Give time for sync before video starts
  MicroWait(500000);
  StreamJPEG();
#endif

  // Never return from this function
  while(1) {
    Yield(); 
  }
}

extern "C"
void __aeabi_assert(const char* s1, const char* s2, int s3)
{
}
