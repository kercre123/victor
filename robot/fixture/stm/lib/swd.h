#ifndef SWD_H
#define SWD_H

#include <stdint.h>
#include "board.h"

//signal map
#define SWDIO           DUT_SWD
#define SWCLK           DUT_SWC
#define NRST            DUT_RESET

//SWD responses & errors
typedef int swd_resp_t;
#define SWD_ACK         1
#define SWD_WAIT        2
#define SWD_FAULT       4
#define SWD_PARITY      8

//swd_read/write() 'char APnDP'
#define DEBUG_PORT      0
#define ACCESS_PORT     1

//SWDP register map
#define SWDP_REG_R_IDCODE     0x0
#define SWDP_REG_W_ABORT      0x0
#define SWDP_REG_RW_CTRLSTAT  0x4 //CTRLSEL=0 in the SELECT register
#define SWDP_REG_RW_WCR       0x4 //CTRLSEL=1 in the SELECT register (optional register)
#define SWDP_REG_R_RESEND     0x8
#define SWDP_REG_W_SELECT     0x8
#define SWDP_REG_R_RDBUF      0xC
#define SWDP_REG_W_ROUTSEL    0xC //(optional register)

//SWDP IDCODE register fields
#define IDCODE_GET_VERSION(idcode_)     ( ((idcode_) & 0xF0000000) >> 28 )
#define IDCODE_GET_PARTNO(idcode_)      ( ((idcode_) & 0x0FFFF000) >> 12 )
#define IDCODE_GET_DESIGNER(idcode_)    ( ((idcode_) & 0x00000FFE) >> 1  )

//SWDP CTRL/STAT register fields
#define SWDP_CTRLSTAT_CSYSPWRUPACK      0x80000000
#define SWDP_CTRLSTAT_CSYSPWRUPREQ      0x40000000
#define SWDP_CTRLSTAT_CDBGPWRUPACK      0x20000000
#define SWDP_CTRLSTAT_CDBGPWRUPREQ      0x10000000
#define SWDP_CTRLSTAT_CDBGRSTACK        0x08000000
#define SWDP_CTRLSTAT_CDBGRSTREQ        0x04000000
#define SWDP_CTRLSTAT_RAZ_SBZP          0x03000000
#define SWDP_CTRLSTAT_TRNCNT            0x00FFF000
#define SWDP_CTRLSTAT_MASKLANE          0x00000F00
#define SWDP_CTRLSTAT_WDATAERR          0x00000080
#define SWDP_CTRLSTAT_READOK            0x00000040
#define SWDP_CTRLSTAT_STICKYERR         0x00000020
#define SWDP_CTRLSTAT_STICKYCMP         0x00000010
#define SWDP_CTRLSTAT_TRNMODE           0x0000000C
#define SWDP_CTRLSTAT_STICKYORUN        0x00000002
#define SWDP_CTRLSTAT_ORUNDETECT        0x00000001

//-----------------------------------
//        SWD Common
//-----------------------------------

void swd_init(void);
void swd_deinit(void); //release pins,resources

swd_resp_t swd_write32(uint32_t addr, uint32_t data);
uint32_t   swd_read32(uint32_t addr);

swd_resp_t swd_write(char APnDP, int A, unsigned long data);
swd_resp_t swd_read(char APnDP, int A, volatile unsigned long *data);

//SWD PHY
void swd_phy_reset(void); //resets target swd state machine
void swd_phy_turnaround_host(); //Switch to host-controlled bus
void swd_phy_turnaround_target(); //Switch to target-controlled bus
void swd_phy_write_bits(int nbits, const unsigned long *pdat);
void swd_phy_read_bits(int nbits, volatile unsigned long *bits);

//-----------------------------------
//        STM32
//-----------------------------------

#define STM32F030XX_PAGESIZE 1024

int swd_stm32_init(); //initialize the swd interface and connect to mcu's debug port
int swd_stm32_deinit(); //teardown & de-init

int swd_stm32_erase(void);
int swd_stm32_flash(uint32_t flash_addr, const uint8_t* bin_start, const uint8_t* bin_end, bool verify = true); //return 0 (success) or error code
int swd_stm32_verify(uint32_t flash_addr, const uint8_t* bin_start, const uint8_t* bin_end);
int swd_stm32_read(uint32_t flash_addr, uint8_t *out_buf, int size);

//Lock the JTAG port - upon next reset, JTAG will not work again - EVER
//Returns 0 on success, 1 on erase failure (chip will survive), 2 on program failure (dead)
int swd_stm32_lock_jtag(void);

//-----------------------------------
//        Dialog DA1458x
//-----------------------------------

int swd_da1458x_load_bin(const uint8_t* bin_start, const uint8_t* bin_end); //return 0 (success) or error code

int swd_da1458x_run();

#endif //SWD_H

