#ifndef SPINE_CRC_H
#define SPINE_CRC_H

#include <stdint.h>
typedef uint32_t crc_t;

crc_t calc_crc(const uint8_t* buf, int len);


#endif//SPINE_CRC_H
