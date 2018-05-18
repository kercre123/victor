#ifndef OTP_H_
#define OTP_H_

#include <stdint.h>
#include <stdbool.h>
#include "bdaddr.h"
#include "da14580_otp_header.h"

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------

//defined in da14580_otp_header.h - shown here for reference
//#define OTP_ADDR_BASE                     0x040000 /*base address for OTP region*/
//#define OTP_ADDR_HEADER                   0x047F00 /*address where the header lives*/
//#define OTP_ADDR_END                      0x047FFF /*last address in the OTP region*/
//#define OTP_SIZE                          0x8000   /*32kB*/
//#define OTP_HEADER_SIZE                   0x0100   /*256B*/

#define OTP_WRITE_OK                  0
#define OTP_WRITE_ADDR_NOT_ALIGNED    -1
#define OTP_WRITE_SIZE_NOT_ALIGNED    -2
#define OTP_WRITE_OUTSIDE_OTP_RANGE   -3
#define OTP_WRITE_INTERNAL_ERR        -4

//bdaddr struct overlay in otp header
static inline bdaddr_t* otp_header_get_bdaddr( da14580_otp_header_t* otphead ) {
  return (bdaddr_t*)((int)otphead + OTP_HEADER_BDADDR_OFFSET);
}

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

void otp_read(uint32_t addr, int len, uint8_t *buf);
int  otp_write(uint32_t *dest, uint32_t *src, int size);

//prepare header data for write to otp
void otp_header_init( da14580_otp_header_t* otphead, bdaddr_t *bdaddr );

//-----------------------------------------------------------
//        Debug
//-----------------------------------------------------------

void otp_print_header( const char* heading, da14580_otp_header_t* otphead );

#endif //OTP_H_

