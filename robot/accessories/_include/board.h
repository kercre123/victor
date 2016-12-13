// Contains all the pin definitions for the board we're running on
#ifndef BOARD_H__
#define BOARD_H__

// Production pinout
#define PWR_IDX 7
#define PWR_BIT (1<<PWR_IDX)
// 255=3.6V, 106=1.5V, 71=1.0V - 3.6/255
#define VBAT_ADC_CHAN 4

#define LEDPORT P0
#define LEDDIR  P0DIR
#define LEDCON  P0CON

// Lights must be on P0:  See lights.c if you have to change this or the charlieplexing order
#define LED0 (1<<1)
#define LED1 (1<<0)
#define LED2 (1<<6)   
#define LED3 (1<<2)
#define LED4 (1<<5)
#define LED5 (1<<3)

// Production chargers are wired just a LITTLE differently (LED5 and LED2 are swapped)
#define LEC0 (1<<1)
#define LEC1 (1<<0)
#define LEC2 (1<<3)
#define LEC3 (1<<2)
#define LEC4 (1<<5)
#define LEC5 (1<<6)   

// SPI bus
#define CSB   P11
#define SCK   P01
#define SDI   P03
#define SPI_PINS (2+8)  // SCK and SDI
#define SPIInit() { CSB = 1; P1DIR = 255-2;               /* CSB defaults high and driven */ \
                    SCK = 1; SDI = 1; P0DIR = 0xFF-2-8; } /* SCK and SDI default high and driven */
#define SPIRead() P0DIR = 0xFF-2;           // Just drive SCK, let SDI be an input

// Default direction for LEDs
#define DEFDIR  255

#endif
