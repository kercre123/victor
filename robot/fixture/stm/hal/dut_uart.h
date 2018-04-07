#ifndef __DUT_UART_H
#define __DUT_UART_H

#include <stdint.h>

//-----------------------------------------------------------
//        API
//-----------------------------------------------------------

#ifdef __cplusplus
namespace DUT_UART
{
  void init(int baud = 57600);
  void deinit();
  
  int  printf(const char * format, ... );
  void write(const char *s);
  void putchar(char c);
  
  //@param out_len optionally stores # chars read into buf, even if EOL not found
  //@return 'buf' if EOL found, NULL if timeout or bufsize exceeded
  char* getline(char* buf, int bufsize, int timeout_us = 20*1000, int *out_len = 0);
  int   getchar(int timeout_us = 0);
  
  int getRxDroppedChars();   //get and clear count of dropped rx chars
  int getRxOverflowErrors(); //get and clear count of receive buffer overflows (uart periph)
  int getRxFramingErrors();  //get and clear count of receive framing errors
}
#endif /* __cplusplus */

#endif //__DUT_UART_H

