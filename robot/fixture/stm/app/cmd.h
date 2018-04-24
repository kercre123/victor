#ifndef CMD_H
#define CMD_H

#include <stdint.h>
#include <limits.h>

//-----------------------------------------------------------------------------
//                  Master/Send
//-----------------------------------------------------------------------------

//parameterized command/response delimiters (added/removed internally)
#define CMD_PREFIX        ">>"
#define RSP_PREFIX        "<<"
#define ASYNC_PREFIX      ":"
#define LOG_CMD_PREFIX    ">"
#define LOG_RSP_PREFIX    "<"

//io channels - Helper head vs DUT uarts etc.
enum cmd_io_e {
  CMD_IO_SIMULATOR  = 0,
  CMD_IO_HELPER     = 1,
  CMD_IO_CONSOLE    = CMD_IO_HELPER,
  CMD_IO_DUT_UART   = 2,
  CMD_IO_CONTACTS   = 3, //Charge Contact comms
};
typedef enum cmd_io_e cmd_io;

#define CMD_DEFAULT_TIMEOUT           100

//option flags
#define CMD_OPTS_EXCEPTION_EN         0x0001  //enable exceptions
#define CMD_OPTS_REQUIRE_STATUS_CODE  0x0002  //missing status code is an error
#define CMD_OPTS_ALLOW_STATUS_ERRS    0x0004  //status!=0 not considered an error
#define CMD_OPTS_LOG_CMD              0x0010  //print-log cmd line (>>cmd...)
#define CMD_OPTS_LOG_RSP              0x0020  //print-log rsp line (<<cmd...)
#define CMD_OPTS_LOG_RSP_TIME         0x0040  //print-log append elapsed time to logged rsp line
#define CMD_OPTS_LOG_ASYNC            0x0080  //print-log async line (:async)
#define CMD_OPTS_LOG_OTHER            0x0100  //print-log 'other' rx'd line (informational,uncategorized)
#define CMD_OPTS_LOG_ERRORS           0x0200  //print-log extra error info
#define CMD_OPTS_LOG_ALL              0x03F0  //print-log all
#define CMD_OPTS_DBG_PRINT_ENTRY      0x1000  //debug: print function entry with parsed params
#define CMD_OPTS_DBG_PRINT_RX_PARTIAL 0x2000  //debug: print any unexpected chars, partial line left in rx buffer at cmd end
#define CMD_OPTS_DEFAULT              (CMD_OPTS_EXCEPTION_EN | CMD_OPTS_REQUIRE_STATUS_CODE | CMD_OPTS_LOG_ALL | CMD_OPTS_DBG_PRINT_RX_PARTIAL)

typedef struct { char *p; int size; int wlen; } cmd_dbuf_t;

//Send a command and return response string
//@return response (NULL if timeout)
//e.g. send(IO_DUT, snformat(x,x,"lcdshow %u %s /"Victor DVT/"", solo=0, color="ib"), timeout=100 );
//@param async_handler: user handler, any line beginning with ASYNC_PREFIX
//@param dbuf: if provided, rx dat between cmd and response lines will be copied here. wlen includes \0.
char* cmdSend(cmd_io io, const char* scmd, int timeout_ms = CMD_DEFAULT_TIMEOUT, int opts = CMD_OPTS_DEFAULT, void(*async_handler)(char*) = 0, cmd_dbuf_t *dbuf = 0 );
int cmdStatus(); //parsed rsp status of most recent cmdSend(). status=1st arg, INT_MIN if !exist or bad format
uint32_t cmdTimeMs(); //time it took for most recent cmdSend() to finish

//-----------------------------------------------------------------------------
//                  Line Parsing
//-----------------------------------------------------------------------------
//Parsing methods for ascii input strings.
//Note: valid strings must guarantee no \r \n chars and one valid \0 terminator

//@return parsed integer value of s. INT_MIN on parse err.
int32_t cmdParseInt32(char *s);

//@return u32 value of input hex string (e.g. 'a235dc01'). 0 on parse error + errno set to -1
uint32_t cmdParseHex32(char* s);

//@return n-th argument (mutable static copy, \0-terminated). NULL if !exist.
//n=0 is command. strings enclosed by "" are treated as a single arg.
char* cmdGetArg(char *s, int n, char* out_buf=0, int buflen=0); //overload out_buf/buflen to provide user buffer to hold the argument

//@return # of args in the input string, including command arg
int cmdNumArgs(char *s);

//DEBUG: run some parsing tests
void cmdDbgParseTestbench(void);


#endif //CMD_H

