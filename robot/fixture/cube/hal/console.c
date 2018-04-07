#include <stdarg.h>
#include <stdio.h>
#include "board.h"
#include "console.h"
#include "hal_uart.h"

const int console_len_max = 55;
static char console_buf[console_len_max + 1];
static int console_len = 0;
static bool m_console_echo = 0;

//------------------------------------------------  
//    processing
//------------------------------------------------

//return ptr to full line, when available
char* console_process_char_(char c, int* out_len)
{
  switch(c)
  {
    case '\r': //Return, EOL
    case '\n':
      if( m_console_echo )
        hal_uart_putchar('\n');
      if(console_len)
      {
        console_buf[console_len] = '\0';
        if(out_len)
          *out_len = console_len;
        console_len = 0;
        return console_buf;
      }
      break;
    
    case 0x08: //Backspace
      if(console_len)
      {
        console_len--;
        if( m_console_echo ) {
          hal_uart_putchar(0x08);
          hal_uart_putchar(' '); //overwrite terminal char with a space
          hal_uart_putchar(0x08);
        }
      }
      break;
    
    default:
      if( c >= ' ' && c <= '~' ) //printable char set
      {
        if( console_len < console_len_max ) {
          console_buf[console_len++] = c;
          if( m_console_echo )
            hal_uart_putchar(c);
        }
      }
      //else, ignore all other chars
      break;
  }
  return NULL;
}

//------------------------------------------------  
//    console
//------------------------------------------------

static bool inited = 0;
void console_init(bool echo)
{
  if( inited )
    return;
  
  m_console_echo = echo;
  hal_uart_init();
  inited = 1;
}

void console_echo(bool on)
{
  m_console_echo = on;
}

char* console_getline(int* out_len)
{
  if( inited ) {
    int c = hal_uart_getchar();
    if( c > -1 )
      return console_process_char_(c,out_len);
  }
  return NULL;
}

void console_write(char* s) {
  hal_uart_write(s);
}

int console_printf(const char* format, ...)
{
  char dest[128];
  va_list argptr;
  
  va_start(argptr, format);
  int len = vsnprintf(dest, sizeof(dest), format, argptr);
  va_end(argptr);
  
  hal_uart_write(dest);
  return len > sizeof(dest) ? sizeof(dest) : len;
}

//-----------------------------------------------------------
//        Line Parsing
//-----------------------------------------------------------
//Note: valid command strings must guarantee no \r \n chars and one valid \0 terminator

static inline bool is_whitespace_(char c) {
  return c <= ' ' || c > '~'; //space or non-printable char
}

//seek to start of next argument.
static char* next_arg_(char* s)
{
  //started on a quoted arg. seek to end of quotes
  bool inQuotes = *s == '"';
  while( inQuotes ) {
    if( *++s == NULL )
      return NULL;
    if( *s == '"' )
      break;
  }
  
  //started on normal arg (or closing quote from above). seek next whitespace
  while( !is_whitespace_(*s) ) {
    if( *++s == NULL )
      return NULL;
  }
  
  //finally, seek start of next arg
  while( is_whitespace_(*s) ) {
    if( *++s == NULL )
      return NULL;
  }
  
  return s; //now pointing to first char of next arg (maybe an opening ")
}

char* console_getarg(char *s, int n, char* buf, int buflen)
{
  if( !s || !buf )
    return NULL;
  
  if( is_whitespace_(*s) ) //need initial seek to start from 0th arg
    n++;
  while(n--) { //seek to nth arg
    if( (s = next_arg_(s)) == NULL )
      return NULL; //there is no spoon
  }
  
  //find arg length
  int len = 0;
  bool inQuotes = *s == '"';
  if( inQuotes ) {
    char* temp = ++s;
    while(1) {
      if(*temp == '"' || *temp == NULL)
        break;
      len++; temp++;
    }
  } 
  else { //normal arg
    char* temp = s;
    do { 
      len++;
    } while(*++temp != NULL && !is_whitespace_(*temp) );
  }
  
  //copy to output buffer
  len = len > buflen-1 ? buflen-1 : len; //limit to buffer size
  memcpy(buf, s, len );
  buf[len] = '\0';
  return buf;
}

char* console_getargl(char *s, int n)
{
  const int bufsize = 39;
  static char buf[bufsize+1];
  return console_getarg(s, n, buf, sizeof(buf));
}

int console_num_args(char *s)
{
  if(!s)
    return 0;
  
  int n = is_whitespace_(*s) ? 0 : 1;
  while( (s = next_arg_(s)) != NULL )
    n++;
  return n;
}

