#include "lib/stm32f2xx.h"
#include "hal/aci.h"
#include "hal/uart.h"
#include "hal/timers.h"

#define GPIOC_REQN GPIO_Pin_4
#define GPIOA_RDYN GPIO_Pin_7
#define PINA_SCK   GPIO_PinSource5
#define PINA_RDYN  GPIO_PinSource7

// Number of microseconds to wait for Nordic before trying again next frame
#define NORDIC_TIMEOUT  65   // As long as we can afford (3% of a frame)

// Space for the raw radio messages
u8 g_acibuf[MAX_ACI_BYTES];

void EnableACI(void)
{  
  // Return RDYN to pull-up input and SCK to output
  __disable_irq();
  //g_aciBusy = 1;
  GPIOA->MODER &= ~(GPIO_MODER_MODER0 << (PINA_RDYN*2));   // Revert RDYN to input
  GPIOA->PUPDR |= (GPIO_PuPd_UP << (PINA_RDYN*2));         // Enable RDYN pull-up  
  __enable_irq();
  MicroWait(1);   // Settling time due to weak pull-up on RDYN
}

// Switch LED/ACI multiplexing to LED mode
void DisableACI(void)
{
  __disable_irq();
  
  GPIOA->MODER &= ~(GPIO_MODER_MODER0 << (PINA_RDYN*2));   // Revert RDYN to output
  GPIOA->MODER |= (GPIO_Mode_OUT << (PINA_RDYN*2));        // Will light or not based on BSRR/BRR status
  GPIOA->PUPDR &= ~(0x3 << (PINA_RDYN*2));                 // Disable RDYN pull-up
  
  GPIOA->MODER &= ~(GPIO_MODER_MODER0 << (PINA_SCK*2));    // Revert SCK to input
  //g_aciBusy = 0;
  __enable_irq();
}

void InitACI(void)
{
  int i;
  GPIO_InitTypeDef GPIO_InitStructure;
  SPI_InitTypeDef SPI_InitStructure;

  // Configure clocks and GPIO
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

  // Make sure to get SPI into master mode prior to enabling I/Os
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;  // 1.875MHz (Theoretical max for BLE IC is 3 MHz)
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
  SPI_Init(SPI1, &SPI_InitStructure);
  //SPI_RxFIFOThresholdConfig(SPI1, SPI_RxFIFOThreshold_QF);

  // Set up SPI I/Os for AF mode
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

  // Setup SCK
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_5;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);  //  SPI1
	
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;  // Radio allows MISO to float
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);  //  SPI1

  // Setup RDYN
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_7;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);  //  SPI1
	
  // Set up GPIOs for handshaking
  GPIO_SetBits(GPIOC, GPIOC_REQN);                // Default high (not requesting)
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;   // REQN is an output
  GPIO_InitStructure.GPIO_Pin =  GPIOC_REQN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;    // RDYN is an input
  GPIO_InitStructure.GPIO_Pin =  GPIOA_RDYN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // Enable it, cross your fingers
  SPI_Cmd(SPI1, ENABLE);

  // Finish resetting NRF
  MicroWait(62000);     // NRF datasheet says to wait 62ms

  // Clear g_acibuf
  for (i = 0; i < MAX_ACI_BYTES; i++)
    g_acibuf[i] = 0;
}

// Track whether we are still waiting on Nordic
u8 m_isSendReceiveWaiting = 0;
u8 NeedAnotherSendReceive(void)
{
  return m_isSendReceiveWaiting;
}

// Request ACI service
static void RequestACI()
{
  // Stabilize SCK on correct edge first, then set up RDYN for request
  __disable_irq();
  GPIOA->MODER &= ~(GPIO_MODER_MODER0 << (PINA_SCK*2));
  GPIOA->MODER |= (GPIO_Mode_AF << (PINA_SCK*2));
  __enable_irq();
  GPIO_ResetBits(GPIOC, GPIOC_REQN);
}

// Attempt to exchange buffers with the Nordic
// On a timeout, buffers will not be exchanged
void SendReceiveRadioBuf(void)
{
  volatile u32 v;
  
  // TODO: Create another flag to check if SD is in use.
  //  Set m_isSendReceiveWaiting=1 if it is and return.
  //return;
  
  int i;
  int xferCount = g_acibuf[0];
  
#ifdef ACI_TRACE
  if (xferCount) {
    SlowPutString("\r\nF0->NRF:");
    for (i = 0; i <= xferCount; i++)
      SlowPutInt(g_acibuf[i]);
    SlowPutString("\r\n");
  }
#endif
  
  // Drain anything left in the RX FIFO
  while (SPI1->SR & SPI_SR_RXNE)
  {
    //SlowPutString(" BUG");
    v = SPI1->DR; // Volatile read
  }

  // If we have something to transmit, try that first (possibly going bidirectional)
  if (xferCount)
  {
    xferCount++;    // Include length byte

    // Request attention and await reply (zero on RDYN)
    EnableACI();  
    RequestACI();
    
    // Wait for chip to respond for up NORDIC_TIMEOUT microseconds
    u16 start = getShortMicroCounter();
    while (GPIO_ReadInputData(GPIOA) & GPIOA_RDYN)
    {
      if ((u16)(getShortMicroCounter() - start) > NORDIC_TIMEOUT)
      {
        // Failed to respond in time, unwind the request
        GPIO_SetBits(GPIOC, GPIOC_REQN);
        DisableACI();  
        m_isSendReceiveWaiting = 1;    
        SlowPutString(" TIMEOUT");
        return;
      }
    }
    m_isSendReceiveWaiting = 0;
  // If we have nothing to transmit, see if we have anything to receive
  } else {
    // Wipe out whatever was there before
    for (i = 0; i < MAX_ACI_BYTES; i++)
      g_acibuf[i] = 0;
                             
    // If not ready for data, bail out
    EnableACI();  
    if (GPIO_ReadInputData(GPIOA) & GPIOA_RDYN) {
      DisableACI();
      return;
    }

    // If ready, set up request line
    RequestACI();    
    xferCount = 2;
  }
  
  // Can safely repurpose RDYN now (no longer scanning it)
  __disable_irq();
  GPIOA->MODER |= (GPIO_Mode_AF << (PINA_RDYN*2));  
  __enable_irq();
  
  // Send data while receiving reply
  for (i = 0; i < MAX_ACI_BYTES && xferCount; i++)
  {
    while (!(SPI1->SR & SPI_SR_TXE))     // Wait for TXFIFO to hit zero before sending
      ; // SlowPutChar('w');
    SPI_SendData(SPI1, g_acibuf[i]);
    while (!(SPI1->SR & SPI_SR_RXNE))    // Wait for RXFIFO to show something before receiving is safe
      ; // SlowPutChar('r');
    g_acibuf[i] = SPI1->DR;

    xferCount--;

    // After the second byte received, we may have to boost xferCount to match the Nordic's requirements
    if (1 == i && g_acibuf[1] > xferCount)
    {
      xferCount = g_acibuf[1];
    }
  }

  // Release request line, demultiplex things
  GPIO_SetBits(GPIOC, GPIOC_REQN);
  DisableACI();  
  
  // If we received anything, debug print it
  if (g_acibuf[1]) 
  {
#ifdef ACI_TRACE
    SlowPutString("\r\nNRF->F0:");
    SlowPutLong((getMicroCounter() / 1000) & 4095);
    for (i = 0; i < g_acibuf[1] + 2; i++)
      SlowPutInt(g_acibuf[i]);
    SlowPutString("\r\n");
#endif
  }

  // Wipe "NRF debug byte"
  g_acibuf[0] = 0;
}
