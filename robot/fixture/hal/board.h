#ifndef __BOARD_H
#define __BOARD_H

#include "stm32f2xx.h"

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

void InitBAT(void);
void EnableBAT(void);
void DisableBAT(void);


#endif 
