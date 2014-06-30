// Hanns needed a simple scripting language to test eyes and motors
// Please replace this with a proper test mode later in the project!
#include "lib/stm32f4xx.h"
#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {     
      void HALTestMode()
      {
        while(1)
        {
          // Lights are 0-23, motors are 24-27
          int start[28];
          int step[28];
          int ms[28];
          
          // Stop motors and wait
          memset(start, 0, sizeof(start));
          memset(step, 0, sizeof(step));
          memset(ms, 0, sizeof(ms));          
          printf("\r\n\r\nREADY\r\n");
          
          // Update and run the program
          int reg = 0, m = 0, p1 = 0, p2 = 0, neg = 1, comma = 0, wait = 0;     
          while(1)
          {
            // Update all registers that are active
            for (int i = 0; i < 28; i++)
              if (ms[i])
              {
                ms[i]--;
                start[i] += step[i];
              }
            // Set LEDs to register values
            for (int i = 0; i < 8; i++)
              SetLED((LEDId)i, (LEDColor)( (0xff & (start[i] >> 16)) |
                (0xff00 & (start[i+8] >> 8)) | 
                (0xff0000 & (start[i+16]))));
            // Set motors to register values
            for (int i = 0; i < 4; i++) {
              s8 power = (start[i+24] >> 16);
              MotorSetPower((MotorID)i, power / 100.0);
            }
            
            // Wait 1ms at a time
            MicroWait(1000);
            if (wait > 0) {
              wait--;
              continue;
            }
                        
            // Grab a char, force everything to lowercase
            int c;
            while (-1 != (c = HAL::UARTGetChar(0)))
            {
              if (c >= 'A' && c <= 'Z')
                c += 'z'-'Z';
              
              // At first sign of a control character, parse the input
              if (c < 33)
              {
                int which = 0, howmany = 1;
                printf(" ");
                if (c == 10 || c == 13)
                  printf("\r\n");

                // See if it's part 2 after the comma
                reg = neg*reg;
                neg = 1;
                if (comma) {
                  p2 = reg;
                  reg = 0;
                  comma = 0;
                }
                
                // Figure out which iterator to reload
                if (reg == 0x1ead)
                  which = 24 + MOTOR_HEAD;
                else if (reg == 0x52fd)
                  which = 24 + MOTOR_LIFT;
                else if (reg == 0x5efd)
                  which = 24 + MOTOR_LEFT_WHEEL;
                else if (reg == 0xb201d)
                  which = 24 + MOTOR_RIGHT_WHEEL;
                else if (0xb0 == (reg & ~0xf) || 0x50 == (reg & ~0xf)) {
                  if (0xe == (reg & 0xf))
                    which = LED_RIGHT_EYE_TOP;
                  else if (0xd == (reg & 0xf))
                    which = LED_RIGHT_EYE_BOTTOM;
                  else if (0x5 == (reg & 0xf))
                    which = LED_RIGHT_EYE_LEFT;
                  else if (0xb == (reg & 0xf))
                    which = LED_RIGHT_EYE_RIGHT;
                  else if (0xa == (reg & 0xf)) {
                    which = LED_RIGHT_EYE_TOP;
                    howmany = 4;
                  } else
                    which = reg & 0xf;
                  if (0x50 == (reg & ~0xf))
                    which += (LED_LEFT_EYE_TOP - LED_RIGHT_EYE_TOP);
                } else if (0 == reg) {
                  // Do nothing
                  continue;
                } else {
                  printf("\r\nSyntax error\r\n");
                  reg = 0;
                  continue;
                }
                
                // Load the specified iterators
                int pp1 = p1, pp2 = p2;
                do {
                  for (int i = 0; i < howmany; i++)
                  {
                    ms[which+i] = m;
                    start[which+i] = (pp1&255) << 16;
                    if (which < 24)
                      step[which+i] = (((pp2&255)-(pp1&255)) << 16) / m;
                    else
                      step[which+i] = 0;
                  }
                  which += 8;
                  pp1 >>= 8;
                  pp2 >>= 8;
                } while (which < 24);
                
                reg = 0;
              } else {
                if (c == 'x') {
                  // Stop motors!
                  for (int i = 24; i < 28; i++)
                    start[i] = step[i] = ms[i] = 0;                  
                  reg = 0;
                  neg = 1;
                } else if (c == 'm') {
                  m = reg;
                  reg = 0;
                  neg = 1;
                } else if (c == 'w') {
                  wait = reg;
                  reg = 0;
                  neg = 1;
                  printf("w");
                  break;
                } else if (c == 'p') {
                  p1 = neg*((reg & 15) + (reg >> 4)*10);
                  reg = 0;
                  neg = 1;
                } else if (c == ',') {
                  p1 = neg*reg;
                  reg = 0;
                  neg = 1;
                  comma = 1;
                } else if (c == '-') {
                  neg = -1;
                } else if (c >= '0' && c <= '9') {
                  reg = (reg << 4) | ((c - '0') & 15);
                } else if (c >= 'a' && c <= 'z') {
                  reg = (reg << 4) | ((c - 'a' + 10) & 15);
                } else {
                  continue;
                }
                printf("%c", c);
              }
            }
          }
        }
      }
    }
  }
}
