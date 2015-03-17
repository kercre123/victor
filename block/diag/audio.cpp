/**
* File: audio.cpp
*
* Author: Bryan Hood
* Created: 4/26/2014
*
*
* Description:
*
*    Plays a sine wave and sweep
*
**/

#include "anki/cozmo/robot/hal.h"
#include "hal/portable.h"
#include "lib/stm32f4xx.h"

//#define TESTPOINT


#define VOLUME      0.05f

const uint16_t sine[512] = {0x0000, 0x0192, 0x0324, 0x04B6, 0x0648, 0x07D9, 0x096A, 0x0AFB, 0x0C8C, 0x0E1C, 0x0FAB, 0x113A, 0x12C8, 0x1455, 0x15E2, 0x176E,
                            0x18F9, 0x1A82, 0x1C0B, 0x1D93, 0x1F1A, 0x209F, 0x2223, 0x23A6, 0x2528, 0x26A8, 0x2826, 0x29A3, 0x2B1F, 0x2C99, 0x2E11, 0x2F87,
                            0x30FB, 0x326E, 0x33DF, 0x354D, 0x36BA, 0x3824, 0x398C, 0x3AF2, 0x3C56, 0x3DB8, 0x3F17, 0x4073, 0x41CE, 0x4325, 0x447A, 0x45CD,
                            0x471C, 0x4869, 0x49B4, 0x4AFB, 0x4C3F, 0x4D81, 0x4EBF, 0x4FFB, 0x5133, 0x5268, 0x539B, 0x54C9, 0x55F5, 0x571D, 0x5842, 0x5964,  
                            0x5A82, 0x5B9C, 0x5CB3, 0x5DC7, 0x5ED7, 0x5FE3, 0x60EB, 0x61F0, 0x62F1, 0x63EE, 0x64E8, 0x65DD, 0x66CF, 0x67BC, 0x68A6, 0x698B,
                            0x6A6D, 0x6B4A, 0x6C23, 0x6CF8, 0x6DC9, 0x6E96, 0x6F5E, 0x7022, 0x70E2, 0x719D, 0x7254, 0x7307, 0x73B5, 0x745F, 0x7504, 0x75A5,
                            0x7641, 0x76D8, 0x776B, 0x77FA, 0x7884, 0x7909, 0x7989, 0x7A05, 0x7A7C, 0x7AEE, 0x7B5C, 0x7BC5, 0x7C29, 0x7C88, 0x7CE3, 0x7D39,
                            0x7D89, 0x7DD5, 0x7E1D, 0x7E5F, 0x7E9C, 0x7ED5, 0x7F09, 0x7F37, 0x7F61, 0x7F86, 0x7FA6, 0x7FC1, 0x7FD8, 0x7FE9, 0x7FF5, 0x7FFD,
                            0x7FFF, 0x7FFD, 0x7FF5, 0x7FE9, 0x7FD8, 0x7FC1, 0x7FA6, 0x7F86, 0x7F61, 0x7F37, 0x7F09, 0x7ED5, 0x7E9C, 0x7E5F, 0x7E1D, 0x7DD5,
                            0x7D89, 0x7D39, 0x7CE3, 0x7C88, 0x7C29, 0x7BC5, 0x7B5C, 0x7AEE, 0x7A7C, 0x7A05, 0x7989, 0x7909, 0x7884, 0x77FA, 0x776B, 0x76D8,
                            0x7641, 0x75A5, 0x7504, 0x745F, 0x73B5, 0x7307, 0x7254, 0x719D, 0x70E2, 0x7022, 0x6F5E, 0x6E96, 0x6DC9, 0x6CF8, 0x6C23, 0x6B4A,
                            0x6A6D, 0x698B, 0x68A6, 0x67BC, 0x66CF, 0x65DD, 0x64E8, 0x63EE, 0x62F1, 0x61F0, 0x60EB, 0x5FE3, 0x5ED7, 0x5DC7, 0x5CB3, 0x5B9C,
                            0x5A82, 0x5964, 0x5842, 0x571D, 0x55F5, 0x54C9, 0x539B, 0x5268, 0x5133, 0x4FFB, 0x4EBF, 0x4D81, 0x4C3F, 0x4AFB, 0x49B4, 0x4869,
                            0x471C, 0x45CD, 0x447A, 0x4325, 0x41CE, 0x4073, 0x3F17, 0x3DB8, 0x3C56, 0x3AF2, 0x398C, 0x3824, 0x36BA, 0x354D, 0x33DF, 0x326E,
                            0x30FB, 0x2F87, 0x2E11, 0x2C99, 0x2B1F, 0x29A3, 0x2826, 0x26A8, 0x2528, 0x23A6, 0x2223, 0x209F, 0x1F1A, 0x1D93, 0x1C0B, 0x1A82,
                            0x18F9, 0x176E, 0x15E2, 0x1455, 0x12C8, 0x113A, 0x0FAB, 0x0E1C, 0x0C8C, 0x0AFB, 0x096A, 0x07D9, 0x0648, 0x04B6, 0x0324, 0x0192,
                            0x0000, 0xFE6E, 0xFCDC, 0xFB4A, 0xF9B8, 0xF827, 0xF696, 0xF505, 0xF374, 0xF1E4, 0xF055, 0xEEC6, 0xED38, 0xEBAB, 0xEA1E, 0xE892,
                            0xE707, 0xE57E, 0xE3F5, 0xE26D, 0xE0E6, 0xDF61, 0xDDDD, 0xDC5A, 0xDAD8, 0xD958, 0xD7DA, 0xD65D, 0xD4E1, 0xD367, 0xD1EF, 0xD079,
                            0xCF05, 0xCD92, 0xCC21, 0xCAB3, 0xC946, 0xC7DC, 0xC674, 0xC50E, 0xC3AA, 0xC248, 0xC0E9, 0xBF8D, 0xBE32, 0xBCDB, 0xBB86, 0xBA33,
                            0xB8E4, 0xB797, 0xB64C, 0xB505, 0xB3C1, 0xB27F, 0xB141, 0xB005, 0xAECD, 0xAD98, 0xAC65, 0xAB37, 0xAA0B, 0xA8E3, 0xA7BE, 0xA69C,
                            0xA57E, 0xA464, 0xA34D, 0xA239, 0xA129, 0xA01D, 0x9F15, 0x9E10, 0x9D0F, 0x9C12, 0x9B18, 0x9A23, 0x9931, 0x9844, 0x975A, 0x9675,
                            0x9593, 0x94B6, 0x93DD, 0x9308, 0x9237, 0x916A, 0x90A2, 0x8FDE, 0x8F1E, 0x8E63, 0x8DAC, 0x8CF9, 0x8C4B, 0x8BA1, 0x8AFC, 0x8A5B,
                            0x89BF, 0x8928, 0x8895, 0x8806, 0x877C, 0x86F7, 0x8677, 0x85FB, 0x8584, 0x8512, 0x84A4, 0x843B, 0x83D7, 0x8378, 0x831D, 0x82C7,
                            0x8277, 0x822B, 0x81E3, 0x81A1, 0x8164, 0x812B, 0x80F7, 0x80C9, 0x809F, 0x807A, 0x805A, 0x803F, 0x8028, 0x8017, 0x800B, 0x8003,
                            0x8001, 0x8003, 0x800B, 0x8017, 0x8028, 0x803F, 0x805A, 0x807A, 0x809F, 0x80C9, 0x80F7, 0x812B, 0x8164, 0x81A1, 0x81E3, 0x822B,
                            0x8277, 0x82C7, 0x831D, 0x8378, 0x83D7, 0x843B, 0x84A4, 0x8512, 0x8584, 0x85FB, 0x8677, 0x86F7, 0x877C, 0x8806, 0x8895, 0x8928,
                            0x89BF, 0x8A5B, 0x8AFC, 0x8BA1, 0x8C4B, 0x8CF9, 0x8DAC, 0x8E63, 0x8F1E, 0x8FDE, 0x90A2, 0x916A, 0x9237, 0x9308, 0x93DD, 0x94B6,
                            0x9593, 0x9675, 0x975A, 0x9844, 0x9931, 0x9A23, 0x9B18, 0x9C12, 0x9D0F, 0x9E10, 0x9F15, 0xA01D, 0xA129, 0xA239, 0xA34D, 0xA464,
                            0xA57E, 0xA69C, 0xA7BE, 0xA8E3, 0xAA0B, 0xAB37, 0xAC65, 0xAD98, 0xAECD, 0xB005, 0xB141, 0xB27F, 0xB3C1, 0xB505, 0xB64C, 0xB797,
                            0xB8E4, 0xBA33, 0xBB86, 0xBCDB, 0xBE32, 0xBF8D, 0xC0E9, 0xC248, 0xC3AA, 0xC50E, 0xC674, 0xC7DC, 0xC946, 0xCAB3, 0xCC21, 0xCD92,
                            0xCF05, 0xD079, 0xD1EF, 0xD367, 0xD4E1, 0xD65D, 0xD7DA, 0xD958, 0xDAD8, 0xDC5A, 0xDDDD, 0xDF61, 0xE0E6, 0xE26D, 0xE3F5, 0xE57E,
                            0xE707, 0xE892, 0xEA1E, 0xEBAB, 0xED38, 0xEEC6, 0xF055, 0xF1E4, 0xF374, 0xF505, 0xF696, 0xF827, 0xF9B8, 0xFB4A, 0xFCDC, 0xFE6E}; 




namespace Anki
{
  namespace Cozmo
  {
    namespace DIAG_HAL
    {
      // Define SPI pins with macros
      #ifndef  TESTPOINT
      GPIO_PIN_SOURCE(AUDIO_WS, GPIOB, 12);
      GPIO_PIN_SOURCE(AUDIO_CK, GPIOB, 13);
      GPIO_PIN_SOURCE(AUDIO_SD, GPIOB, 15);
      #else  
      GPIO_PIN_SOURCE(AUDIO_WS, GPIOB, 9);
      GPIO_PIN_SOURCE(AUDIO_CK, GPIOI, 1);
      GPIO_PIN_SOURCE(AUDIO_SD, GPIOI, 3);
      #endif


      // Initialize I2S (I2S2)
      void AudioInit()
      {
        // Enable peripheral clock
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
        
        // Enable  GPIO clocks
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);  
        #ifdef  TESTPOINT
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOI, ENABLE);    // test points
        #endif
        // Peripherals alternate function
        GPIO_PinAFConfig(GPIO_AUDIO_CK, SOURCE_AUDIO_CK, GPIO_AF_SPI2);
        GPIO_PinAFConfig(GPIO_AUDIO_WS, SOURCE_AUDIO_WS, GPIO_AF_SPI2);
        GPIO_PinAFConfig(GPIO_AUDIO_SD, SOURCE_AUDIO_SD, GPIO_AF_SPI2);

        // Initalize Pins
        GPIO_InitTypeDef GPIO_InitStructure;
        // Set SPI alternate function pins
        
        #ifndef  TESTPOINT
        GPIO_InitStructure.GPIO_Pin = PIN_AUDIO_WS | PIN_AUDIO_CK | PIN_AUDIO_SD;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_AUDIO_CK, &GPIO_InitStructure);  // GPIOB
        #else 
        GPIO_InitStructure.GPIO_Pin = PIN_AUDIO_WS;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_AUDIO_WS, &GPIO_InitStructure);  // GPIOB
        
        GPIO_InitStructure.GPIO_Pin = PIN_AUDIO_CK | PIN_AUDIO_SD;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
        GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
        GPIO_Init(GPIO_AUDIO_CK, &GPIO_InitStructure);  // GPIOI
        #endif

        I2S_InitTypeDef I2S_InitStructure;
        I2S_InitStructure.I2S_Mode = I2S_Mode_MasterTx;
        I2S_InitStructure.I2S_Standard = I2S_Standard_Phillips;
        I2S_InitStructure.I2S_DataFormat = I2S_DataFormat_32b;
        I2S_InitStructure.I2S_MCLKOutput = I2S_MCLKOutput_Disable;
        I2S_InitStructure.I2S_AudioFreq = ((uint32_t)68000); // 44.1k XXX (something is wrong here. clock isn't right)
        I2S_InitStructure.I2S_CPOL = I2S_CPOL_Low;

        I2S_Init(SPI2, &I2S_InitStructure);
        
        RCC_I2SCLKConfig(RCC_I2S2CLKSource_PLLI2S);
        RCC_PLLI2SCmd(ENABLE);
        RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY);
        
        
        // Enable the SPI
        I2S_Cmd(SPI2, ENABLE);
      }


      void Sweep() // 3 second sweep
      {
        while(1)
        {
          for(int32_t i = 0; i < 132300; i++) // 3 seconds at 44.1k
          {
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(10000*sin(0.00000010769f*i*i+0.0047492f*i)) & 0xFFFF);
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(10000*sin(0.00000010769f*i*i+0.0047492f*i)) & 0xFFFF);
          }
        }
      }


      void SineHigh() // one period, 1000 Hz
      {
        while(1)
        {
          for(int32_t i = 0; i < 44; i++) // 3 seconds at 44.1k
          {
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(5000*sin(0.142799666f*i)) & 0xFFFF);
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(5000*sin(0.142799666f*i)) & 0xFFFF);
          }
        }
      }
      
      
      void SineLow() // one period, 200 Hz
      {

          for(int32_t i = 0; i < 221; i++) // 3 seconds at 44.1k
          {
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(20000*sin(0.028430702f*i)) & 0xFFFF);
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(20000*sin(0.028430702f*i)) & 0xFFFF);
          }

      }


      void MakeNoise()  // obsolete test code
      {
        uint16_t freq1, freq2;
        freq1 = 100;
        freq2 = 1000;
        while(1)
        {
          for(int32_t i = 0; i < 132300; i++) // 3 seconds at 44.1k
          {
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            //SPI_I2S_SendData(SPI2, (int)(VOLUME*sin(2*3.14159256f*i*((freq2-freq1)*i/132300+freq1))) & 0xFFFF);
            SPI_I2S_SendData(SPI2, (int)(10000*sin(0.00000010769f*i*i+0.0047492f*i)) & 0xFFFF);
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(10000*sin(0.00000010769f*i*i+0.0047492f*i)) & 0xFFFF);
            //printf("%x ",  (int)(VOLUME*(s16)sin((float)((0.0427428f*i)+628.32f))) & 0xFFFF);
            //printf("%d ", (int)(2000*sin(((0.0427428f*i)+628.32f))));
          }
          
        /* // working
          for(uint16_t i = 0; i <512; i++)
          {
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(VOLUME*(s16)sine[i]) & 0xFFFF);
         */ //working
            /*
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(VOLUME*(s16)sine[i]) & 0xFFFF);
                        // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(VOLUME*(s16)sine[i]) & 0xFFFF);
                        // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, (int)(VOLUME*(s16)sine[i]) & 0xFFFF);
            */
          
            /*
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, 0x000);
            
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, 0x000);
            
            // Wait until TXE = 1 (wait until transmit buffer is empty)
            while(!(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE)))
            {
            }
            SPI_I2S_SendData(SPI2, 0x000);
            */



            //printf("%x ", (int)(VOLUME*(s16)sine[i]) & 0xFFFF);
            /* //working
          }
          */ // working
        }
      }
      
   
    }
  }
}
