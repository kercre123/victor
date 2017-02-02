#include <string.h>

// Placeholder bit-banging I2C implementation
// Vandiver:  Replace me with a nice DMA version that runs 4 transactions at a time off HALExec
// For HAL use only - see i2c.h for instructions, imu.cpp and camera.cpp for examples 
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/drop.h"
#include "anki/cozmo/robot/buildTypes.h"
#include "hal/portable.h"
#include "hal/i2c.h"
#include "hal/imu.h"
#include "hal/oled.h"
#include "MK02F12810.h"
#include "hal/hardware.h"

#define MAX_IRQS 4    // We have time for up to 4 I2C operations per drop

// Cribbed from core_cm4.h - these versions inline properly
#define EnableIRQ(IRQn) NVIC->ISER[(uint32_t)((int32_t)IRQn) >> 5] = (uint32_t)(1 << ((uint32_t)((int32_t)IRQn) & (uint32_t)0x1F))
#define DisableIRQ(IRQn) NVIC->ICER[((uint32_t)(IRQn) >> 5)] = (1 << ((uint32_t)(IRQn) & 0x1F))

using namespace Anki::Cozmo::HAL;

enum I2C_State {
  I2C_STATE_SET_EXPOSURE,
  I2C_STATE_IMU_SELECT,
  I2C_STATE_IMU_READ,
  I2C_STATE_OLED_SELECT,
  I2C_STATE_OLED_RECT,
  I2C_STATE_OLED_DATA
};

// Internal Settings
enum I2C_Control {
  I2C_C1_COMMON = I2C_C1_IICEN_MASK | I2C_C1_IICIE_MASK,

  // These are top level modes
  I2C_CTRL_STOP = I2C_C1_COMMON,
  I2C_CTRL_READ = I2C_C1_COMMON | I2C_C1_MST_MASK,
  I2C_CTRL_SEND = I2C_C1_COMMON | I2C_C1_MST_MASK | I2C_C1_TX_MASK,
    
  // These are modifiers
  I2C_CTRL_NACK = I2C_C1_TXAK_MASK,
  I2C_CTRL_RST  = I2C_C1_RSTA_MASK
};

// NOTE: THESE MUST BE POWERS OF TWO FOR PERFORMANCE REASONS
// DISPLAY WORD QUEUE CAN BE SHRANK ONCE THE ESPRESSIF FLOW
// CONTROL HAS BEEN IMPLEMENTED (should be 128 byte)
static const int DISPLAY_WORD_QUEUE = 1 << 8;
static const int RECTANGLE_QUEUE = 8;

static uint8_t displayFifo[DISPLAY_WORD_QUEUE];
static ScreenRect rectFifo[RECTANGLE_QUEUE];

static unsigned int displayWriteIndex;
static unsigned int displayReadIndex;
static unsigned int displayCount;

static unsigned int rectReadIndex;
static unsigned int rectWriteIndex;
static unsigned int rectCount;
static bool rectPending;

static ScreenRect* activeOutputRect;
static ScreenRect* activeInputRect;

// Random state values
#ifndef FCC_TEST
static unsigned int _irqsleft = 0; // int for performance
#endif
static I2C_State _currentState;
static unsigned int _stateCounter;
static bool _readIMU;
static bool _active;
static uint8_t* _readBuffer;

// Union of various camera settings so we only need two variables for
// current and target settings instead of two for each setting
union CameraSettings {
  uint16_t exposure;
  uint8_t gain;
};

static union CameraSettings currentCameraSettings;
static union CameraSettings targetCameraSettings;
// Which field to grab from the CameraSettings union
static bool areCameraSettingsForGain;

// These are the pointers for convenience
static uint8_t* const READ_TARGET = (uint8_t*)&IMU::ReadState;
static const int READ_SIZE = sizeof(IMU::ReadState);

extern "C" void I2C0_IRQHandler(void) __attribute__((section("CODERAM")));

// Send a stop condition first thing to make sure perfs are not holding the bus
static inline void SendEmergencyStop(void) {
  GPIO_SET(GPIO_I2C_SCL, PIN_I2C_SCL);

  // Drive PWDN and RESET to safe defaults
  GPIO_OUT(GPIO_I2C_SCL, PIN_I2C_SCL);
  SOURCE_SETUP(GPIO_I2C_SCL, SOURCE_I2C_SCL, SourceGPIO);

  // GPIO, High drive, open drain
  GPIO_IN(GPIO_I2C_SDA, PIN_I2C_SDA);
  SOURCE_SETUP(GPIO_I2C_SDA, PIN_I2C_SDA, SourceGPIO);
  
  // Clock the output 100 times
  for (int i = 0; i < 100; i++) {
    GPIO_RESET(GPIO_I2C_SCL, PIN_I2C_SCL);
    MicroWait(1);
    GPIO_SET(GPIO_I2C_SCL, PIN_I2C_SCL);
    MicroWait(1);
  }
}

void Anki::Cozmo::HAL::I2C::Init()
{
  SendEmergencyStop();
  
  // Enable clocking on I2C, PortB, PortE, and DMA
  SIM_SCGC4 |= SIM_SCGC4_I2C0_MASK;
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;

  // Configure port mux for i2c
  PORTB_PCR1 = PORT_PCR_MUX(2) |
               PORT_PCR_ODE_MASK |
               PORT_PCR_DSE_MASK |
               PORT_PCR_PE_MASK |
               PORT_PCR_PS_MASK;   //I2C0_SDA
  PORTE_PCR24 = PORT_PCR_MUX(5) |
                PORT_PCR_DSE_MASK;  //I2C0_SCL

  // Configure i2c
  I2C0_F  = I2C_F_ICR(0x1A);
  I2C0_C1 = I2C_CTRL_STOP;

  // Configure IRQs
  NVIC_SetPriority(I2C0_IRQn, 0);
}

void Anki::Cozmo::HAL::I2C::Start()
{
  memset(rectFifo, 0, sizeof(rectFifo));
  
  // Clear out rectangle buffers
  displayWriteIndex = 0;
  displayReadIndex = 0;
  displayCount = 0;

  rectReadIndex = 0;
  rectWriteIndex = 0;
  rectCount = 0;

  activeOutputRect = NULL;
  activeInputRect = rectFifo;

  // Setup current state (We send a bunch of junk data initially to unjam the i2c bus)
  _currentState = I2C_STATE_OLED_RECT;
  _stateCounter = 0;

  _active = false;
  _readIMU = true;
  rectPending = true;

  // Assume current camera settings are fine
  currentCameraSettings.exposure = 0;
  targetCameraSettings.exposure = 0;
  areCameraSettingsForGain = false;

  // Enable the bus with sufficient time for things to stablize
  I2C0_C1 = I2C_CTRL_SEND;
}

bool Anki::Cozmo::HAL::I2C::GetWatermark(void) {
  return displayCount >= (DISPLAY_WORD_QUEUE / 2);
}

void Anki::Cozmo::HAL::I2C::Enable(void) { 
  #ifndef FCC_TEST
  // Kick off the IRQ Handler again
  _irqsleft = MAX_IRQS;
  #endif
  
  // Can we safely transition out of a rectangle (idle)
  // Note: this only happens on drop boundaries for CPU conservation
  if (_currentState == I2C_STATE_OLED_DATA ||
      (_currentState == I2C_STATE_OLED_RECT && activeOutputRect == NULL)) {
    
    if (currentCameraSettings.exposure != targetCameraSettings.exposure) {
      if(areCameraSettingsForGain) {
        currentCameraSettings.gain = targetCameraSettings.gain;
      } else {
        currentCameraSettings.exposure = targetCameraSettings.exposure;
      }
      
      _currentState = I2C_STATE_SET_EXPOSURE;
      _stateCounter = 0;
    } else if (_readIMU) {
      _currentState = I2C_STATE_IMU_SELECT;
      _stateCounter = 0;

      _readBuffer = READ_TARGET;
      _readIMU = false;
    }
  }

  if (!_active) {
    _active = true;
    I2C0_IRQHandler();
  }

  EnableIRQ(I2C0_IRQn);
}

void Anki::Cozmo::HAL::I2C::SetCameraExposure(uint16_t lines) {
  targetCameraSettings.exposure = lines;
  areCameraSettingsForGain = false;
}

void Anki::Cozmo::HAL::I2C::SetCameraGain(uint8_t gain) {
  targetCameraSettings.gain = gain;
  areCameraSettingsForGain = true;
}

void Anki::Cozmo::HAL::I2C::ReadIMU(void) {
  _readIMU = true;
}

void Anki::Cozmo::HAL::I2C::FeedFace(bool rect, const uint8_t *face_bytes) {
  if (rect) {
    // Out of space for this rectangle, ignore
    if (rectCount >= RECTANGLE_QUEUE) {
      return ;
    }

    // Fill out the face with garbage to prevent stalling
    // This only occurs if SPI has failed
    int missing = activeInputRect->total - activeInputRect->received;
    if (missing > 0) {
      displayCount += missing;
      displayWriteIndex = (displayWriteIndex + missing) % DISPLAY_WORD_QUEUE;
    }

    // Write to the next rectangle
    activeInputRect = &rectFifo[rectWriteIndex];
    rectWriteIndex = (rectWriteIndex+1) % RECTANGLE_QUEUE;

    memcpy(activeInputRect, face_bytes, MAX_SCREEN_BYTES_PER_DROP);
    activeInputRect->total = 
      (activeInputRect->right - activeInputRect->left + 1) * 
      (activeInputRect->bottom - activeInputRect->top + 1);
    activeInputRect->sent = 0;
    activeInputRect->received = 0;

    // Select a rectangle if there isn't one currently
    if (activeOutputRect == NULL) {
      activeOutputRect = &rectFifo[rectReadIndex];
      rectReadIndex = (rectReadIndex+1) % RECTANGLE_QUEUE;
    }

    rectCount++;
  } else {
    // Enqueue our bytes
    int bytes = MIN(MAX_SCREEN_BYTES_PER_DROP, activeInputRect->total - activeInputRect->received);
    activeInputRect->received += bytes;
    displayCount += bytes;
    
    // Copy bytes into our ring bugger
    while (bytes-- > 0) {
      displayFifo[displayWriteIndex] = *(face_bytes++);
      displayWriteIndex = (displayWriteIndex + 1) % DISPLAY_WORD_QUEUE;
    }
  }
}

void I2C0_IRQHandler(void) {
  #ifndef FCC_TEST
  if (0 == --_irqsleft) {
    DisableIRQ(I2C0_IRQn);
  }
  #endif
  
  I2C0_S = I2C_S_IICIF_MASK;

  // This happens first, since it could kick off another I2C bus op
  // NOTE: there are two exit states so I can't put this in the switch

  if(_currentState == I2C_STATE_IMU_READ) {
    // We have received a byte (not processed)
    --_stateCounter;

    // NACK final byte
    if (_stateCounter == 1) {
      I2C0_C1 = I2C_CTRL_READ | I2C_CTRL_NACK;
    } else if (_stateCounter == 0) {
      I2C0_C1 = I2C_CTRL_SEND;

      // Reselect the OLED display if we need to continue transmitting data
      _currentState = rectPending ? I2C_STATE_OLED_RECT : I2C_STATE_OLED_SELECT;
    }

    *(_readBuffer++) = I2C0_D;
  }

  switch (_currentState) {
    case I2C_STATE_OLED_RECT:
      if (activeOutputRect == NULL) {
        _active = false;
        break ;
      }

      switch(_stateCounter++) {
        case 0:
          I2C0_C1 = I2C_CTRL_SEND | I2C_CTRL_RST;
          I2C0_D = SLAVE_WRITE(OLED_ADDR);
          break ;
        case 1:
          I2C0_D = I2C_COMMAND | I2C_CONTINUATION;
          break ;
        case 2:
          I2C0_D = COLUMNADDR;
          break ;
        case 3:
          I2C0_D = activeOutputRect->left;
          break ;
        case 4:
          I2C0_D = activeOutputRect->right;
          break ;
        case 5:
          I2C0_D = PAGEADDR;
          break ;
        case 6:
          I2C0_D = activeOutputRect->top;
          break ;
        case 7:
          I2C0_D = activeOutputRect->bottom;
          
          _currentState = I2C_STATE_OLED_SELECT;
          _stateCounter = 0;
          rectPending = false;

          break ;
      }      
      break ;
      
    // Select the IMU for reading data
    case I2C_STATE_IMU_SELECT:
      // Select our IMU
      switch(_stateCounter++) {
        case 0:
          I2C0_C1 = I2C_CTRL_SEND | I2C_CTRL_RST;
          I2C0_D = SLAVE_WRITE(ADDR_IMU);
          break ;
        case 1:
          I2C0_D = DATA_8;
          break ;
        case 2:
          I2C0_C1 = I2C_CTRL_SEND | I2C_CTRL_RST;
          I2C0_D = SLAVE_READ(ADDR_IMU);
          break ;
        case 3:
          I2C0_C1 = I2C_CTRL_READ;
          uint8_t dummy = I2C0_D;

          _currentState = I2C_STATE_IMU_READ;
          _stateCounter = READ_SIZE;
          break ;
      }

      break ;
   
    // Select the OLED for writing pixels (default state)
    case I2C_STATE_OLED_SELECT:
      switch(_stateCounter++) {
        case 0:
          I2C0_C1 = I2C_CTRL_SEND | I2C_CTRL_RST;
          I2C0_D = SLAVE_WRITE(OLED_ADDR);
          break ;
        case 1:
          I2C0_D = I2C_DATA | I2C_CONTINUATION;
          _currentState = I2C_STATE_OLED_DATA;
        
          break ;
      }
      break ;

    case I2C_STATE_OLED_DATA:
      // We are out of data
      if (displayCount == 0) {
        _active = false;
        break ;
      }

      // We have an active rectangle output data
      I2C0_D = displayFifo[displayReadIndex];
      displayReadIndex = (displayReadIndex + 1) % DISPLAY_WORD_QUEUE;
      displayCount--;

      // Select next rectangle when this is completed
      if (++activeOutputRect->sent >= activeOutputRect->total) {
        if (--rectCount > 0) {
          // Select the next rectangle
          activeOutputRect = &rectFifo[rectReadIndex];
          rectReadIndex = (rectReadIndex+1) % RECTANGLE_QUEUE;
        } else {
          activeOutputRect = NULL;
        }

        _currentState = I2C_STATE_OLED_RECT;
        _stateCounter = 0;
        rectPending = true;
      }

      break ;

    case I2C_STATE_SET_EXPOSURE:
      switch(_stateCounter++) {
        case 0:
          I2C0_C1 = I2C_CTRL_STOP;
          _active = false;
          break ;
        case 1:
          I2C0_C1 = I2C_CTRL_SEND;
          I2C0_D = SLAVE_WRITE(CAMERA_ADDR);
          break ;
        case 2:
          I2C0_D = (areCameraSettingsForGain ? CAMERA_GAIN : CAMERA_EXPOSURE);
          break ;
        case 3:
          if(areCameraSettingsForGain) {
            I2C0_D = (uint8_t)(currentCameraSettings.gain);
            
            // Skip state 4 since gain is only a byte
            _stateCounter++;
          } else {
            I2C0_D = (uint8_t)(currentCameraSettings.exposure >> 8);
          }
          break ;
        case 4:
          I2C0_D = (uint8_t)currentCameraSettings.exposure;
          break ;
        case 5:
          I2C0_C1 = I2C_CTRL_STOP;
          _active = false;
          break ;
        case 6:
          I2C0_C1 = I2C_CTRL_SEND;
          _currentState = I2C_STATE_IMU_SELECT;
          _stateCounter = 0;

          _readBuffer = READ_TARGET;
          _readIMU = false;
          _active = false;
          break ;
      }
      break ;
  }
}

// === Syncronous calls ===
// Don't warn about unused functions
#pragma diag_suppress 177
static void WriteByte(uint8_t data) {
  I2C0_D = data;
  while (~I2C0_S & I2C_S_IICIF_MASK) ;
  I2C0_S |= I2C_S_IICIF_MASK;
}

static void writeBlockSync(const uint8_t *bytes, int len) {
  I2C0_S |= I2C_S_IICIF_MASK;

  I2C0_C1 = I2C_CTRL_SEND;
  for (int i = 0; i < len; i++) {
    I2C0_D = *(bytes++);
    while (~I2C0_S & I2C_S_IICIF_MASK) ;
    I2C0_S |= I2C_S_IICIF_MASK;
  }
}

void Anki::Cozmo::HAL::I2C::WriteSync(const uint8_t *bytes, int len) {
  writeBlockSync(bytes, len);
  I2C0_C1 = I2C_CTRL_STOP;
  MicroWait(5);
}

void Anki::Cozmo::HAL::I2C::WriteReg(uint8_t slave, uint8_t addr, uint8_t data) {
  uint8_t cmd[] = { SLAVE_WRITE(slave), addr, data };

  WriteSync(cmd, sizeof(cmd));
}

uint8_t Anki::Cozmo::HAL::I2C::ReadReg(uint8_t slave, uint8_t addr) {
  uint8_t cmd[] = { SLAVE_WRITE(slave), addr };

  writeBlockSync(cmd, sizeof(cmd));

  I2C0_C1 = I2C_CTRL_SEND | I2C_CTRL_RST;
  I2C0_D = SLAVE_READ(slave);
  while (~I2C0_S & I2C_S_IICIF_MASK) ;
  I2C0_S |= I2C_S_IICIF_MASK;
  
  I2C0_C1 = I2C_CTRL_READ | I2C_CTRL_NACK;
  uint8_t data = I2C0_D;  // Dummy read for turnaround
  while (~I2C0_S & I2C_S_IICIF_MASK) ;
  I2C0_S |= I2C_S_IICIF_MASK;

  I2C0_C1 = I2C_CTRL_STOP;
  MicroWait(5);

  return I2C0_D;
}
