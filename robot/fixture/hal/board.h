// Based on Drive Testfix, updated for Cozmo EP1 Testfix
#ifndef __BOARD_H
#define __BOARD_H

#include "stm32f2xx.h"

#define PINC_CHGTX           6
#define PINC_CHGRX           7
#define PINB_SCL               8
#define PINB_SDA               9

#define GPIOC_CHGTX          (1 << PINC_CHGTX)
#define GPIOC_CHGRX          (1 << PINC_CHGRX)
#define GPIOB_SCL         (1 << PINB_SCL)
#define GPIOB_SDA         (1 << PINB_SDA)

#define PINB_VDD   0
#define ADC_VDD    8

#define PINC_RESET 5
#define ADC_RESET 15

#define PINC_TRX 12
#define GPIOC_TRX (1 << PINC_TRX)

#define PINA_ENCHG 15
#define GPIOA_ENCHG (1 << PINA_ENCHG)

#define PINB_SWD  10
#define GPIOB_SWD (1 << PINB_SWD)
#define PINB_SWC  11
#define GPIOB_SWC (1 << PINB_SWC)

#define PINB_MOTDRV_IN1   12
#define PINB_MOTDRV_EN1   13
#define PINB_MOTDRV_IN2   14
#define PINB_MOTDRV_EN2   15
#define GPIOB_MOTDRV_IN1  (1 << PINB_MOTDRV_IN1)
#define GPIOB_MOTDRV_EN1  (1 << PINB_MOTDRV_EN1)
#define GPIOB_MOTDRV_IN2  (1 << PINB_MOTDRV_IN2)
#define GPIOB_MOTDRV_EN2  (1 << PINB_MOTDRV_EN2)

#define PINC_BOARD_ID0    13
#define PINC_BOARD_ID1    14
#define PINC_BOARD_ID2    15
#define GPIOC_BOARD_ID0   (1 << PINC_BOARD_ID0)
#define GPIOC_BOARD_ID1   (1 << PINC_BOARD_ID1)
#define GPIOC_BOARD_ID2   (1 << PINC_BOARD_ID2)

#define PINA_NRF_SWD  11
#define GPIOA_NRF_SWD (1 << PINA_NRF_SWD)
#define PINA_NRF_SWC  12
#define GPIOA_NRF_SWC (1 << PINA_NRF_SWC)

#define PINC_NRF_RX  10
#define GPIOC_NRF_RX (1 << PINC_NRF_RX)
#define PINC_NRF_TX  11
#define GPIOC_NRF_TX (1 << PINC_NRF_TX)

#define PINB_DEBUGTX 6
#define GPIOB_DEBUGTX (1 << PINB_DEBUGTX)

#define PINA_DUTCS 4
#define PINA_SCK 5
#define PINA_MISO 6
#define PINA_MOSI 7
#define PINA_PROGHV 9
#define PINC_RESET 5

// Backpack LEDs/ADC channels
#define PINA_BPLED0 2
#define PINA_BPLED1 3
#define PINA_BPLED2 6
#define PINA_BPLED3 7

// Quadrature encoder IOs/ADC channels
#define PINA_ENCLED 4
#define PINC_ENCA   3
#define ADC_ENCA    13
#define PINC_ENCB   4
#define ADC_ENCB    14

typedef enum 
{
  LEDRED = 0,
  LEDGREEN = 1
} Led_TypeDef;


#define LED1_PIN                         GPIO_Pin_8
#define LED1_GPIO_PORT                   GPIOC
#define LED1_GPIO_CLK                    RCC_AHB1Periph_GPIOC
  
#define LED2_PIN                         GPIO_Pin_9
#define LED2_GPIO_PORT                   GPIOC
#define LED2_GPIO_CLK                    RCC_AHB1Periph_GPIOC

#define LEDn                             2

void STM_EVAL_LEDInit(Led_TypeDef Led);
void STM_EVAL_LEDOn(Led_TypeDef Led);
void STM_EVAL_LEDOff(Led_TypeDef Led);
void STM_EVAL_LEDToggle(Led_TypeDef Led);

//Fixture Hardware revisions
typedef enum {
  BOARD_REV_1_0_REV1  = 0,  //Fixture release 1.0 rev1
  BOARD_REV_1_0_REV2  = 1,  //Fixture release 1.0 rev2
  BOARD_REV_1_0_REV3  = 2,  //Fixture release 1.0 rev3
  BOARD_REV_1_5_0     = 3,  //Fixture release 1.5.0
} board_rev_t;

void InitBoard(void);
board_rev_t GetBoardRev(void);  //read the board revision
char* GetBoardRevStr(void);     //get board revision, descriptive string (const)
u8   GetBoardID(void);          //read the ID set resistors to determine test mode
void EnableBAT(void);
void DisableBAT(void);
void EnableVEXT(void);
void DisableVEXT(void);

#endif 
