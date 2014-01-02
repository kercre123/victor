///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     I2C Slave Functions.
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <stdio.h>
#include "isaac_registers.h"
#include "mv_types.h"
#include "DrvI2c.h"
#include "DrvIcb.h"
#include "DrvGpio.h"
#include "DrvCpr.h"
#include "I2CSlaveApi.h"


// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

//#define I2CDetected(x) (*(volatile unsigned*) 0x80000000 =(x))
#define I2CDetected(x) ((void)(x))
#define I2C_DEVICE_NO 2

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------

static I2CStat I2CStatus[I2C_DEVICE_NO];

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
void I2CHandler(u32 source);

// 6: Functions Implementation
// ----------------------------------------------------------------------------

void I2CSLAVE_CODE_SECTION(I2CSlaveSetupCallbacks)(I2CSlaveHandle *hndl, i2cReadAction*  cbReadAction, i2cWriteAction* cbWriteAction, i2cBytesToSend* cbBytesToSend)
{
    hndl->cbReadAction = cbReadAction;
    hndl->cbWriteAction = cbWriteAction;
    hndl->cbBytesToSend = cbBytesToSend;
}

// Init the I2C Slave
int I2CSLAVE_CODE_SECTION(I2CSlaveInit)(I2CSlaveHandle *hndl, u32 blockNumber,I2CSlaveAddrCfg *config)
{
    u32 i2cSelection;

    if (hndl->i2cConfig.device==IIC1_BASE_ADR){
        i2cSelection=IIC1_BASE_ADR;
    }else{
        i2cSelection=IIC2_BASE_ADR;
    }
    I2CStatus[blockNumber].regSpaceMemory = (u32*)(config->regMemAddr);
    I2CStatus[blockNumber].regMemSize = config->regMemSize;

    DrvIicSlaveInit( hndl->i2cConfig.device, config->slvAddr,0x1, hndl->i2cConfig.speedKhz );

    // Set the IRQ source
    hndl->irqSource = IRQ_IIC1 + blockNumber;
    I2CStatus[blockNumber].i2cHndl = hndl;

    //Set Rx TH level to 2 in order to determine later the operation cmd
    SET_REG_WORD(i2cSelection+IIC_ENABLE, 0);
    SET_REG_WORD(i2cSelection+IIC_RX_TL, 20);
    SET_REG_WORD(i2cSelection+IIC_ENABLE, 1);

    // Clear all interrupts
    u32 irqStat = GET_REG_WORD_VAL(i2cSelection+IIC_CLR_INTR);

    // Configure the interrupt mask
    // We must set interrupt flags for:  RxFIFOFull, RxReadRequest, StopDetect
    SET_REG_WORD(i2cSelection+IIC_INTR_MASK, IIC_IRQ_RX_FULL | IIC_IRQ_RD_REQ | IIC_IRQ_STOP_DET | IIC_IRQ_RX_DONE | IIC_IRQ_TX_ABRT);

    // Associate the handler
    DrvIcbSetIrqHandler(hndl->irqSource, I2CHandler);

    // Set the interrupt properties in the entire system
    // A "level" interrupt is essential to avoid loosing new interrupts
    // that happen during the interrupt handler operation
    DrvIcbConfigureIrq(hndl->irqSource, I2CSLAVE_INTERRUPT_LEVEL, POS_LEVEL_INT);

    // Clear interrupt from this source
    DrvIcbIrqClear(hndl->irqSource);

    // Enable this specific interrupt
    DrvIcbEnableIrq(hndl->irqSource);

    // Init variables
    I2CStatus[blockNumber].addrSize = config->dataByteNo;
    I2CStatus[blockNumber].dataSize = config->dataByteNo;
    I2CStatus[blockNumber].i2CCmd = NO_CMD;
    I2CStatus[blockNumber].receivedBytes = 0;
    I2CStatus[blockNumber].i2cState = ADDR_BYTE1;
}

static void ProcessRxBufferBytes(u32 bytesToRead, u32 blockNumber) {
    u32 val;
    u32 i2cSelection;

    if (blockNumber == I2C_BLOCK_1){
        i2cSelection = IIC1_BASE_ADR;
    }else{
        i2cSelection = IIC2_BASE_ADR;
    }

    while ((bytesToRead--)>0) {
        if(I2CStatus[blockNumber].i2cState == ADDR_BYTE1)
        {
            // First 1 or 2 bytes are the slave reg
            I2CStatus[blockNumber].addrReg = GET_REG_WORD_VAL(i2cSelection+IIC_DATA_CMD);

            // The received bytes are 0
            I2CStatus[blockNumber].receivedBytes = 0;
            if( I2CStatus[blockNumber].addrSize != 2)
            {
                I2CStatus[blockNumber].i2cState = DATA_BYTE1;
            }
            else
            {
                I2CStatus[blockNumber].i2cState = ADDR_BYTE2;
            }
        }
        else if(I2CStatus[blockNumber].i2cState == ADDR_BYTE2)
        {
            I2CStatus[blockNumber].addrReg = (I2CStatus[blockNumber].addrReg << 8) | GET_REG_WORD_VAL(i2cSelection+IIC_DATA_CMD) ;
            I2CStatus[blockNumber].i2cState = DATA_BYTE1;
        }
        else if(I2CStatus[blockNumber].i2cState == DATA_BYTE1)
        {
            if(GET_REG_WORD_VAL(i2cSelection+IIC_RXFLR))
            {
                I2CStatus[blockNumber].i2CCmd = WRITE_CMD;
                // ^ This will force a later stop condition to notify the user
                // of the library via the callback function

                //dataVal[I2CStatus.addrReg] = GET_REG_WORD_VAL(i2cSelection+IIC_DATA_CMD);
                I2CStatus[blockNumber].regSpaceMemory[I2CStatus[blockNumber].addrReg] = GET_REG_WORD_VAL(i2cSelection+IIC_DATA_CMD);

                if(I2CStatus[blockNumber].dataSize != 2)
                {
                    I2CStatus[blockNumber].addrReg++;
                    I2CStatus[blockNumber].receivedBytes++;
                }
                else
                {
                    I2CStatus[blockNumber].i2cState = DATA_BYTE2;
                }
            }
        }
        else if(I2CStatus[blockNumber].i2cState == DATA_BYTE2)
        {
            if(GET_REG_WORD_VAL(i2cSelection+IIC_RXFLR))
            {
                //dataVal[I2CStatus.addrReg] = (dataVal[I2CStatus.addrReg] << 8) | GET_REG_WORD_VAL(i2cSelection+IIC_DATA_CMD);
                I2CStatus[blockNumber].regSpaceMemory[I2CStatus[blockNumber].addrReg] =
                        (I2CStatus[blockNumber].regSpaceMemory[I2CStatus[blockNumber].addrReg] << 8) | GET_REG_WORD_VAL(i2cSelection+IIC_DATA_CMD);

                I2CStatus[blockNumber].addrReg++;
                I2CStatus[blockNumber].receivedBytes++;
                I2CStatus[blockNumber].i2cState = DATA_BYTE1;
            }
        }
    }
}


// Manage the data in case of a Slave Receiver
void I2CHandler(u32 source)
{
    u32 i2cNum, i2cSelection;
    u32 val;

    if (source == IRQ_IIC1){
        i2cSelection = IIC1_BASE_ADR;
        i2cNum = I2C_BLOCK_1;
    }else{
        i2cSelection = IIC2_BASE_ADR;
        i2cNum = I2C_BLOCK_2;
    }
    I2CDetected('D');
    I2CStatus[i2cNum].status = GET_REG_WORD_VAL(i2cSelection+IIC_INTR_STAT);

    // First clear all the interrupts that we are going to handle.
    // This way we wont miss any new interrupts that happen during the
    // Interrupt handler.
    // No need to clear IIC_IRQ_RX_FULL, because it's cleared by hardware.
    if (I2CStatus[i2cNum].status & IIC_IRQ_RD_REQ)
        GET_REG_WORD_VAL(i2cSelection+IIC_CLR_RD_REQ);
    if (IIC_INTR_STAT & IIC_CLR_TX_ABRT)
        GET_REG_WORD_VAL(i2cSelection+IIC_CLR_TX_ABRT);
    if (I2CStatus[i2cNum].status & IIC_IRQ_STOP_DET)
        GET_REG_WORD_VAL(i2cSelection+IIC_CLR_STOP_DET);
    if (I2CStatus[i2cNum].status & IIC_IRQ_RX_DONE) // warning: evil naming scheme of register. This is TX from out point of view.
        GET_REG_WORD_VAL(i2cSelection+IIC_CLR_RX_DONE);
    // We also don't want to read any more bytes then there are
    // in the current transaction, otherwise we might interpret
    // address bytes of the next transaction as bulk data.
    // This is the best that we can do, because there is no way to
    // auto-force the peripheral into wait-states while we handle
    // the Stop bit, like the RD_REQ can.
    // All that this allows is to overlap receiving of bytes, and the interrupt
    // handler. There is no way to do flow control with this hardware.
    u32 bytesToRead = GET_REG_WORD_VAL(i2cSelection+IIC_RXFLR);

    if(I2CStatus[i2cNum].status & IIC_IRQ_RX_DONE) { // warning: evil naming scheme of register. This is TX from out point of view.
        I2CStatus[i2cNum].i2cState = ADDR_BYTE1;
        I2CStatus[i2cNum].i2CCmd = NO_CMD;
    }

    if(I2CStatus[i2cNum].status & IIC_IRQ_STOP_DET)
    {
        I2CDetected('P');

        ProcessRxBufferBytes(bytesToRead, i2cNum);

        if(I2CStatus[i2cNum].i2CCmd == WRITE_CMD)
        {

            I2CStatus[i2cNum].i2cHndl->cbWriteAction(I2CStatus[i2cNum].addrReg-I2CStatus[i2cNum].receivedBytes,
                    I2CStatus[i2cNum].regSpaceMemory[I2CStatus[i2cNum].addrReg-I2CStatus[i2cNum].receivedBytes],
                    I2CStatus[i2cNum].receivedBytes);
        }
        I2CStatus[i2cNum].i2cState = ADDR_BYTE1;
        I2CStatus[i2cNum].i2CCmd = NO_CMD;
    }

    // If the bus is IDLE and we have to figure out if we have a WRITE or a READ cmd
    if(I2CStatus[i2cNum].status & IIC_IRQ_RD_REQ)
    {
        // It is a read operation
        I2CStatus[i2cNum].i2CCmd = READ_CMD;
        I2CDetected('R');

        ProcessRxBufferBytes(bytesToRead, i2cNum);

        u32 noNeed=0;
        u32 bytesToSend = 1;
        u32 i;

        if(I2CStatus[i2cNum].i2cHndl->cbBytesToSend != NULL) {
            bytesToSend = I2CStatus[i2cNum].i2cHndl->cbBytesToSend(I2CStatus[i2cNum].addrReg);
        }

        //Send data to Master
        if(I2CStatus[i2cNum].dataSize != 2) {
            for(i = 0; i < bytesToSend && (GET_REG_WORD_VAL(i2cSelection + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL); i++) {
                I2CStatus[i2cNum].i2cHndl->cbReadAction(I2CStatus[i2cNum].addrReg, &val);
                I2CStatus[i2cNum].regSpaceMemory[I2CStatus[i2cNum].addrReg] = val;
                SET_REG_WORD(i2cSelection+IIC_DATA_CMD, (I2CStatus[i2cNum].regSpaceMemory[I2CStatus[i2cNum].addrReg] & 0xff));
                I2CStatus[i2cNum].addrReg++;
            }
        } else {
            for(i = 0; i < bytesToSend && (GET_REG_WORD_VAL(i2cSelection + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL); i++) {
                if (I2CStatus[i2cNum].i2cState != DATA_BYTE2) {
                    I2CStatus[i2cNum].i2cHndl->cbReadAction(I2CStatus[i2cNum].addrReg, &val);
                    I2CStatus[i2cNum].regSpaceMemory[I2CStatus[i2cNum].addrReg] = val;
                    SET_REG_WORD(i2cSelection+IIC_DATA_CMD, ((val >> 8) & 0xff));
                    I2CStatus[i2cNum].i2cState = DATA_BYTE2;
                } else {
                    if(GET_REG_WORD_VAL(i2cSelection + IIC_STATUS) & IIC_STATUS_TX_FIFO_NOT_FULL) {
                        SET_REG_WORD(i2cSelection+IIC_DATA_CMD, (I2CStatus[i2cNum].regSpaceMemory[I2CStatus[i2cNum].addrReg] & 0xff));
                        I2CStatus[i2cNum].i2cState = DATA_BYTE1;
                        I2CStatus[i2cNum].addrReg++;
                    }
                }
            }
        }
    }
    else  if(I2CStatus[i2cNum].status & IIC_IRQ_RX_FULL)
    {
        I2CDetected('W');

        ProcessRxBufferBytes(bytesToRead, i2cNum);
    }

    // Clear interrupt from this source
    DrvIcbIrqClear(I2CStatus[i2cNum].i2cHndl->irqSource);
}


