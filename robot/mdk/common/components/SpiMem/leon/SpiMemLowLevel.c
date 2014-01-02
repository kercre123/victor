///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
///
///

#include "DrvCpr.h"
#include "DrvSpi.h"
#include "DrvGpio.h"
#include "brdGpioCfgs/brdMv0153GpioDefaults.h"

#include "SpiMemLowLevelApi.h"


#if LOGGING
#include <stdio.h>
static const char hex[] = "0123456789ABCDEF";
char logg[5*1024];
#define LOGIDX u32 logidx = 0
#define LOGCLR() ({u32 i;for(i=0;i<2048;++i)logg[i]=0;})
#define LOGC(c) (logg[logidx++]=(c))
#define LOGB(c,v) (logg[logidx++]=(c),logg[logidx++]=hex[((v)>>4)&0xF],logg[logidx++]=hex[(v)&0xF])
#define LOGBIF(cond, c,v) if(wr_ptr)(logg[logidx++]=(c),logg[logidx++]=hex[((v)>>4)&0xF],logg[logidx++]=hex[(v)&0xF])
#define PLOG() (logg[logidx++]='\n',printf(logg))
#else
#define LOGIDX
#define LOGCLR()
#define LOGC(c)
#define LOGB(c,v)
#define LOGBIF(cond,c,v)
#define PLOG()
#endif


SPICMD spiWREN  = { .has_addr = 0, .n_dummy = 0, .cmd = 0x06 }; // enable write
SPICMD spiWRDI  = { .has_addr = 0, .n_dummy = 0, .cmd = 0x04 }; // disable write
SPICMD spiRDSR  = { .has_addr = 0, .n_dummy = 0, .cmd = 0x05 }; // read status
SPICMD spiWRSR  = { .has_addr = 0, .n_dummy = 0, .cmd = 0x01 }; // write status
SPICMD spiREAD  = { .has_addr = 1, .n_dummy = 0, .cmd = 0x03 }; // read
SPICMD spiFREAD = { .has_addr = 1, .n_dummy = 1, .cmd = 0x0B }; // fast read
SPICMD spiPP    = { .has_addr = 1, .n_dummy = 0, .cmd = 0x02 }; // page program
SPICMD spiSE    = { .has_addr = 1, .n_dummy = 0, .cmd = 0x20 }; // sector erase
SPICMD spiBE    = { .has_addr = 1, .n_dummy = 0, .cmd = 0xD8 }; // block erase
SPICMD spiCE    = { .has_addr = 0, .n_dummy = 0, .cmd = 0xC7 }; // chip erase
SPICMD spiDP    = { .has_addr = 0, .n_dummy = 0, .cmd = 0xB9 }; // deep powerdown
SPICMD spiRDID  = { .has_addr = 0, .n_dummy = 0, .cmd = 0x9F }; // read ID
SPICMD spiREMS  = { .has_addr = 1, .n_dummy = 0, .cmd = 0x90 }; // read Mfg EM & ID
SPICMD spiRES   = { .has_addr = 1, .n_dummy = 0, .cmd = 0xAB }; // release from powerdown and read electo sig


void spiMemExec(u32 SPI_BASE_ADR_local, SPICMD *cmd, u32 n_rw, u8 *wr_ptr, u8 *rd_ptr, pointer_type pt )
{
    u32 n_cmd, n_wr, n_rd;
    u8 wdata, rdata;
    LOGIDX;
    LOGCLR();

    if( !cmd )
        return;

    // clean the receive FIFO (if not empty )
    DrvSpiFlushFifo(SPI_BASE_ADR_local);

    // select the slave
    DrvSpiSs (SPI_BASE_ADR_local, SLV_SEL_1);

    // clear any eventual events/errors (such as RX underflow)
    SET_REG_WORD(SPI_BASE_ADR_local + EVENT_REG, ~0);

    // send the command
    LOGB('c',cmd->cmd);
    DrvSpiSendWord(SPI_BASE_ADR_local, cmd->cmd); n_cmd = 1;

    // send the address (if any)
    if( cmd->has_addr )
    {
        LOGB('a',cmd->addr[0]);
        DrvSpiSendWord(SPI_BASE_ADR_local, cmd->addr[0]); ++n_cmd;

        LOGB('a',cmd->addr[1]);
        DrvSpiSendWord(SPI_BASE_ADR_local, cmd->addr[1] ); ++n_cmd;

        LOGB('a',cmd->addr[2]);
        DrvSpiSendWord(SPI_BASE_ADR_local, cmd->addr[2] ); ++n_cmd;
    }

    // send dummy bytes
    for( wdata = 0; wdata < cmd->n_dummy; --wdata )
    {
        LOGC('d');
        DrvSpiSendWord(SPI_BASE_ADR_local, 0xFF );  ++n_cmd;
    }

    n_wr = n_rd = 0;
    // send/receive the actual data
    while( n_wr < n_rw )
    {
        // read incoming data
        while((GET_REG_WORD_VAL(SPI_BASE_ADR_local + EVENT_REG) & NE))
        {
            rdata = DrvSpiRecvWord(SPI_BASE_ADR_local);

            if( n_cmd ) // discard dummies appeared as responses to the cmd itself
            {
                --n_cmd;
            }
            else // got data to receive?
            {
                ++n_rd;
                if( rd_ptr )
                {
                    LOGB('r',rdata);
                    swcLeonSetByte(rd_ptr++, pt, rdata);
                }
            }
        }

        // send as much data as possible
        while ( ( n_wr < n_rw ) &&
                ( GET_REG_WORD_VAL(SPI_BASE_ADR_local + EVENT_REG) & NF ) &&
               !( GET_REG_WORD_VAL(SPI_BASE_ADR_local + EVENT_REG) & NE ))
        {
            //get one byte
            wdata = wr_ptr ? swcLeonGetByte(wr_ptr++, pt): (u8)~0;

            // send one byte
            LOGBIF(wr_ptr, 'w',wdata);
            DrvSpiSendWord(SPI_BASE_ADR_local, wdata ); ++n_wr;
        }
    }

    // discard or receive the remaining data
    while( n_cmd || n_rd < n_rw )
    {
        // wait if incoming FIFO empty
        while( !(GET_REG_WORD_VAL(SPI_BASE_ADR_local + EVENT_REG) & NE) );

        rdata = DrvSpiRecvWord(SPI_BASE_ADR_local);

        if( n_cmd ) // discard dummies appeared as responses to the cmd itself
        {
            --n_cmd;
        }
        else // got data to receive?
        {
            ++n_rd;
            if( rd_ptr )
            {
                LOGB('r',rdata);
                swcLeonSetByte(rd_ptr++, pt, rdata);
            }
        }
    }

    // de-select the slave
    DrvSpiSs (SPI_BASE_ADR_local, SLV_DES_ALL);

    LOGC('\n');
    PLOG();
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// to begin with, just use full-speed SPI (we're running of a <=13.5MHz xtal,
// w/ no PLL settings)
// NOTE: max supported freq is 100MHz; READ command can take up to 50MHz,
// fast read up to 100MHz
void spiMemInit (void)
{
    //enable SPI1 clocks
    DrvCprSysDeviceAction(ENABLE_CLKS ,DEV_SPI1);

    u32 sysClock = DrvCprGetSysClockKhz();

    // clock and reset SPI
    DrvCprSysDeviceAction(RESET_DEVICES ,DEV_SPI1);
    // disable for configuration
    DrvSpiEn(SPI1_BASE_ADR, 0);

    // configure 1 = master | 7 = 8bit | 1 = MSB first | 0 = not 3 wire
    DrvSpiInit(SPI1_BASE_ADR, 1, 7, 1, 0);
    // configure 3MHz clock
    DrvSpiTimingCfg(SPI1_BASE_ADR, 0, 0, 0, sysClock, 50000);
    // disable interrupts mask
    DrvSpiInMask(SPI1_BASE_ADR, 0);

    // flush and re-enable SPI
    DrvSpiFlushFifo(SPI1_BASE_ADR);
    DrvSpiEn(SPI1_BASE_ADR, 1);

    //enable last bit event
    DrvSpiSetLastBit(SPI1_BASE_ADR);

    //select EEPROM as SLAVE
    DrvSpiSs (SPI1_BASE_ADR, SLV_SEL_1);

    DrvGpioInitialiseRange(brdMv0153GpioCfgDefault);

    SpiMemWREN();
    SpiMemWRSR(0); // the BP bits are non-zero on power up on some flashes

}




