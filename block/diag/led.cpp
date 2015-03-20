/**
 * File: imu.cpp
 *
 * Author: Bryan Hood
 * Created: 4/11/2014
 *
 *
 * Description:
 *
 *		
 *
 * Copyright: Anki, Inc. 2013
 *
 **/

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "lib/stm32f4xx.h"



// Define SPI pins with macros
GPIO_PIN_SOURCE(LED_EYE_CLK, GPIOA, 11);
GPIO_PIN_SOURCE(LED_EYE_RST, GPIOA, 12);
GPIO_PIN_SOURCE(LED_EYE_RED, GPIOA, 2);
GPIO_PIN_SOURCE(LED_EYE_GREEN, GPIOA, 1);
GPIO_PIN_SOURCE(LED_EYE_BLUE, GPIOB, 1);
GPIO_PIN_SOURCE(LED_STAT_RED, GPIOE, 6);
GPIO_PIN_SOURCE(LED_STAT_GREEN, GPIOB, 9);
GPIO_PIN_SOURCE(LED_STAT_BLUE, GPIOE, 5);
GPIO_PIN_SOURCE(LED_IR, GPIOH, 13);

namespace Anki
{
  namespace Cozmo
  {
    namespace DIAG_HAL
    {
			void PulseWait(GPIO_TypeDef* gp, u32 pin, u32 microsec, bool active_level);
	
				void LEDAllOff()
			{
				// Eye
				GPIO_SET(GPIO_LED_EYE_RED, PIN_LED_EYE_RED);
				GPIO_SET(GPIO_LED_EYE_GREEN, PIN_LED_EYE_GREEN); 
				GPIO_SET(GPIO_LED_EYE_BLUE, PIN_LED_EYE_BLUE); 
				
				// Status
				GPIO_SET(GPIO_LED_STAT_RED, PIN_LED_STAT_RED);
				GPIO_SET(GPIO_LED_STAT_GREEN, PIN_LED_STAT_GREEN); 
				GPIO_SET(GPIO_LED_STAT_BLUE, PIN_LED_STAT_BLUE); 
				
				// IR
				GPIO_SET(GPIO_LED_IR, PIN_LED_IR);
			}
      
      // Setup all GPIO
      void LEDInit()
			{
				
				RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
				RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
				RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);
				RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOH, ENABLE);
				
				// Initalize Pins
				GPIO_InitTypeDef GPIO_InitStructure;
				
				// Set LED output pins
				GPIO_InitStructure.GPIO_Pin =  PIN_LED_EYE_CLK | PIN_LED_EYE_RST;
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
				GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
				GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
				GPIO_Init(GPIO_LED_EYE_CLK, &GPIO_InitStructure);  // GPIOA
				
				GPIO_InitStructure.GPIO_Pin =  PIN_LED_EYE_RED | PIN_LED_EYE_GREEN; 
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
				GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
				GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
				GPIO_Init(GPIO_LED_EYE_RED, &GPIO_InitStructure);  // GPIOA
				
				GPIO_InitStructure.GPIO_Pin =  PIN_LED_STAT_GREEN | PIN_LED_EYE_BLUE;
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
				GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
				GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
				GPIO_Init(GPIO_LED_STAT_GREEN, &GPIO_InitStructure);  // GPIOB
				
				GPIO_InitStructure.GPIO_Pin = PIN_LED_STAT_RED | PIN_LED_STAT_BLUE; 
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
				GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
				GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
				GPIO_Init(GPIO_LED_STAT_RED, &GPIO_InitStructure);  // GPIOE
				
				GPIO_InitStructure.GPIO_Pin = PIN_LED_IR; 
				GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
				GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
				GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
				GPIO_Init(GPIO_LED_IR, &GPIO_InitStructure);  // GPIOH
				
				LEDAllOff();
				GPIO_RESET(GPIO_LED_EYE_CLK, PIN_LED_EYE_CLK);
				GPIO_RESET(GPIO_LED_EYE_RST, PIN_LED_EYE_RST);
				
			}


			void LEDBlinkStatus()
			{
				PulseWait(GPIO_LED_STAT_RED, PIN_LED_STAT_RED, 500000, 0);
				PulseWait(GPIO_LED_STAT_GREEN, PIN_LED_STAT_GREEN, 500000, 0);
				PulseWait(GPIO_LED_STAT_BLUE, PIN_LED_STAT_BLUE, 500000, 0);
			}
			
			void LEDBlinkEye()
			{
				// reset 
				PulseWait(GPIO_LED_EYE_RST, PIN_LED_EYE_RST, 100, 1);

				for (uint8_t j = 0; j<3; j++)
				{
					for (uint16_t n = 0; n<1000; n++)
					{
						for (uint8_t i = 0; i<8; i++)
						{
							if (j % 3 == 0)
								PulseWait(GPIO_LED_EYE_RED, PIN_LED_EYE_RED, 100, 0);
							else if (j % 3 == 1)
								PulseWait(GPIO_LED_EYE_GREEN, PIN_LED_EYE_GREEN, 100, 0);
							else
								PulseWait(GPIO_LED_EYE_BLUE, PIN_LED_EYE_BLUE, 100, 0);
							//Clock
							PulseWait(GPIO_LED_EYE_CLK, PIN_LED_EYE_CLK, 10, 1);
						}
					}
				}
			}
			
			void LEDBlinkIR()
			{
				PulseWait(GPIO_LED_IR, PIN_LED_IR, 1000000, 0);
			}

			

			void PulseWait(GPIO_TypeDef* gp, u32 pin, u32 microsec, bool active_level)
			{
				if(active_level == 0)
				{
					GPIO_RESET(gp, pin);
					HAL::MicroWait(microsec);
					GPIO_SET(gp, pin);
				}
				else if(active_level == 1)
				{
					GPIO_SET(gp, pin);
					HAL::MicroWait(microsec);
					GPIO_RESET(gp, pin);
				}
			}
		}
	}
}