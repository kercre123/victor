#ifndef HAL_CONSOLE_H
#define HAL_CONSOLE_H

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------
//empty

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

void console_init(bool echo); //echo rx'd chars on the uart
void console_echo(bool on); //echo on/off

//process uart rx to internal buffer (must poll to prevent overflows)
//@return ptr to complete \0-term string once \n is found. NULL if incomplete line.
char* console_getline(int* out_len);

void console_write(char* s);
int  console_printf(const char* format, ...);

//-----------------------------------------------------------
//        Line Parsing
//-----------------------------------------------------------

//get n-th argument in the string (copy to buf, \0-terminated).
//args are whitespace delimited, unless enclosed by "".
//@return 'buf' if success. NULL if !exist.
char* console_getarg(char *s, int n, char* buf, int buflen);
char* console_getargl(char *s, int n); //^^getarg, using internal static buffer (valid until next getargl call)

//@return # of args in the string
int console_num_args(char *s);

#endif //HAL_CONSOLE_H

