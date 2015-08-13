/*
 */
 
// Standard C Included Files
#include <stdio.h>
#include <stdlib.h>

// SDK Included Files
#include "board.h"
#include "fsl_debug_console.h"

#include "uart.h"
#include "oled.h"
#include "spi.h"

// Use PTA4 / FTM0_CH1 / pin 10 on eval board because it is otherwise unused
void InitPWM(int period)
{
    // Enable FTM0 clock
    SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;
    
    //setup pwm module
    FTM0_SC = 0;        // Turn off timer so we can change it
    FTM0_MOD = period-1; //set timer period
    FTM0_C1SC = FTM_CnSC_ELSB_MASK|FTM_CnSC_MSB_MASK; //edge-aligned pwm
    FTM0_C1V = period/2;    // 50% duty cycle
    FTM0_SC = FTM_SC_CLKS(1); // Use bus clock with a /1 prescaler

    // Mux FTM0_CH1 onto PTA4
    PORTA_PCR4   = PORT_PCR_MUX(3); // Alt 3 
    
    FTM0_CNT = 0; //make sure flag is clear when dma requests are enabled
    FTM0_C1SC &= ~FTM_CnSC_CHF_MASK; //channel flag must be read and then written 0 to clear
}

void DMAFill(char* dest)
{
    volatile char test[256];
    
    SIM_SCGC6 |= SIM_SCGC6_DMAMUX_MASK;
    SIM_SCGC7 |= SIM_SCGC7_DMA_MASK;
    
    // Give DMA max priority on the bus - 0 means fixed priority, WTF docs?
    MCM_PLACR = 0; // MCM_PLACR_ARB_MASK;
    
    DMA_CR = DMA_CR_CLM_MASK;  // Continuous loop mode? (Makes no difference?)
    
    DMA_TCD0_CSR = 0; //clears done flag and dreq bit
    DMA_TCD0_NBYTES_MLNO = 256; //number of transfers
    // For some reason 32-bit source doesn't work - need to do 8-bit source!
    DMA_TCD0_ATTR = DMA_ATTR_SSIZE(0)|DMA_ATTR_DSIZE(0); // source 8-bit, dest 8-bit
    DMA_TCD0_SOFF = 0; // source doesn't increment
    DMA_TCD0_SADDR = (uint32_t)&GPIOA_PDIR;
    DMA_TCD0_DOFF = 1; //destination address increments
    DMA_TCD0_DADDR = (uint32_t)dest; //destination address
    DMA_TCD0_CITER_ELINKNO = 1; //current major loop iteration for 1 transfer
    DMA_TCD0_BITER_ELINKNO = 1; //beginning major loop iterations, 1 transfer only
    DMA_TCD0_DLASTSGA = 0; //just to make sure
    DMA_TCD0_CSR = DMA_CSR_START_MASK;
    
    // DO NOT TOUCH DMA REGISTERS!  It definitely slows down fast capture...
    //while (!(DMA_TCD0_CSR & DMA_CSR_DONE_MASK)) {} // wait until completed
        
    int x = 10000;    // Really only need 512 CPU cycles or so
    while (x--)
      ;
       // *((volatile int*)0x1FFFFF00);
}

/*!
 * @brief Main function
 */
int main (void)
{
  char dest[256];
    
  // Initialize standard SDK demo application pins
  hardware_init();
  
  // Call this function to initialize the console UART. This function
  // enables the use of STDIO functions (printf, scanf, etc.)
  dbg_uart_init();

  PRINTF("\r\nTesting self-contained project file.\n\n\r");

  spi_init();
  i2c_init();
  uart_init();

  int x = 0;
  while (true) 
  {
      while (x < 30)
      {
          if (x < 30) {
              x++;
              InitPWM(x);
          }
          PRINTF("%2d: ", x);
          DMAFill(dest);
          int first1 = 0, first0 = 0, cols = 160;
          for (int i = 0; i < 256 && cols > 0; i++)
          {
              int c = dest[i] & 16;
              first1 |= c;
              if (first1)
                  first0 |= (!c);
              if (first0 && first1) {
                  PRINTF(c ? "1" : ".");
                  cols--;
              }
          }
          PRINTF("\n");
      }
    }
}
