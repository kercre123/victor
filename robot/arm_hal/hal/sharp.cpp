#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/common/robot/trig_fast.h"
#include "hal/portable.h"

#include <stdlib.h>

GPIO_PIN_SOURCE(SCL, GPIOD, 6);
GPIO_PIN_SOURCE(SDA1, GPIOB, 10);
GPIO_PIN_SOURCE(SDA2, GPIOD, 7);
GPIO_PIN_SOURCE(INT, GPIOF, 7);

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

// Ambine light result, clear
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

struct I2CInterface
{
  char                ADDR_SLAVE_W;
  char                ADDR_SLAVE_R;
  GPIO_TypeDef*       GPIO_SCL;
  GPIO_TypeDef*       GPIO_SDA;
  uint32_t            PIN_SCL;
  uint32_t            PIN_SDA;
};

/* Goes in HAL
struct ProximityValues
{
  uint16_t    left;
  uint16_t    right;
  uint16_t    forward;
};
*/


namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {

      I2CInterface IRLeft, IRForward, IRRight;
      const u32 I2C_WAIT = 8;   // 8uS between clock edges - or 62.5KHz I2C

      // Soft I2C stack, borrowed from Arduino (BSD license)
      static void DriveSCL(u8 bit, I2CInterface *iface)
      {
        if (bit)
          GPIO_SET(iface->GPIO_SCL, iface->PIN_SCL);
          //GPIO_SET(GPIO_SCL, PIN_SCL);
        else
          GPIO_RESET(iface->GPIO_SCL, iface->PIN_SCL);;
          //GPIO_RESET(GPIO_SCL, PIN_SCL);

        MicroWait(I2C_WAIT);
      }

      static void DriveSDA(u8 bit, I2CInterface *iface)
      {
        if (bit)
          GPIO_SET(iface->GPIO_SDA, iface->PIN_SDA);
          //GPIO_SET(GPIO_SDA, PIN_SDA);
        else
          GPIO_RESET(iface->GPIO_SDA, iface->PIN_SDA);
          //GPIO_RESET(GPIO_SDA, PIN_SDA);

        MicroWait(I2C_WAIT);
      }

      // Read SDA bit by allowing it to float for a while
      // Make sure to start reading the bit before the clock edge that needs it
      static u8 ReadSDA(I2CInterface *iface)
      {
        GPIO_SET(iface->GPIO_SDA, iface->PIN_SDA);
        //GPIO_SET(GPIO_SDA, PIN_SDA);
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
          //if (m == 1)
          //  ReadSDA();  // Let SDA fall prior to last bit
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

      static void SensorInit(I2CInterface *iface)
      {
        data_write(iface, 0x01, PRST_4 | RES_A_14BIT | RANGE_A_X8);
        data_write(iface, 0x02, INTTYPE_LEVEL | RES_P_14BIT | RANGE_P_X8);
        data_write(iface, 0x03, INTVAL_0 | IS_130MA | PIN_PROX );
        data_write(iface, PL_LOW, 0x0F); // Set proximity sensor low threshold
        data_write(iface, PL_HIGH, 0x00);
        data_write(iface, PH_LOW, 0x0F); // Set proximity sensor high threshold
        data_write(iface, PH_HIGH, 0x00); 
        data_write(iface, 0x00, OP3 | OP2 | OP1_PS); // Enable sensor in PS mode
      }


      void SharpInit()
      {
       
        // Configure sharp sensors
        IRLeft.ADDR_SLAVE_W       =   ADDR_SLAVE_W_L;
        IRLeft.ADDR_SLAVE_R       =   ADDR_SLAVE_R_L;
        IRLeft.GPIO_SCL           =   GPIO_SCL;
        IRLeft.GPIO_SDA           =   GPIO_SDA1;
        IRLeft.PIN_SCL            =   PIN_SCL;
        IRLeft.PIN_SDA            =   PIN_SDA1;
        
        IRForward.ADDR_SLAVE_W    =   ADDR_SLAVE_W_L;
        IRForward.ADDR_SLAVE_R    =   ADDR_SLAVE_R_L;
        IRForward.GPIO_SCL        =   GPIO_SCL;
        IRForward.GPIO_SDA        =   GPIO_SDA2;
        IRForward.PIN_SCL         =   PIN_SCL;
        IRForward.PIN_SDA         =   PIN_SDA2;
        
        IRRight.ADDR_SLAVE_W      =   ADDR_SLAVE_W_H;
        IRRight.ADDR_SLAVE_R      =   ADDR_SLAVE_R_H;
        IRRight.GPIO_SCL          =   GPIO_SCL;
        IRRight.GPIO_SDA          =   GPIO_SDA1;
        IRRight.PIN_SCL           =   PIN_SCL;
        IRRight.PIN_SDA           =   PIN_SDA1;
        
        
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
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOD, &GPIO_InitStructure); 
        
        GPIO_InitStructure.GPIO_Pin = PIN_SDA1;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIOB, &GPIO_InitStructure); 

        GPIO_InitStructure.GPIO_Pin = PIN_INT;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
        GPIO_Init(GPIOF, &GPIO_InitStructure);     

        MicroWait(1000);
          
        SensorInit(&IRLeft);
        SensorInit(&IRForward);
        SensorInit(&IRRight);
      }
      
      void GetProximity(ProximityValues *prox)
      {
            prox->left = (data_read(&IRLeft, D2_HIGH) & 0xFF) << 8 | (data_read(&IRLeft, D2_LOW) & 0xFF);
            prox->forward = (data_read(&IRForward, D2_HIGH) & 0xFF) << 8 | (data_read(&IRForward, D2_LOW) & 0xFF);
            prox->right = (data_read(&IRRight, D2_HIGH) & 0xFF) << 8 | (data_read(&IRRight, D2_LOW) & 0xFF);
      }
      
    }
  }
}
