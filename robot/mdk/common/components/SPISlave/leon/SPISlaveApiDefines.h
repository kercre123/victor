///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// This is the file that contains all the Sensor definitions of constants, typedefs,
/// structures or enums or exported variables from this module

#ifndef _SPI_SLAVE_API_DEFINES_H_
#define _SPI_SLAVE_API_DEFINES_H_


#ifndef SPISLAVE_INTERRUPT_LEVEL
#define SPISLAVE_INTERRUPT_LEVEL 1
#endif


enum	// block number
{
    SPI1 = 1,
    SPI2,
	SPI3
};

typedef struct
{
    u32 dev_addr;	// SPI device address
	u32 rx_reg;		// Rx register
	u32 tx_reg;		// Tx register
	u32 ev_reg;		// Event register
}spiSlaveCfg;	

typedef struct
{
	u32 ms;		// master/slave
    u32 len;	// transfer length (= len+1 bits; RTM)
    u32 rev;	// reverse data
    u32 tw;		// three wire mode
    u32 cpol;	// clock polarity
    u32 cpha;	// clock phase
	u32 cg;		// clock gap
}spiSlaveTiming;


enum	// possible commands from master
{
    S_READ,		// single read
    S_WRITE,	// single write
    B_READ,		// bulk read 
    B_WRITE		// bulk write
};


enum	// possible states	
{
	SPI_UPDATE_CMD,		// Command
	SPI_READ_1,			// Read address
	SPI_READ_2,			// Padding
	SPI_READ_3,			// Padding
	SPI_WRITE_1,		// Write address
	SPI_WRITE_2,		// Write value
	SPI_BULK_READ_1,	// Bulk address (offset)
	SPI_BULK_READ_2,	// Bulk size (in bytes)
	SPI_BULK_READ_3,	// Padding
	SPI_BULK_READ_4		// Wait and transfer
};

typedef enum
{
	BLOCK_END_NORMAL,
	BLOCK_END_ABORT
}spiTransactionStatus;

typedef void (spi_short_read)(u32, u16*);
typedef void (spi_short_write)(u32, u32);
typedef void (spi_get_block_ptr)(u32*);
typedef void (spi_block_end)(u32, spiTransactionStatus);

typedef struct	// callbacks
{
	spi_short_read*		cbSingleRead;
	spi_short_write*	cbSingleWrite;
	spi_get_block_ptr*	cbGetBlockPtr;
	spi_block_end*		cbBlockEnd;
}spiCallBackStruct_t;

typedef struct	// SPI handle
{
	u32 device;
	spiCallBackStruct_t *callbackfnc;
    u32 irqSource;
}spiSlaveHandle;


#endif
