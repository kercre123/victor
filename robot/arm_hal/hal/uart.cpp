#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"

#define BAUDRATE 1000000

#define GPIO_X        GPIOA
#define RCC_GPIO      RCC_AHB1Periph_GPIOA
#define RCC_UART      RCC_APB2Periph_USART1
#define GPIO_AF       GPIO_AF_USART1
#define UART          USART1

#define SOURCE_TX     9
#define SOURCE_RX     10
#define PIN_TX        (1 << SOURCE_TX)
#define PIN_RX        (1 << SOURCE_RX)


namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      void UARTInit()
      {
        // Clock configuration
        RCC_AHB1PeriphClockCmd(RCC_GPIO, ENABLE);
        RCC_APB2PeriphClockCmd(RCC_UART, ENABLE);

        // Configure the pins for UART in AF mode
        GPIO_InitTypeDef GPIO_InitStructure;
        GPIO_InitStructure.GPIO_Pin = PIN_TX | PIN_RX;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_X, &GPIO_InitStructure);
        GPIO_PinAFConfig(GPIO_X, SOURCE_TX, GPIO_AF_USART1);
        GPIO_PinAFConfig(GPIO_X, SOURCE_RX, GPIO_AF_USART1);

        // Configure the UART for the appropriate baudrate
        USART_InitTypeDef USART_InitStructure;
        USART_Cmd(UART, DISABLE);
        USART_InitStructure.USART_BaudRate = BAUDRATE;
        USART_InitStructure.USART_WordLength = USART_WordLength_8b;
        USART_InitStructure.USART_StopBits = USART_StopBits_1;
        USART_InitStructure.USART_Parity = USART_Parity_No;
        USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
        USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
        USART_Init(UART, &USART_InitStructure);
        USART_Cmd(UART, ENABLE);
      }

      int UARTPutChar(int c)
      {
        UART->DR = c;
        while (!(UART->SR & USART_FLAG_TXE))
          ;
        return c;
      }

      void UARTPutString(const char* s)
      {
        while (*s)
          UARTPutChar(*s++);
      }

      s32 UARTGetChar(u32 timeout)
      {
        u32 startTime = GetMicroCounter();

        do
        {
          // Make sure there's data in the FIFO
          if (UART->SR & USART_SR_RXNE)
          {
            // Ensure there won't be sign-extension
            return UART->DR & 0xff;
          }
        }
        while ((GetMicroCounter() - startTime) < timeout);

        // No data, so return with an error
        return -1;
      }
    }
  }
}

// Override fputc and fgetc for our own UART methods
int std::fputc(int c, FILE* f)
{
  Anki::Cozmo::HAL::UARTPutChar(c);
  return c;
}

int std::fgetc(FILE* f)
{
  return Anki::Cozmo::HAL::UARTGetChar();
}
