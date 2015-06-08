#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"
#include "anki/cozmo/robot/spineData.h"

#include "messages.h"

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

      void PrintCrap();

      //TimeStamp_t GetTimeStamp(void){ return (TimeStamp_t)0; }
      TimeStamp_t t_;
      TimeStamp_t GetTimeStamp(void){ return t_; }
      void SetTimeStamp(TimeStamp_t t) {t_ = t;}

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

      extern "C" {
        void EnableIRQ() {
          __enable_irq();
        }

        void DisableIRQ() {
          __disable_irq();
        }
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

int JPEGStart(u8* out, int width, int height, int quality);
int JPEGCompress(u8* out, u8* in);
int JPEGEnd(u8* out);
void JPEGInit();

// This function streams JPEG video in LongExecution
void StreamJPEG()
{
  using namespace Anki::Cozmo;

  const int FRAMESKIP = 1;  // Skip every other frame
  const int WIDTH = 320, HEIGHT = 240, QUALITY = 50;

  // Stack-allocate enough space for two whole image chunks, to handle overflow
  u8 buffer[IMAGE_CHUNK_SIZE*2];
  // Point message buffer +2 bytes ahead, to round 14 byte header up to 16 bytes for buffer alignment
  Anki::Cozmo::Messages::ImageChunk* m = (Anki::Cozmo::Messages::ImageChunk*)(buffer + 2);

  // Initialize the encoder
  JPEGStart(m->data, WIDTH, HEIGHT, QUALITY);

  m->resolution = Anki::Vision::CAMERA_RES_QVGA;
  m->imageEncoding = Anki::Vision::IE_MINIPEG_GRAY;
  m->imageId = 0;

  while (1)
  {
    // Skip frames (to prevent choking the Espressif)
    for (int i = 0; i < FRAMESKIP; i++)
    {
      while (HAL::CamGetReadyRow() != 0)
        ;
      while (HAL::CamGetReadyRow() == 0)
        ;
    }

    // Synchronize the timestamp with camera - wait for first row to arrive
    while (HAL::CamGetReadyRow() != 0)
      ;

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

      // At EOF, finish frame
      int eof = (row == HEIGHT-8);
      if (eof)
        datalen += JPEGEnd(m->data + datalen);

      // Write out any full chunks, or at EOF, anything left
      while (datalen >= IMAGE_CHUNK_SIZE || (eof && datalen))
      {
        // Leave imageChunkCount at 255 until the final chunk
        m->imageChunkCount = (eof && datalen <= IMAGE_CHUNK_SIZE) ? m->chunkId+1 : 255;
        m->chunkSize = MIN(datalen, IMAGE_CHUNK_SIZE);

        // On the first chunk, write the quality into the image (cheesy hack)
        if (0 == m->chunkId)
          m->data[0] = QUALITY;

        // Keep trying to send this message, even if it means a frame tear
        while (!HAL::RadioSendImageChunk(m, Messages::GetSize(Messages::ImageChunk_ID)))
          ;

        // Copy anything left at end to front of buffer
        datalen -= m->chunkSize;
        if (datalen)
          memcpy(m->data, m->data + IMAGE_CHUNK_SIZE, datalen);
        m->chunkId++;
      }
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

  Anki::Cozmo::Robot::Init();
  g_halInitComplete = true;
  //printf("init complete!\r\n");

  // Give time for sync before video starts
  MicroWait(500000);
  StreamJPEG();
#endif

  // Never return from this function
  while(1)
  {}
}

extern "C"
void __aeabi_assert(const char* s1, const char* s2, int s3)
{
}
