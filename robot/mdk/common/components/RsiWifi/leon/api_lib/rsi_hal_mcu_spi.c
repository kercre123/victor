/**
 * @file
 * @version		2.3.0.0
 * @date 		2011-May-30
 *
 * Copyright(C) Redpine Signals 2011
 * All rights reserved by Redpine Signals.
 *
 * @section License
 * This program should be used on your own responsibility.
 * Redpine Signals assumes no responsibility for any losses
 * incurred by customers or third parties arising from the use of this file.
 *
 */

/**
 * Includes
 */
#include "rsi_global.h"
#include <isaac_registers.h>
#include <stdio.h>
#include <DrvGpio.h>
#include <DrvSpi.h>
#include <DrvIcb.h>
#include <DrvCpr.h>
#include <DrvTimer.h>

void isrSpiRdWr(u32 source);

/**
 * Global Variables
 */
u32 rsiSpiBaseAddr;

/**
 * Global Defines
 */
#define SPI_FIFO_DEPTH 16

volatile uint8 *gPtrBuf;
volatile uint16 gBufLen;
volatile uint16 gBufTodo;
volatile uint16 gBufIdx;
volatile uint8 *gValBuf;
volatile uint8 gMode;
volatile uint32 gNTimes;
volatile uint32 spiIRQ;
void (*gCallBack)();

void mv_spiInit(u32 spiBaseAddr)
{
    u32 spiClk;

    rsiSpiBaseAddr = spiBaseAddr;

    if (rsiSpiBaseAddr == SPI1_BASE_ADR)
    {
        spiClk = DEV_SPI1;
        spiIRQ = IRQ_SPI1;
    }
    else if (rsiSpiBaseAddr == SPI2_BASE_ADR)
    {
        spiClk = DEV_SPI2;
        spiIRQ = IRQ_SPI2;
    }
    else
    {
        spiClk = DEV_SPI3;
        spiIRQ = IRQ_SPI3;
    }

    // init clocks
    DrvCprSysDeviceAction(ENABLE_CLKS,  spiClk );

    // debug purposes
    // checking irq/total time ratio
    //DrvGpioMode(103, D_GPIO_DIR_OUT | D_GPIO_MODE_7); // Always ensure that GPIO is enabled

    // setup spi
    DrvSpiSs(rsiSpiBaseAddr, SLV_DES_ALL);
    DrvSpiEn(rsiSpiBaseAddr, 0);
    DrvSpiInit(rsiSpiBaseAddr, 1/*master*/, 7/*wsize*/, 1/*MSB first*/, 0/*4 wire mode*/);
    DrvSpiTimingCfg(rsiSpiBaseAddr, 0/*cpol*/, 0/*cpha*/,
            0/*cg=x many clocks between transactions*/, 180000, 30000);    //720, 10// we should try for 15000 if this works
    *(unsigned*) (rsiSpiBaseAddr + EVENT_REG) = 0x00005F00;
    DrvSpiInMask(rsiSpiBaseAddr, LTE);
    DrvSpiEn(rsiSpiBaseAddr, 1);
    DrvSpiSs(rsiSpiBaseAddr, SLV_SEL_2);

    DrvIcbSetIrqHandler(spiIRQ, isrSpiRdWr);
    DrvIcbConfigureIrq(spiIRQ, SPI_INTERRUPT_LEVEL, POS_EDGE_INT);
    DrvIcbEnableIrq(spiIRQ);

    gCallBack = NULL;
}

void isrSpiRdWr(u32 source)
{

    void (*lCallBack)();
#ifdef RSI_BIT_32_SUPPORT
    uint32 dummy_ret;
#else
    uint8 dummy_ret;
#endif
    uint32 temp;
    int8 i, fifo_level;

    while ((*(volatile unsigned*) (rsiSpiBaseAddr + EVENT_REG) & NE))
        RSI_SPI_READ_DUMMY_BYTE(dummy_ret);

    fifo_level = (SPI_FIFO_DEPTH < gBufTodo) ? SPI_FIFO_DEPTH : gBufTodo;
    if ((gBufIdx + fifo_level) < gBufLen)
    {
        //setup for a new FIFO Empty irq
        SET_REG_WORD(rsiSpiBaseAddr + EVENT_REG, LT);
        SET_REG_WORD(rsiSpiBaseAddr + COMMAND_REG, LST);
    }

    for (i = 0; i < fifo_level; i++)
    {
#ifdef RSI_BIT_32_SUPPORT
        if(gMode == RSI_MODE_8BIT)
#endif
        {
            RSI_SPI_SEND_BYTE(gPtrBuf[gBufIdx]);
            gBufIdx += 1;
        }
#ifdef RSI_BIT_32_SUPPORT
        else
        {
            temp = gPtrBuf[gBufIdx+0] << 0 |    // this deserves a second look
            ptrBuf[gBufIdx+1] << 8 |
            ptrBuf[gBufIdx+2] << 16 |
            ptrBuf[gBufIdx+3] << 24;
            RSI_SPI_SEND_4BYTE(temp);
            // Byte order should not change while sending data
            gBufIdx+=4;
        }
#endif

        if (gNTimes < 2)
        {
#ifdef RSI_BIT_32_SUPPORT
            if(gMode == RSI_MODE_8BIT)
#endif
            {
                RSI_SPI_READ_BYTE(gValBuf[gNTimes]);
            }
#ifdef RSI_BIT_32_SUPPORT
            else
            {
                RSI_SPI_READ_4BYTE( temp );
                gValBuf[gNTimes+0] = temp & 0x000000ff;    // this deserves a second look
                gValBuf[gNTimes+1] = (temp >> 8) & 0x000000ff;
                gValBuf[gNTimes+2] = (temp >> 16) & 0x000000ff;
                gValBuf[gNTimes+3] = (temp >> 24) & 0x000000ff;
                // Byte order should not change while reading data
            }
#endif
        }
        else
        {
#ifdef RSI_BIT_32_SUPPORT
            if(gMode == RSI_MODE_8BIT)
#endif
            {
                RSI_SPI_READ_DUMMY_BYTE(dummy_ret);
            }
#ifdef RSI_BIT_32_SUPPORT
            else
            {
                RSI_SPI_READ_4BYTE(dummy_ret);
            }
#endif
        }
        if (gBufIdx < gBufLen)
            gNTimes++;
    }
    gBufTodo -= fifo_level;

    if ((gBufIdx >= gBufLen) && (gCallBack != NULL))
    {
        lCallBack = gCallBack;
        gCallBack = NULL;
        lCallBack();
    }
    // add a sleep of 100 micro for cases when other interrupts are used,such as a cif interrupt
    SleepMicro(100);
}

/*==================================================================*/
/**
 * @fn 		int16 rsi_spiSend(uint8 *ptrBuf,uint16 bufLen,uint8 *valBuf,uint8 mode)
 * @param[in]	uint8 *ptrBuf, pointer to the buffer with the data to be sent/received
 * @param[in]	uint16 bufLen, number of bytes to send
 * @param[in]	uint8 *valBuf, pointer to the buffer where the response is stored
 * @param[in]	uint8 mode, To indicate mode 8 BIT/32 BIT.
 * @param[out]	None
 * @return	0, 0=success
 * @section description	
 * This API is used to send data to the Wi-Fi module through the SPI interface.
 */

int16 rsi_spiSend(uint8 *ptrBuf, uint16 bufLen, uint8 *valBuf, uint8 mode)
{

    gPtrBuf = ptrBuf;
    gBufLen = bufLen;
    gBufTodo = bufLen;
    gValBuf = valBuf;
    gMode = mode;
    gNTimes = 0;
    gBufIdx = 0;

    DrvIcbIrqTrigger(spiIRQ);

    if (gCallBack == NULL)
    {
        while (gBufIdx < gBufLen)
            NOP;
    }
    return 0;
}

int16 rsi_spiSendNoIrq(uint8 *ptrBuf, uint16 bufLen, uint8 *valBuf, uint8 mode)
{
    uint16              i,j;
    #ifdef RSI_BIT_32_SUPPORT
        uint32 dummy_ret;
    #else
        uint8 dummy_ret;
    #endif
        for (i=0,j=0; i < bufLen; j++) {
    #ifdef RSI_BIT_32_SUPPORT
            if(mode == RSI_MODE_8BIT)
    #endif
            {
                RSI_SPI_SEND_BYTE(ptrBuf[i]);
                i+=1;
            }
    #ifdef RSI_BIT_32_SUPPORT
            else
            {
                RSI_SPI_SEND_4BYTE(*(uint32 *)&ptrBuf[i]);
                // Byte order should not change while sending data
                i+=4;
            }
    #endif
            if((j<2))
            {
    #ifdef RSI_BIT_32_SUPPORT
                if(mode == RSI_MODE_8BIT)
    #endif
                {
                    RSI_SPI_READ_BYTE(valBuf[j]);
                }
    #ifdef RSI_BIT_32_SUPPORT
                else
                {
                    RSI_SPI_READ_4BYTE(*(uint32 *)&valBuf[j]);
                    // Byte order should not change while reading data
                }
    #endif
            }
            else
            {
    #ifdef RSI_BIT_32_SUPPORT
                if(mode == RSI_MODE_8BIT)
    #endif
                    RSI_SPI_READ_BYTE(dummy_ret);
    #ifdef RSI_BIT_32_SUPPORT
                else
                    RSI_SPI_READ_4BYTE(dummy_ret);
    #endif
            }
        }
        return 0;
}

/*==================================================================*/
/**
 * @fn 		int16 rsi_spiRecv(uint8 *ptrBuf,uint16 bufLen,uint8 mode)
 * @param[in]	uint8 *ptrBuf, pointer to the buffer with the data to be sent/received
 * @param[in]	uint16 bufLen, number of bytes to send
 * @param[in]	uint8 mode, To indicate the mode 8 BIT / 32 BIT.
 * @param[out]	None
 * @return	0, 0=success
 * @description	This API is used to receive data from Wi-Fi module through the SPI interface.
 */

int16 rsi_spiRecv(uint8 *ptrBuf, uint16 bufLen, uint8 mode)
{
    int16 i;
    uint32 temp;
#ifdef RSI_BIT_32_SUPPORT
    uint32 dummy=0;
    uint32 wc;

    wc = *(volatile unsigned*)(SPI1_BASE_ADR + MODE_REG) >> 20 & 0x0F;    // word size
    if( wc==7 && mode==RSI_MODE_32BIT )
    {    //switch to 32bit
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT( RSI_PL3,"Switch to 32bit\n");
#endif

        *(volatile unsigned*)(SPI1_BASE_ADR + MODE_REG) = 0x07020000;
    }
    if( wc==0 && mode==RSI_MODE_8BIT )
    {    //switch to 8bit
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT( RSI_PL3,"Switch to 8bit\n");
#endif
        *(volatile unsigned*)(SPI1_BASE_ADR + MODE_REG) = 0x07720000;
    }

#else
    uint8 dummy = 0;
#endif

    for (i = 0; i < bufLen;)
    {
#ifdef RSI_BIT_32_SUPPORT
        if(mode == RSI_MODE_8BIT)
#endif
        {
            RSI_SPI_SEND_BYTE(dummy);
        }
#ifdef RSI_BIT_32_SUPPORT
        else
        {
            RSI_SPI_SEND_4BYTE(dummy);
        }
#endif

#ifdef RSI_BIT_32_SUPPORT
        if(mode == RSI_MODE_8BIT)
#endif
        {
            RSI_SPI_READ_BYTE(ptrBuf[i]);
            i += 1;
        }
#ifdef RSI_BIT_32_SUPPORT
        else
        {
            RSI_SPI_READ_4BYTE( temp );
            ptrBuf[i+0] = temp & 0x000000ff;    // this deserves a second look
            ptrBuf[i+1] = (temp >> 8) & 0x000000ff;
            ptrBuf[i+2] = (temp >> 16) & 0x000000ff;
            ptrBuf[i+3] = (temp >> 24) & 0x000000ff;
            //Byte order should not change while reading data
            i+=4;
        }
#endif
#ifdef RSI_DEBUG_PRINT
        RSI_DPRINT(RSI_PL10, "%02x", (uint16 )ptrBuf[i]);
#endif
    }
    return 0;
}
