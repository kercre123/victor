#include <rt_misc.h>

// We want to use our own UART methods in place of semihosting with the debugger
#pragma import(__use_no_semihosting_swi)

extern "C"
{  
  struct FILE{ int handle; };

  FILE __stdout;
  FILE __stdin;
  FILE __stderr;

  int ferror(FILE *f)
  {
    return 0;
  }

  void _ttywrch(int c)
  {
    //Anki::Cozmo::HAL::UARTPutChar(c);
  }

  void _sys_exit(int return_code)
  {
  label:
    goto label;  // Endless loop
  }
}
