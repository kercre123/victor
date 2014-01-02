///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     SPI Slave Functions.
///

// 1: Includes
// ----------------------------------------------------------------------------
#include <stdio.h>
#include "mv_types.h"
#include "isaac_registers.h"
#include "DrvSpi.h"
#include "DrvIcb.h"
#include "DrvGpio.h"
#include "DrvCpr.h"
#include "SPISlaveApi.h"
#include "DrvTimer.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
static spiSlaveHandle *spiHndl;
u32 bulk_base_addr;				// base address for the bulk buffer we get from the app
u16 *meta_addr;					// relative address of the active bulk buffer

volatile u32 bulk_size = 0;
volatile u32 transfer;
volatile u16 addr;
volatile u16 offset;
volatile u8 cmd;

tyTimeStamp timestamp;
volatile u32 clocks = 0;
volatile u32 totalClocks = 0;

u16 val;
u32 spi_next_state = SPI_UPDATE_CMD;

// 4: Static Local Data
// ----------------------------------------------------------------------------
// Structure array containing SPI block information
static const spiSlaveCfg spiX[3] = {
{
	.dev_addr	= SPI1_BASE_ADR,
	.rx_reg		= SPI1_BASE_ADR + RECEIVE_REG,
	.tx_reg		= SPI1_BASE_ADR + TRANSMIT_REG,
	.ev_reg		= SPI1_BASE_ADR + EVENT_REG
},
{
	.dev_addr	= SPI2_BASE_ADR,
	.rx_reg		= SPI2_BASE_ADR + RECEIVE_REG,
	.tx_reg		= SPI2_BASE_ADR + TRANSMIT_REG,
	.ev_reg		= SPI2_BASE_ADR + EVENT_REG
},
{
	.dev_addr	= SPI3_BASE_ADR,
	.rx_reg		= SPI3_BASE_ADR + RECEIVE_REG,
	.tx_reg		= SPI3_BASE_ADR + TRANSMIT_REG,
	.ev_reg		= SPI3_BASE_ADR + EVENT_REG
}
};

// SPI slave default timings:
static spiSlaveTiming spiSlaveDefaultTiming =
{
	
	.ms		 = 0x0,		// slave mode
	.len	 = 0xF,		// 16bit transfers
	.rev	 = 0x1,		// reversed data
	.tw		 = 0x0,		// three wire mode OFF
	.cpol	 = 0x0,		// polarity
	.cpha	 = 0x0,		// phase
	.cg		 = 0x0		// clock gap off

};

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

static void SPIHandler(u32 unused)
{	
  while (GET_REG_WORD_VAL(spiX[spiHndl->device].ev_reg) & NE) {
	// State machine
	switch(spi_next_state)
	{
	case SPI_UPDATE_CMD:
	
		// Read the first two bytes (command)
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);
		// Decode the command
		cmd = transfer >> 16;

		switch(cmd)
		{
		case S_READ:
			spi_next_state = SPI_READ_1;
			break;
		case S_WRITE:
			spi_next_state = SPI_WRITE_1;
			break;
		case B_READ:
			spi_next_state = SPI_BULK_READ_1;
			break;
		default:
			// invalid command from master
			spi_next_state = SPI_UPDATE_CMD;
			break;
		}
		
		break;

	case SPI_READ_1:
		// Get the address
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);
		// Decode it
		addr = transfer >> 16;
		
		spiHndl->callbackfnc->cbSingleRead(addr, &val);

		// Write response if there's room in the FIFO
		if((GET_REG_WORD_VAL(spiX[spiHndl->device].ev_reg) & NF))
			SET_REG_WORD(spiX[spiHndl->device].tx_reg, val << 16);

		spi_next_state = SPI_READ_2;
		break;

	case SPI_READ_2:
		// Master does not yet read actual data
		// Empty FIFO (dummy data)
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);
		spi_next_state = SPI_READ_3;
		break;

	case SPI_READ_3:
		// Master reads actual data during this transfer
		// Empty FIFO (dummy data)
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);
		spi_next_state = SPI_UPDATE_CMD;
		break;

	case SPI_WRITE_1:
		// Get the address
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);

		// Decode the address
		addr = transfer >> 16;

		spi_next_state = SPI_WRITE_2;
		break;

	case SPI_WRITE_2:
		// Get the write value
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);
		// Decode it
		val = transfer >> 16;
		// Update it
		spiHndl->callbackfnc->cbSingleWrite(addr, val);

		spi_next_state = SPI_UPDATE_CMD;
		break;

	case SPI_BULK_READ_1:
		
		// Get the bulk address offset
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);

		// Decode the offset
		offset = transfer >> 16;
		offset >>= 1;

		// Call the app to get the base address for the bulk buffer
		spiHndl->callbackfnc->cbGetBlockPtr(&bulk_base_addr);
		
		// We'll be reading from an offset address
		// relative to the base metadata address
		meta_addr = (u16*)bulk_base_addr + offset;

		spi_next_state = SPI_BULK_READ_2;
		break;
	
	case SPI_BULK_READ_2:
		// Get the size of the bulk read
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);

		// Decode the size (endianness) + 1 transfer delay (2B)
		bulk_size = (transfer >> 16) + 2;

		val = *meta_addr;

		// Write first 2 bytes if there's room in the FIFO
		if((GET_REG_WORD_VAL(spiX[spiHndl->device].ev_reg) & NF))
		{
			SET_REG_WORD(spiX[spiHndl->device].tx_reg, val << 16);
			bulk_size -= 2;
			meta_addr += 1;
		}
		
		spi_next_state = SPI_BULK_READ_3;
		break;
	
	case SPI_BULK_READ_3:
		// Empty the FIFO (dummy data)
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);

		val = *meta_addr;
		// Write next 2 bytes if there's room in the FIFO
		if((GET_REG_WORD_VAL(spiX[spiHndl->device].ev_reg) & NF))
		{
			SET_REG_WORD(spiX[spiHndl->device].tx_reg, val << 16);
			bulk_size -= 2;
			meta_addr += 1;
		}

		spi_next_state = SPI_BULK_READ_4;
		break;

	case SPI_BULK_READ_4:
		// The master will start to read valid data starting 
		// from this tranfer

		// Read dummy RX
		transfer = *(u32*)(spiX[spiHndl->device].rx_reg);

		if(bulk_size > 2)	// still got data to send
		{
			val = *meta_addr;
			
			// Write 2 bytes of metadata if there's room in the FIFO
			if((GET_REG_WORD_VAL(spiX[spiHndl->device].ev_reg) & NF))
				SET_REG_WORD(spiX[spiHndl->device].tx_reg, val << 16);
			
			bulk_size -= 2;
			meta_addr += 1;
			
			spi_next_state = SPI_BULK_READ_4;
		}
		else if(bulk_size > 0)	// Next to last transfer from master. Don't respond with anything
		{
			bulk_size -= 2;
			spi_next_state = SPI_BULK_READ_4;
		}
		else // End of metadata bulk block reached
		{
			spi_next_state = SPI_UPDATE_CMD;
			spiHndl->callbackfnc->cbBlockEnd(0, BLOCK_END_NORMAL);
		}
		break;
		
	default:
		spi_next_state = SPI_UPDATE_CMD;
		break;

	}

	DrvIcbIrqClear( spiHndl->irqSource );
  }
}

int SPISlaveInit(spiSlaveHandle *hndl, u32 blockNumber, spiSlaveTiming *cfg, spiCallBackStruct_t *cb_ptr)
{
	
	spiSlaveTiming timingCfg;	
	spiHndl = hndl;
	
	spiHndl->device = blockNumber - 1;

	// Timing configuration
	if(cfg == NULL)
		timingCfg = spiSlaveDefaultTiming;
	else
		timingCfg = *cfg;

	// Disable SPI module
	DrvSpiEn(spiX[spiHndl->device].dev_addr, 0);

	// Configure mode
	DrvSpiInit(spiX[spiHndl->device].dev_addr, 
				 timingCfg.ms, 
				 timingCfg.len, 
				 timingCfg.rev, 
				 timingCfg.tw);

	// Configure timing
	DrvSpiTimingCfg(spiX[spiHndl->device].dev_addr, 
					   timingCfg.cpol, 
					   timingCfg.cpha, 
					   timingCfg.cg, 
					   0, 
					   0);

	// Enable SPI module
	DrvSpiEn(spiX[spiHndl->device].dev_addr, 1);

	// Unmask Not Empty interrupt
	SET_REG_WORD(spiX[spiHndl->device].dev_addr + MASK_REG, NEE);

	// Save callback routine
	hndl->callbackfnc = cb_ptr;

	// Set the IRQ source
	hndl->irqSource = IRQ_SPI1 + blockNumber - 1;

	// Disable ICB (Interrupt Control Block) while setting new interrupt
	DrvIcbDisableIrq(hndl->irqSource);

	// Clear any existing interrupts
	DrvIcbIrqClear(hndl->irqSource);

	// Configure Leon to accept traps on any level
	swcLeonSetPIL(0);

	// Configure interrupt handlers
	DrvIcbSetIrqHandler(hndl->irqSource, SPIHandler);

	// Configure interrupts
	DrvIcbConfigureIrq(hndl->irqSource, SPISLAVE_INTERRUPT_LEVEL, POS_EDGE_INT);

	// Trigger the interrupts
	DrvIcbEnableIrq(hndl->irqSource);

	// Can enable the interrupt now
	swcLeonEnableTraps();

}
