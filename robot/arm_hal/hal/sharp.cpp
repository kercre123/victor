/**
* File: sharp.cpp
*
* Author: Bryan Hood
* Created: 10/7/2014
*
*
* Description:
*
*  Driver file for Sharp proximity sensor
*
* This code is EXPERIMENTAL for Cozmo proto 2.x, please remove for Cozmo proto 3+
* Only returns valid data on robot #2
* 
* // Goes in HAL
* enum sharpID
* {
*   IRleft,
*   IRforward,
*   IRright
* };
* 
* typedef struct
* {
*   u16           left;
*   u16           right;
*   u16           forward;
*   sharpID       latest;   // Most up to date sensor value
* } ProximityValues;
* 
*       // Interrupt driven proxmity (CALL AT BEGINNING OF LOOP)
*       // Note: this function is pipelined. // latency ~= 5 ms (1 main loop)
*       //       - returns data (from last function call)
*       //       - wake up the next sensor
*       //       - wait ~3.5 ms
*       //       - read from sensor
*       // Only call once every 5ms (1 main loop)
*       // current order is left -> right -> forward
*       void GetProximity(ProximityValues *prox)
*
*  Implementation notes:
*
* Copyright: Anki, Inc. 2014
*
**/

#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/trig_fast.h"
#include "hal/portable.h"

#include <stdlib.h>

GPIO_PIN_SOURCE(SCL, GPIOD, 6);
GPIO_PIN_SOURCE(SDA1, GPIOB, 10);
GPIO_PIN_SOURCE(SDA2, GPIOD, 7);
GPIO_PIN_SOURCE(INT, GPIOF, 7);
GPIO_PIN_SOURCE(TRIGGER, GPIOB, 11);

#undef assert
#define assert(x) if(!(x)) while(1);

// I2C_TRIGGER is for debugging on the scope
#define I2C_TRIGGER


// Registers

// 00H
#define OP3           0x80        // 0: shutdown mode 1: operating mode.
#define OP2           0x40        // 0: auto shutdown mode 1: continuous operating mode.
#define OP1_PSALS     0 << 4      // PS and ALS alternating mode 
#define OP1_ALS       1 << 4      // ALS mode
#define OP1_PS        2 << 4      // PS mode
#define OP1_TEST      3 << 4      // Test mode for PS
#define PROX          0x08        // 0: non-detection, 1: detection
#define FLAG_P        0x04        // 0: non-interrupt, 1: interrupt
#define FLAG_A        0x02        // 0: non-interrupt, 1: interrupt

// 01H
#define PRST_1        0 << 6      // 1 measurement cycle
#define PRST_4        1 << 6      // 4 measurement cycles
#define PRST_8        2 << 6      // 8 measurement cycles
#define PRST_16       3 << 6      // 16 measurement cycles
#define RES_A_19BIT   0 << 3
#define RES_A_18BIT   1 << 3
#define RES_A_17BIT   2 << 3
#define RES_A_16BIT   3 << 3      // recommended
#define RES_A_14BIT   4 << 3      // recommended
#define RES_A_12BIT   5 << 3
#define RES_A_10BIT   6 << 3
#define RES_A_8BIT    7 << 3
#define RANGE_A_X1    0
#define RANGE_A_X2    1
#define RANGE_A_X4    2
#define RANGE_A_X8    3           // recommended
#define RANGE_A_X16   4           // recommended
#define RANGE_A_X32   5           // recommended
#define RANGE_A_X64   6           // recommended
#define RANGE_A_X128  7           // recommended

// 02H
#define INTTYPE_LEVEL 0 << 6
#define INTTYPE_PULSE 1 << 6  
#define RES_P_16BIT   0 << 3 
#define RES_P_14BIT   1 << 3      // recommended
#define RES_P_12BIT   2 << 3      // recommended
#define RES_P_10BIT   3 << 3      // recommended    
#define RANGE_P_X1    0
#define RANGE_P_X2    1
#define RANGE_P_X4    2           // recommended
#define RANGE_P_X8    3           // recommended
#define RANGE_P_X16   4
#define RANGE_P_X32   5
#define RANGE_P_X64   6
#define RANGE_P_X128  7

// 03H
#define INTVAL_0      0 << 6      // 00: 0 time, 01: 4 times, 10: 8 times, 11: 16 times
#define INTVAL_4      1 << 6
#define INTVAL_8      2 << 6
#define INTVAL_16     3 << 6    
#define IS_16MA       0 << 4
#define IS_33MA       1 << 4 
#define IS_65MA       2 << 4
#define IS_130MA      3 << 4      //  recommended
#define PIN_FLAG_P    0 << 2
#define PIN_FLAG_A    1 << 2
#define PIN_FLAG_AP   2 << 2
#define PIN_PROX      3 << 2
#define FREQ          0x02        // 0: 327.5kHz (Duty during a PS measurement: 25.0%) 1: 81.8kHz (Duty during a PS measurement: 6.3%)  
#define RST           0x01        // Initialize all registers by writing 1 in RST register. RST register is also initialized automatically and becomes 0.

// Ambient light low threshold setting
#define TL_HIGH   0x05
#define TL_LOW    0x04

// Ambient light high threshold setting
#define TH_HIGH   0x07
#define TH_LOW    0x06

// Proximity low threshold setting (Loff)
#define PL_HIGH   0x09
#define PL_LOW    0x08

// Proximity high threshold setting (Lon)
#define PH_HIGH   0x0B
#define PH_LOW    0x0A

// Ambient light result, clear
#define D0_HIGH   0x0D
#define D0_LOW    0x0C

// Ambient light result, IR
#define D1_HIGH   0x0F
#define D1_LOW    0x0E

// Proxmity result
#define D2_HIGH   0x11
#define D2_LOW    0x10
    
// Addresses
#define ADDR_SLAVE_W_H 0x88 //slave address of GP2AP030A00F for write mode
#define ADDR_SLAVE_R_H 0x89 //slave address of GP2AP030A00F for read mode

#define ADDR_SLAVE_W_L 0x72 //slave address of GP2AP030A00F for write mode
#define ADDR_SLAVE_R_L 0x73 //slave address of GP2AP030A00F for read mode

// I2C configuration
#define I2C_ACK (0)

static volatile char I2Cstate;

struct I2CInterface
{
  char                ADDR_SLAVE_W;
  char                ADDR_SLAVE_R;
  GPIO_TypeDef*       GPIO_SCL;
  GPIO_TypeDef*       GPIO_SDA;
  uint32_t            PIN_SCL;
  uint32_t            PIN_SDA;
};

struct I2CWrite_Msg
{
  volatile I2CInterface*    IFACE;
  volatile char             ADDR_WORD;
  volatile char             DATA;
} write_msg;

struct I2CRead_Msg
{
  volatile I2CInterface*    IFACE;
  volatile char             ADDR_WORD;
  volatile char             DATA[2];
} read_msg;


namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {

      I2CInterface iface_left, iface_forward, iface_right;
      
      const u32 I2C_WAIT = 1;   // 8uS between clock edges - or 62.5KHz I2C (these numbers are likely not valid)

      // Soft I2C stack, borrowed from Arduino (BSD license)
      static void DriveSCL(u8 bit, I2CInterface *iface)
      {
        if (bit)
          GPIO_SET(iface->GPIO_SCL, iface->PIN_SCL);
        else
          GPIO_RESET(iface->GPIO_SCL, iface->PIN_SCL);;

        MicroWait(I2C_WAIT);
      }
      

      static void DriveSDA(u8 bit, I2CInterface *iface)
      {
        if (bit)
          GPIO_SET(iface->GPIO_SDA, iface->PIN_SDA);
        else
          GPIO_RESET(iface->GPIO_SDA, iface->PIN_SDA);

        MicroWait(I2C_WAIT);
      }
      
      // Read SDA bit by allowing it to float for a while
      // Make sure to start reading the bit before the clock edge that needs it
      static u8 ReadSDA(I2CInterface *iface)
      {
        GPIO_SET(iface->GPIO_SDA, iface->PIN_SDA);
        MicroWait(I2C_WAIT);
        return !!(GPIO_READ(iface->GPIO_SDA) & iface->PIN_SDA);
      }
      
      static u8 Read(u8 last, I2CInterface *iface)
      {
        u8 b = 0, i;

        for (i = 0; i < 8; i++)
        {
          b <<= 1;
          ReadSDA(iface);
          DriveSCL(1, iface);
          b |= ReadSDA(iface);
          DriveSCL(0, iface);
        }

        // send Ack or Nak
        if (last)
          DriveSDA(1, iface);
        else
          DriveSDA(0, iface);

        DriveSCL(1, iface);
        DriveSCL(0, iface);
        DriveSDA(1, iface);

        return b;
      }

      // Issue a Stop condition
      static void Stop(I2CInterface *iface)
      {
        DriveSDA(0, iface);
        DriveSCL(1, iface);
        DriveSDA(1, iface);
      }

      // Write byte and return true for Ack or false for Nak
      static u8 Write(u8 b,  I2CInterface *iface)
      {
        u8 m;
        // Write byte
        for (m = 0x80; m != 0; m >>= 1)
        {
          DriveSDA(m & b, iface);
          DriveSCL(1, iface);
          DriveSCL(0, iface);
        }

        DriveSCL(1, iface);
        b = ReadSDA(iface);
        DriveSCL(0, iface);

        return b;
      }

      // Issue a Start condition for I2C address with Read/Write bit
      static void Start(I2CInterface *iface)
      {
        DriveSDA(0, iface);
        DriveSCL(0, iface);
      }
      

      static uint16_t data_read(I2CInterface *iface, int addr)
      {
        uint16_t read_data = 0x0000;
        unsigned char ack;
        Start(iface); //start condition
        ack = Write(iface->ADDR_SLAVE_W, iface); //slave address send
        if (ack != I2C_ACK){
          //m_ErrorMsg();
        }
        ack = Write(addr, iface); //word address send
        if (ack != I2C_ACK) {
          //m_ErrorMsg();
        }
        Stop(iface);
        Start(iface);
        ack = Write(iface->ADDR_SLAVE_R, iface); //slave address send
        if (ack != I2C_ACK) {
          //m_ErrorMsg();
          }
        read_data = Read(1, iface); //nack
        Stop(iface);
        
        return read_data;
       } //End of data_read function
      
      
      static void data_write(I2CInterface *iface, int word_addr, int write_data)
      {
        unsigned char ack;
        
        Start(iface); //start condition     
        ack = Write((u8)(iface->ADDR_SLAVE_W), iface); //slave address send      
        if (ack != I2C_ACK) {
          Stop(iface);
          //m_ErrorMsg();
          return;
        }
        ack = Write(word_addr, iface ); //word address send
        if (ack != I2C_ACK) {
          Stop(iface);
          //m_ErrorMsg();
          return;
        }   
        ack = Write(write_data, iface); //write data send
        if (ack != I2C_ACK) {
          Stop(iface);
          //m_ErrorMsg();
          return;
        }
        Stop(iface);
        
        return;
      }


      static void SensorWakeup(I2CInterface *iface)
      {
        data_write(iface, 0x00,  OP1_PS | OP3); // PS mode and wakeup (operation)!
      }


      static void SensorInit(I2CInterface *iface)
      {
        data_write(iface, 0x01, PRST_1 | RES_A_14BIT | RANGE_A_X8);
        data_write(iface, 0x02, INTTYPE_PULSE | RES_P_10BIT | RANGE_P_X8);
        data_write(iface, 0x03, INTVAL_0 | IS_130MA | PIN_FLAG_P );
        data_write(iface, PL_LOW, 0x0F); // Set proximity sensor low threshold
        data_write(iface, PL_HIGH, 0x00);
        data_write(iface, PH_LOW, 0x0F); // Set proximity sensor high threshold
        data_write(iface, PH_HIGH, 0x00); 
        data_write(iface, 0x00,  OP1_PS); // Enable sensor in PS mode, auto-shutdown, currently in shutdown
      }


      void SharpInit()
      {
        // Set message sent flags (for interrupt operation)
        I2Cstate = 0;
       
        // Configure sharp sensors
        // forward
        iface_forward.ADDR_SLAVE_W       =   ADDR_SLAVE_W_L;
        iface_forward.ADDR_SLAVE_R       =   ADDR_SLAVE_R_L;
        iface_forward.GPIO_SCL           =   GPIO_SCL;
        iface_forward.GPIO_SDA           =   GPIO_SDA1;
        iface_forward.PIN_SCL            =   PIN_SCL;
        iface_forward.PIN_SDA            =   PIN_SDA1;
        
        // right
        iface_right.ADDR_SLAVE_W    =   ADDR_SLAVE_W_L;
        iface_right.ADDR_SLAVE_R    =   ADDR_SLAVE_R_L;
        iface_right.GPIO_SCL        =   GPIO_SCL;
        iface_right.GPIO_SDA        =   GPIO_SDA2;
        iface_right.PIN_SCL         =   PIN_SCL;
        iface_right.PIN_SDA         =   PIN_SDA2;
        
        // left
        iface_left.ADDR_SLAVE_W      =   ADDR_SLAVE_W_H;
        iface_left.ADDR_SLAVE_R      =   ADDR_SLAVE_R_H;
        iface_left.GPIO_SCL          =   GPIO_SCL;
        iface_left.GPIO_SDA          =   GPIO_SDA1;
        iface_left.PIN_SCL           =   PIN_SCL;
        iface_left.PIN_SDA           =   PIN_SDA1;
        
        
        // Enable GPIO clocks
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
        
        // Init Gpio
        GPIO_SET(GPIO_SCL, PIN_SCL);
        GPIO_SET(GPIO_SDA1, PIN_SDA1);
        GPIO_SET(GPIO_SDA2, PIN_SDA2);
        
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_SCL | PIN_SDA2;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;   // This MUST be set fast otherwise reads are unreliable 
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // Changed to UP for reliability (some hardware might not have pullups???)
        GPIO_Init(GPIOD, &GPIO_InitStructure); 
        
        GPIO_InitStructure.GPIO_Pin = PIN_SDA1;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;   // This MUST be set fast otherwise reads are unreliable 
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // Changed to UP for reliability (some hardware might not have pullups???)
        GPIO_Init(GPIOB, &GPIO_InitStructure); 
        
        GPIO_InitStructure.GPIO_Pin = PIN_INT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;   // This MUST be set fast otherwise reads are unreliable 
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_Init(GPIOF, &GPIO_InitStructure);     
        
        #ifdef I2C_TRIGGER
        GPIO_InitStructure.GPIO_Pin = PIN_TRIGGER;   // test port pin
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOB, &GPIO_InitStructure); 
        #endif


        // Set for 250 kHz clk signal. 89/1 prescaleer/period
        TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);
        TIM_TimeBaseStructure.TIM_Prescaler = 179; // 89
        TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseStructure.TIM_Period = 0x0001;
        TIM_TimeBaseStructure.TIM_ClockDivision = 0;
        TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
        TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);
        
        TIM_SelectOnePulseMode(TIM7, TIM_OPMode_Single);
        TIM7->EGR = TIM_PSCReloadMode_Immediate;
        
        // Route interrupt
        NVIC_InitTypeDef NVIC_InitStructure;
        NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn ;
        NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  
        NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2; 
        NVIC_Init(&NVIC_InitStructure);      
        // TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE); // Enable interrupt
        TIM_Cmd(TIM7, ENABLE);
        
        MicroWait(1000);
         
        SensorInit(&iface_left);
        SensorInit(&iface_forward);
        SensorInit(&iface_right); 
      }
      
         
      // Set up an interrupt based write.
      static void SetWriteMsg(I2CInterface *iface, int word_addr, int write_data)
      {
        write_msg.IFACE = iface;
        write_msg.ADDR_WORD = word_addr;
        write_msg.DATA = write_data;
      }
      
      
      // Set up an interrupt based read.
      static void SetReadMsg(I2CInterface *iface, int word_addr)
      {
        read_msg.IFACE = iface;
        read_msg.ADDR_WORD = word_addr;
      }

      // Interrupt driven proxmity (CALL AT BEGINNING OF LOOP)
      // Note: this function is pipelined. // latency ~= 5 ms (1 main loop)
      //       - returns data (from last function call)
      //       - wake up the next sensor
      //       - wait ~3.5 ms
      //       - read from sensor
      // Only call once every 5ms (1 main loop)
      // current order is left -> right -> forward
      void GetProximity(ProximityValues *prox)
      {
        static sharpID ID = IRleft;      
        
        // Only continue if we are robot #2;
        if (*(int*)(0x1FFF7A10) != 0x001d001d )
        {
          prox->forward = 0;
          prox->left = 0;
          prox->right = 0;
	  prox->latest = IRleft;
          return;
        }
        

        while (I2Cstate != 0) // both messages must be sent to proceed
        {
          MicroWait(1000);
          printf("State = %i (not hex!!)\r\n", I2Cstate);
        }
        
        switch(ID)
        {
          case IRforward:
            prox->latest = ID;
            ID = IRleft; // get left next
            prox->forward = (read_msg.DATA[1] & 0xFF) << 8 | (read_msg.DATA[0] & 0xFF);
            SetWriteMsg(&iface_left, 0x00, OP1_PS | OP3);   // schedule a write: wakeup, proximity mode
            SetReadMsg(&iface_left, D2_LOW); // schedule a read
            break;
            
          case IRleft:
            prox->latest = ID;
            ID = IRright; // get right next
            prox->left = (read_msg.DATA[1] & 0xFF) << 8 | (read_msg.DATA[0] & 0xFF);   
            SetWriteMsg(&iface_right, 0x00, OP1_PS | OP3); // schedule a write: wakeup, proximity mode
            SetReadMsg(&iface_right, D2_LOW); // schedule a read
            break;
            
          case IRright:
            prox->latest = ID;
            ID = IRforward; // get forward next
            prox->right = (read_msg.DATA[1] & 0xFF) << 8 | (read_msg.DATA[0] & 0xFF);   
            SetWriteMsg(&iface_forward, 0x00, OP1_PS | OP3);  // schedule a write: wakeup, proximity mode
            SetReadMsg(&iface_forward, D2_LOW); // schedule a read 
            break;  
        }
/*
        if (read_msg.STATUS == I2C_FAILURE || write_msg.STATUS == I2C_FAILURE)
          prox->status = IR_I2C_ERROR;
        else if ( read_msg.DATA[1] > 3) // should only be 10 bits, rules out double read errors
          prox->status = IR_I2C_ERROR;
        else
          prox->status = IR_GOOD;
          
        write_msg.SENT_FLAG = I2C_UNSENT;
        read_msg.SENT_FLAG = I2C_UNSENT;
*/
        TIM_ITConfig(TIM7, TIM_IT_Update, ENABLE); // Enable interrupt
      }
      
    }
  }
}


/*
Notes:
 - This ISR is currently configured for a particular sharp sensor configuration. Basic operation is as follows:
 1) Data write (1 byte) // wake up sensor
 2) Wait ~ 3.5-4 ms // wait for data to become available
 3) Data read (2 bytes) // read both bytes of data
 
 - Each interrupt represents a half I2C clock cycle. The I2Cstate machine is in a switch I2Cstatement
    that is linear except for 8 bit read/writes which loop 
    
 - The interrupt flag is cleared at the beginning of the routine
 - 
*/
extern "C" void TIM7_IRQHandler(void)
{
  static TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  static char bits, m;
  static int long_count;
//  TIM7->CR1 &= ~0x01;           // Disable Counter
  TIM7->SR = 0;        // Reset interrupt flag
  
  // check for new messages
  // set m, I2Cstate, bits here
  if(I2Cstate == 0)
  {
    m = 0x80; // reset m
    bits = 0;
    I2Cstate = 0x00;
    read_msg.DATA[0] = 0x00;
    read_msg.DATA[1] = 0x00;
    long_count = 0;
  }
  
  switch(I2Cstate)
  {
    //// send a start bit ////
    case 0x00:  // start bit (part 1)
      #ifdef I2C_TRIGGER
      GPIO_SET(GPIO_TRIGGER, PIN_TRIGGER);
      #endif
      GPIO_RESET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
    
    case 0x01: // start bit (part 2)
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;
      
    //// send slave_write address ////
    case 0x02: // reset clock, set data (part 1 x 8)
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      (m & write_msg.IFACE->ADDR_SLAVE_W) ? GPIO_SET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA) : GPIO_RESET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      m >>= 1;
      I2Cstate++;
      // wait
      break;
    
    case 0x03: // set clock (part 2 x 8)
      GPIO_SET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      m == 0 ? I2Cstate++ : I2Cstate--;
      // wait
      break;
    
    case 0x04: // end with a clock reset, and reset m (part 3)
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL); 
      m = 0x80; // reset m
      I2Cstate++;
      // wait
      break;

    //// receive ack ////
    case 0x05: // set clock and data (part 1)
      GPIO_SET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      GPIO_SET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
                
    case 0x06: // read ack and reset clock (part 2)
      if ((!!(GPIO_READ(write_msg.IFACE->GPIO_SDA) & write_msg.IFACE->PIN_SDA)) != I2C_ACK)
      {
        assert(0);
        I2Cstate = 0x35;
      }
      else // success
      {
        I2Cstate++;
      }
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      GPIO_RESET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      // wait
      break;
      
    //// send word address ////
    case 0x07: // reset clock, set data (part 1 x 8)
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      (m & write_msg.ADDR_WORD) ? GPIO_SET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA) : GPIO_RESET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      m >>= 1;
      I2Cstate++;
      // wait
      break;
    
    case 0x08: // set clock (part 2 x 8)
      GPIO_SET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      m == 0 ? I2Cstate++ : I2Cstate--;
      // wait
      break;
    
    case 0x09: // end with a clock reset and reset m (part 3)
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      m = 0x80; // reset m
      I2Cstate++;
      // wait
      break;
      
    //// receive ack ////
    case 0x0A: // set clock and data (part 1)
      GPIO_SET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      GPIO_SET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
                
    case 0x0B: // read data and reset clock (part 2)
      if ((!!(GPIO_READ(write_msg.IFACE->GPIO_SDA) & write_msg.IFACE->PIN_SDA)) != I2C_ACK)
      {
        assert(0);
        I2Cstate = 0x35;
      }
      else // success
      {
        I2Cstate++;
      }
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      GPIO_RESET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      // wait
      break;

    //// send data ////
    case 0x0C: // reset clock, set data (part 1 x 8)
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      (m & write_msg.DATA) ? GPIO_SET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA) : GPIO_RESET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      m >>= 1;
      I2Cstate++;
      // wait
      break;
    
    case 0x0D: // set clock (part 2 x 8)
      GPIO_SET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      m == 0 ? I2Cstate++ : I2Cstate--;
      // wait
      break;
    
    case 0x0E: // end with a clock reset and reset m (part 3)
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      m = 0x80; // reset m
      I2Cstate++;
      // wait
      break;
       
    //// receive ack ////
    case 0x0F: // set clock and data (part 1)
      GPIO_SET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      GPIO_SET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
                
    case 0x10: // read data and reset clock (part 2)
      if ((!!(GPIO_READ(write_msg.IFACE->GPIO_SDA) & write_msg.IFACE->PIN_SDA)) != I2C_ACK)
      {
        assert(0);
        I2Cstate = 0x35;
      }
      else // success
      {
        I2Cstate++;
      }
      GPIO_RESET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      GPIO_RESET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      // wait
      break;
      
    //// stop condition ////
    case 0x11: // reset data (part 1)
      GPIO_RESET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
      
    case 0x12: // set clock (part 2)
      GPIO_SET(write_msg.IFACE->GPIO_SCL, write_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;
      
    case 0x13: // set data (part 3)
      GPIO_SET(write_msg.IFACE->GPIO_SDA, write_msg.IFACE->PIN_SDA);
      if(long_count == 750)
      {
        I2Cstate++;
        long_count = 0;
      }
      else
      {
        long_count++;
      }
      // wait
      //TIM7->ARR = 2000; // approx 3.75 or 4 ms wait
      break;

//////////////////////////////


    //// send a start bit ////
    case 0x14:  // start bit (part 1)
      GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      I2Cstate++;
      //TIM7->ARR = 1; // put this back
      // wait
      break;
    
    case 0x15: // start bit (part 2)
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;
      
    //// send slave_write address ////
    case 0x16: // reset clock, set data (part 1 x 8)
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      (m & read_msg.IFACE->ADDR_SLAVE_W) ? GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA) : GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      m >>= 1;
      I2Cstate++;
      // wait
      break;
    
    case 0x17: // set clock (part 2 x 8)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      m == 0 ? I2Cstate++ : I2Cstate--;
      // wait
      break;
    
    case 0x18: // end with a clock reset, and reset m (part 3)
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL); 
      m = 0x80; // reset m
      I2Cstate++;
      // wait
      break;

    //// receive ack ////
    case 0x19: // set clock and data (part 1)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
                
    case 0x1A: // read ack and reset clock (part 2)
      if ((!!(GPIO_READ(read_msg.IFACE->GPIO_SDA) & read_msg.IFACE->PIN_SDA)) != I2C_ACK)
      {
        assert(0);
        I2Cstate = 0x35;
      }
      else // success
      {
        I2Cstate++;
      }
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      // wait
      break;
      
    //// send word address ////
    case 0x1B: // reset clock, set data (part 1 x 8)
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      (m & read_msg.ADDR_WORD) ? GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA) : GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      m >>= 1;
      I2Cstate++;
      // wait
      break;
    
    case 0x1C: // set clock (part 2 x 8)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      m == 0 ? I2Cstate++ : I2Cstate--;
      // wait
      break;
    
    case 0x1D: // end with a clock reset and reset m (part 3)
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      m = 0x80; // reset m
      I2Cstate++;
      // wait
      break;
      
    //// receive ack ////
    case 0x1E: // set clock and data (part 1)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
                
    case 0x1F: // read data and reset clock (part 2)
      if ((!!(GPIO_READ(read_msg.IFACE->GPIO_SDA) & read_msg.IFACE->PIN_SDA)) != I2C_ACK)
      {
        assert(0);
        I2Cstate = 0x35;
      }
      else // success
      {
        I2Cstate++;
      }
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      // wait
      break;
     
    //// stop condition ////
    case 0x20: // set clock (part 1)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;

    case 0x21: // set data (part 2)
      GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
     
    //// send a start bit ////
    case 0x22:  // start bit (part 1)
      GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
    
    case 0x23: // start bit (part 2)
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;
    
    //// send slave_read address ////
    case 0x24: // reset clock, set data (part 1 x 8)
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      (m & read_msg.IFACE->ADDR_SLAVE_R) ? GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA) : GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      m >>= 1;
      I2Cstate++;
      // wait
      break;
    
    case 0x25: // set clock (part 2 x 8)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      m == 0 ? I2Cstate++ : I2Cstate--;
      // wait
      break;
    
    case 0x26: // end with a clock reset, and reset m (part 3)
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL); 
      m = 0x80; // reset m
      I2Cstate++;
      // wait
      break;
      
    //// receive ack ////
    case 0x27: // set clock and data (part 1)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
                
    case 0x28: // read data and reset clock (part 2)
      if ((!!(GPIO_READ(read_msg.IFACE->GPIO_SDA) & read_msg.IFACE->PIN_SDA)) != I2C_ACK)
      {
        assert(0);
        I2Cstate = 0x35;
      }
      else // success
      {
        I2Cstate++;
      }
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      //GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA); // (let data continue to float?!) TODO, fix the others in ACK. maybe they don't need it?
      // wait
      break;

    //// read data 1 ////
    case 0x29: // set clock high, read data (part 1)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      read_msg.DATA[0] |= ((!!(GPIO_READ(read_msg.IFACE->GPIO_SDA) & read_msg.IFACE->PIN_SDA)));
      bits++;
      I2Cstate++;
      // wait
      break;
      
    case 0x2A: // set clock low
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      if(bits == 8)
      {
        I2Cstate++;
        bits = 0;
      }
      else
      {
        read_msg.DATA[0] <<= 1;
        I2Cstate--;
      }
      // wait
      break;
    
    //// send ack ////
    case 0x2B: // set the ack
      I2C_ACK == 0 ? GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA) : GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
      
    case 0x2C: // clock high
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;
      
    case 0x2D: // clock low, let data float
      GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;
      
    //// read data 2 ////
    case 0x2E: // set clock high, read data (part 1)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      read_msg.DATA[1] |= ((!!(GPIO_READ(read_msg.IFACE->GPIO_SDA) & read_msg.IFACE->PIN_SDA)));
      bits++;
      I2Cstate++;
      // wait
      break;
      
    case 0x2F: // set clock low
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      if(bits == 8)
      {
        I2Cstate++;
        bits = 0;
      }
      else
      {
        read_msg.DATA[1] <<= 1;
        I2Cstate--;
      }
      // wait
      break;
      
    //// send nack ////
    case 0x30: // set the nack
      I2C_ACK == 0 ? GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA) : GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      I2Cstate++;
      // wait
      break;
      
    case 0x31: // clock high
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;    
    
    //// stop condition (SUCCESS)////
    case 0x32: // reset data (part 1)
      GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;
      
    case 0x33: // set clock (part 2)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;

    case 0x34: // set data (part 3)
      GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      #ifdef I2C_TRIGGER
      GPIO_RESET(GPIO_TRIGGER, PIN_TRIGGER);
      #endif
      TIM_ITConfig(TIM7, TIM_IT_Update, DISABLE); // Disable interrupt
      I2Cstate = 0;
      // wait
      break;
      
    //// stop condition (FAILURE) ////
    case 0x35: // reset data (part 1)
      GPIO_RESET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      GPIO_RESET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;
      
    case 0x36: // set clock (part 2)
      GPIO_SET(read_msg.IFACE->GPIO_SCL, read_msg.IFACE->PIN_SCL);
      I2Cstate++;
      // wait
      break;

    case 0x37: // set data (part 3)
      GPIO_SET(read_msg.IFACE->GPIO_SDA, read_msg.IFACE->PIN_SDA);
      #ifdef I2C_TRIGGER
      GPIO_RESET(GPIO_TRIGGER, PIN_TRIGGER);
      #endif
      TIM_ITConfig(TIM7, TIM_IT_Update, DISABLE); // Disable interrupt
      I2Cstate = 0;
      // wait
      break;
  }
  TIM7->CR1 = TIM_CR1_CEN | TIM_CR1_URS | TIM_CR1_OPM; // Fire off one pulse
//   TIM7->CR1 |= 0x01;           // Ensable Counter
}



