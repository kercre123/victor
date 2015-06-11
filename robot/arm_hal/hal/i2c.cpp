// Simple interrupt-driven Soft I2C peripheral capable of running long transactions asynchronously
// For HAL use only - see i2c.h for instructions, imu.cpp and camera.cpp for examples 
#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "hal/i2c.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      // Current I2C state as used in interrupt handler
      static s16* volatile m_list = NULL;  // Pointer to next I2COp

      // I2C pins
      GPIO_PIN_SOURCE(SCL, GPIOE, 3);
      GPIO_PIN_SOURCE(SDA, GPIOE, 5);

      #define GPIO_RCC  RCC_AHB1Periph_GPIOE

      // Use Timer 7 since it's available for general purpose use
      #define TIM        TIM7
      #define APB1_RCC   RCC_APB1Periph_TIM7
      #define IRQn       TIM7_IRQn
      #define IRQHandler TIM7_IRQHandler

      // Set I2C speed
      #define I2C_KHZ   100
      #define TICKS_PER_EDGE (APB1_CLOCK_MHZ * 1000 / I2C_KHZ / 2)

      extern "C" void IRQHandler(void);

      // Set up I2C hardware and stack
      void I2CInit()
      {
        RCC_AHB1PeriphClockCmd(GPIO_RCC, ENABLE);
        RCC_APB1PeriphClockCmd(APB1_RCC, ENABLE);

        GPIO_SET(GPIO_SCL, PIN_SCL);
        GPIO_SET(GPIO_SDA, PIN_SDA);
        
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_SCL | PIN_SDA;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;        
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_SCL, &GPIO_InitStructure); 

        // Initialize one-shot timer for background I2C update
        TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
        TIM_TimeBaseStructure.TIM_Prescaler = 0;
        TIM_TimeBaseStructure.TIM_Period = TICKS_PER_EDGE - 1;
        TIM_TimeBaseStructure.TIM_ClockDivision = 0;
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
        TIM_TimeBaseInit(TIM, &TIM_TimeBaseStructure);

        // Enable timer and interrupts
        TIM_SelectOnePulseMode(TIM, TIM_OPMode_Single);
        TIM_ITConfig(TIM, TIM_IT_Update, ENABLE);
        TIM_Cmd(TIM, ENABLE);
        TIM->CR1 |= TIM_CR1_URS;  // Prevent spurious interrupt when we touch EGR
        TIM->EGR = TIM_PSCReloadMode_Immediate;

        // Route interrupt
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannel = IRQn;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
        NVIC_Init(&NVIC_InitStructure);
      }

      // Read a register - wait for completion
      // @param addr 7-bit I2C address (not shifted left)
      // @param reg 8-bit register
      int I2CRead(u8 addr, u8 reg)
      {
        s16 ops[] = { 
          StartBit, 
          addr << 1,       // Write
          reg,             // Register number
          StopBit,
          StartBit,
          (addr << 1) + 1, // Read
          ReadByteEnd,     // Register value
          StopBit,
          EndOfList
        };
        
        I2CRun(ops);
        I2CWait();
        
        return ops[6];
      }

      // Write a register - wait for completion
      // @param addr 7-bit I2C address (not shifted left)
      // @param reg 8-bit register
      void I2CWrite(u8 addr, u8 reg, u8 val)
      {
        s16 ops[] = { 
          StartBit, 
          addr << 1,       // Write
          reg,             // Register number
          val,             // Register value
          StopBit,
          EndOfList
        };
        
        I2CRun(ops);
        I2CWait();
      }

      // Asynchronously run a list of I2C operations
      // You must call I2CWait before reading back the results
      // @param list Pointer to an array of I2COps (see below)
      void I2CRun(s16* list)
      {
        if (m_list != NULL) // Don't wait if a list is already in progress
          return;
        
        m_list = list;
        IRQHandler();   // Kick off the interrupt handler
      }

      // Wait for any outstanding I2CRun to complete
      void I2CWait()
      {
        while (NULL != m_list)
          ;
      }
    }
  }
}

// Handle one edge of the I2C clock - a single bit takes two edges
extern "C" void IRQHandler(void)
{
  using namespace Anki::Cozmo::HAL;
  
  // Clock edge count - even edges are SCK low + SDA setup, odd edges are SCK high + SDA sample
  // The final edge always leaves SCK low
  static s8 s_edge = 0;
  static u16 s_read = 0;  // Current value being read
  
  TIM->SR = 0;            // Acknowledge interrupt
  if (NULL == m_list)     // Give up if done
    return;

  // Execute the next operation in the list
  int op = *m_list;
  switch (op)
  {
    case EndOfList:     // Reset state and exit - no more interrupts
      m_list = NULL;
      s_edge = 0;
      return;
    
    case StartBit:      // Send a start bit
      GPIO_RESET(GPIO_SDA, PIN_SDA);    // First reset SDA
      if (1 == s_edge)
      {
        GPIO_RESET(GPIO_SCL, PIN_SCL);  // Then SCL
        s_edge = -1;                    // Advance to next operation
      }
      break;
      
    case StopBit:       // Send a stop bit
      GPIO_RESET(GPIO_SDA, PIN_SDA);  // Drop SDA
      if (1 == s_edge)
        GPIO_SET(GPIO_SCL, PIN_SCL);  // Raise SCL
      if (2 == s_edge)
      {
        GPIO_SET(GPIO_SDA, PIN_SDA);  // Raise SDA
        s_edge = -1;                  // Advance to next operation
      }
      break;
      
    case ReadByte:    // Read a byte
    case ReadByteEnd: 
      if (s_edge & 1)                 // Odd edges drive SCL high and sample
      {
        GPIO_SET(GPIO_SCL, PIN_SCL);
        s_read = (s_read << 1) | (!!(GPIO_READ(GPIO_SDA) & PIN_SDA));
      } else {                        // Even edges drive SCL low and setup
        GPIO_RESET(GPIO_SCL, PIN_SCL);
        GPIO_SET(GPIO_SDA, PIN_SDA);
      }
      if (16 == s_edge && ReadByte == op)
        GPIO_RESET(GPIO_SDA, PIN_SDA);  // Optionally send an ACK to continue reading
      if (18 == s_edge)
      {
        *m_list = (s_read >> 1) & 255;  // Store the read byte (minus ACK/NAK)
        s_edge = -1;                    // Advance to next operation       
      }
      break;
      
    default:        // Write a byte
      if (s_edge & 1)                 // Odd edges drive SCL high and sample
      {
        GPIO_SET(GPIO_SCL, PIN_SCL);
        s_read = !!(GPIO_READ(GPIO_SDA) & PIN_SDA);   // Remember ACK/NAK
      } else {                        // Even edges drive SCL low and setup
        GPIO_RESET(GPIO_SCL, PIN_SCL);
        int bit = (op << (s_edge >> 1)) & 0x80; // Fetch each bit, MSB to LSB
        if (s_edge >= 16 || bit)
          GPIO_SET(GPIO_SDA, PIN_SDA);
        else
          GPIO_RESET(GPIO_SDA, PIN_SDA);
      }
      if (18 == s_edge)
      {
        // *m_list = s_read;          // Could possibly store NAK/ACK
        s_edge = -1;                  // Advance to next operation
      }
      break;
  }
 
  s_edge++;           // Go to next edge
  if (0 == s_edge)    // On start of new command, advance list pointer
    m_list++;

  TIM->CR1 = TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_OPM; // Fire off another pulse
}
