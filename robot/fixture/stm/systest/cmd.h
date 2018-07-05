#ifndef CMD_H
#define CMD_H

#include <stdbool.h>
#include <stdint.h>

//fixture error codes
#define FIXTURE_EXTERNAL
#include "../app/fixture.h"

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------

//parameterized command/response delimiters
#define CMD_PREFIX        ">>"
#define RSP_PREFIX        "<<"
//#define ASYNC_PREFIX      "!!"
//#define LOG_CMD_PREFIX    ">"
//#define LOG_RSP_PREFIX    "<"

#define STATUS_NOT_A_CMD      -2  //input string is not a command (no prefix?)
#define STATUS_NULL           -1  //null or unexpected
#define STATUS_OK             0
#define STATUS_UNKNOWN_CMD    1   //unrecognized command
#define STATUS_ARG_NA         2   //missing an expected argument/param
#define STATUS_ARG_PARSE      3   //error parsing one of the arguments. check formatting

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

//process a command (entire line rx'd from uart)
int cmd_process(char* s); //@return status code

#endif //CMD_H

