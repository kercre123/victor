// Contains all the pin definitions for the board we're running on
#ifndef BOARD_H__
#define BOARD_H__

// EP3 board pinout
// Will change for FEP

#define PWR_IDX 7
#define PWR_BIT (1<<PWR_IDX)

#define LEDPORT P0
#define LEDDIR  P0DIR
#define LEDCON  P0CON

// Lights must be on P0:  See lights.c if you have to change this or the charlieplexing order
#ifndef EP3
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
#endif

// Support EP3 for a little while
#ifdef EP3
#define LED0 (1<<1)
#define LED1 (1<<0)
#define LED2 (1<<3)
#define LED3 (1<<2)
#define LED4 (1<<5)
#define LED5 (1<<6)

#warning XXX: EP3 chargers have an incompatible layout
#define LEC0 (1<<0)
#define LEC1 (1<<1)
#define LEC2 (1<<2)
#define LEC3 (1<<3)
#define LEC4 (1<<6)
#define LEC5 (1<<7)   

#define DriveSCL(x)   P11 = x     // P1.1
#define ReadSDA()     P10         // P1.0
#define DriveSDA(x)   do { if (x) P1DIR = 0xFF-2; else P1DIR = 0xFF-2-1; } while(0) // 1=float, 0=drive
#define I2CInit()     { P1CON = PULL_UP | 0;    /* Pull up SDA (there isn't one on board */ \
                        P10 = 0;                /* Default SDA low when driven */ \
                        P0DIR = 0xFF-2; }       /* Default SCL driven */
#endif

// Default direction for LEDs
#define DEFDIR  255

#endif
