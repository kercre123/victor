#ifndef BDADDR_H
#define BDADDR_H

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------

#define BDADDR_SIZE       6
#define BDADDR_STR_SIZE   ((2*BDADDR_SIZE) + BDADDR_SIZE-1)

typedef struct {
  uint8_t addr[BDADDR_SIZE]; //byte order is LSB to MSB (LSB at array 0)
} bdaddr_t;

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

//test if given STATIC RANDOM address is valid, per BLE spec Core_v4.2
//@return true==valid
bool bdaddr_valid(bdaddr_t *bdaddr);

//parse bdaddr to string. format [hex] "##:##:##:##:##:##" LSB to MSB
//@return str (static internal - valid until next call)
char* bdaddr2str(bdaddr_t *bdaddr);

//parse string to bdaddr. expected format format [hex] "##:##:##:##:##:##" LSB to MSB
//(Note, currently very little format validation is performed)
//@return out_bdaddr [shortcut]. Set to all 0 (invalid) on parse error.
bdaddr_t* str2bdaddr(char* str, bdaddr_t *out_bdaddr);

#endif //BDADDR_H

