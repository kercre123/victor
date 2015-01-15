/**
  ******************************************************************************
  * @file    main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    23-March-2012
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/** @addtogroup STM32F0-Discovery_Demo
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay;
uint8_t BlinkSpeed = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
	
	// Initialize system timers, excluding watchdog
// This must run first in main()
void InitTimers(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

  // Initialize the 1 microsecond timer
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
  TIM_TimeBaseStructure.TIM_Prescaler = 47;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = 0xffff;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
  TIM_Cmd(TIM6, ENABLE);
}


// Get the number of microseconds since boot
u32 getMicroCounter(void)
{
	// The code below turns the 16-bit TIM6 into a 32-bit timer
	volatile static u16 high = 0;	// Supply the missing high bits of TIM6
	volatile static u16 last = 0;	// Last read of TIM6
  
  // NOTE:  This must be interrupt-safe for encoder code, so take care below
  //__disable_irq();
  u32 now = TIM6->CNT;	
	
	if (now < last)				// Each time we wrap TIM6, increase the high part by 1
		high++;	
	
	last = now;
  now |= (high << 16);
  //__enable_irq();
  
	return now;
}

// Wait the specified number of microseconds
void MicroWait(u32 us)
{
  u32 start = getMicroCounter();
  us++;   // Wait 1 tick longer, in case we are midway through start tick

  // Note: The below handles wrapping with unsigned subtraction
  while (getMicroCounter()-start < us)
    ;
}

#define GPIO_RES		GPIOA // 8
#define	PIN_RES			GPIO_Pin_0 // 8
#define GPIO_A0			GPIOA // 9
#define	PIN_A0			GPIO_Pin_1 // 9
#define GPIO_RD1		GPIOA // 10
#define	PIN_RD1			GPIO_Pin_2 // 10
#define	GPIO_WR1		GPIOA
#define	PIN_WR1			GPIO_Pin_3 // 11

#define GPIO_DATA		GPIOB

void InitLCDGPIO()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	
	 /* Configure LCD control pins */
  GPIO_SetBits(GPIO_A0, PIN_A0);
	GPIO_SetBits(GPIO_WR1, PIN_WR1);
	GPIO_SetBits(GPIO_RES, PIN_RES);
	GPIO_SetBits(GPIO_RD1, PIN_RD1);
	
	GPIO_InitStructure.GPIO_Pin = PIN_A0 | PIN_WR1 | PIN_RES | PIN_RD1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_A0, &GPIO_InitStructure); /* GPIOA */

	GPIO_SetBits(GPIO_A0, PIN_A0);
	GPIO_SetBits(GPIO_WR1, PIN_WR1);
	GPIO_SetBits(GPIO_RES, PIN_RES);
	GPIO_SetBits(GPIO_RD1, PIN_RD1);

	/* Configure LCD data pins */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_DATA, &GPIO_InitStructure); /* GPIOB */

}

/*****************************************************/
void data_out(unsigned char i)
//Data Output 8-bit parallel Interface
{
	/*
	A0 = 1; //Data register
	WR1 = 0; //Write enable 
	P1 = i; //put data on port 1
	WR1 = 1; //Clock in data
	*/
	GPIO_SetBits(GPIO_A0, PIN_A0);
	MicroWait(20);
	GPIO_ResetBits(GPIO_WR1, PIN_WR1);
	MicroWait(20);
	GPIO_Write(GPIO_DATA, i);
	MicroWait(20);
	GPIO_SetBits(GPIO_WR1, PIN_WR1);
	MicroWait(20);
}

void comm_out(unsigned char i)
//Command Output 8-bit parallel Interface
{
	/*
	A0 = 0; //Instruction register
	WR1 = 0; //Write enable
	P1 = i; //put data on port 1
	WR1 = 1; //Clock in data
	*/
	GPIO_ResetBits(GPIO_A0, PIN_A0);
	MicroWait(20);
	GPIO_ResetBits(GPIO_WR1, PIN_WR1);
	MicroWait(20);
	GPIO_Write(GPIO_DATA, i);
	MicroWait(20);
	GPIO_SetBits(GPIO_WR1, PIN_WR1);
	MicroWait(20);
}
/*****************************************************/

/****************************************************
*
Initialization
For
NT7534H
*
*****************************************************/
void resetLCD()
{
	/*
	RES = 0;
	delay(100);
	RES = 1;
	delay(100);
	*/
	GPIO_ResetBits(GPIO_RES, PIN_RES);
	MicroWait(100000);
	GPIO_SetBits(GPIO_RES, PIN_RES);
	MicroWait(100000);
}

void init_LCD()
{
	resetLCD();
	//CS1 = 0; //Chip Select CS2 = 1; //Chip Select
	//RD1 = 1; //Read disable
	GPIO_SetBits(GPIO_RD1, PIN_RD1);
	comm_out(0xA2); //1/9 bias
	comm_out(0xA0); //ADC select
	comm_out(0xC8); //COM output reverse
	comm_out(0xA4); //display all points normal
	comm_out(0x40); //display start line set
	comm_out(0x25); //internal resistor ratio
	comm_out(0x81); //electronic volume mode set
	comm_out(0x18); //electronic volume
	comm_out(0x2F); //power controller set
	comm_out(0xAF); //display on
	comm_out(0xA5); //display pixels
}
/*****************************************************/
	
	
int main(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
	GPIO_InitTypeDef  GPIO_InitStructure;
	
	int i, j;
	int filter;
	
	InitTimers();
	
	//////////// LED testing
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure); /* GPIOA */

  BlinkLed();
  
  /*
	while(1)
	{
		for(j = 0; j<1001; j++)
		{
			for(i = 0; i<20; i++)
			{
				GPIO_SetBits(GPIOA, GPIO_Pin_8);
				GPIO_SetBits(GPIOA, GPIO_Pin_9);
				GPIO_SetBits(GPIOA, GPIO_Pin_10);
				GPIO_SetBits(GPIOA, GPIO_Pin_11);
				GPIO_SetBits(GPIOA, GPIO_Pin_12);
				MicroWait(j);
				//MicroWait(1000);
				GPIO_ResetBits(GPIOA, GPIO_Pin_8);
				GPIO_ResetBits(GPIOA, GPIO_Pin_9);
				GPIO_ResetBits(GPIOA, GPIO_Pin_10);
				GPIO_ResetBits(GPIOA, GPIO_Pin_11);
				GPIO_ResetBits(GPIOA, GPIO_Pin_12);			
				MicroWait(1000-j);
				//MicroWait(300);
			}
		}
	}*/
	
	////////////
	
	init_LCD();
	InitLCDGPIO();
	
	while(1)
	{
		data_out(0xA5);
		//comm_out(0xA6);
		//MicroWait(500000);
		//comm_out(0xA7);
		MicroWait(50000);
	}
	
  /* Enable the GPIO_LED Clock */
  //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

  /* Configure the GPIO_LED pin */
	/*
  GPIO_SetBits(GPIOB, GPIO_Pin_1);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_1);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_0);
	*/
  
  
  /* SysTick end of count event each 1ms */
  //RCC_GetClocksFreq(&RCC_Clocks);
  //SysTick_Config(RCC_Clocks.HCLK_Frequency / 100000);
   
 
  
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in 1 ms.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{
  TimingDelay = nTime;

  while(TimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (TimingDelay != 0x00)
  { 
    TimingDelay--;
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
