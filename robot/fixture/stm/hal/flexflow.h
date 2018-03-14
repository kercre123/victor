#ifndef __FLEXFLOW_H
#define __FLEXFLOW_H

#include <stdint.h>

//-----------------------------------------------------------
//        API
//-----------------------------------------------------------

#ifdef __cplusplus
namespace FLEXFLOW
{
  void init(int baud = 3000000);
  void deinit();
  
  int  printf(const char * format, ... );
  void write(const char *s);
  void putchar(char c);
}
#endif /* __cplusplus */

#endif //__FLEXFLOW_H

