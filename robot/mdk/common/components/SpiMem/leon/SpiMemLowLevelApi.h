///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @addtogroup SpiMem
/// @{
/// @brief FatFS low-level back-end component for use on SPI chip
///

#ifndef SPIMEMLOWLEVELAPI_H_
#define SPIMEMLOWLEVELAPI_H_

#include <swcLeonUtils.h>
#include <isaac_registers.h>
#include <mv_types.h>


typedef struct
{
    u32 has_addr, n_dummy;
    u8 cmd;
    u8 addr[3];
} SPICMD;


extern SPICMD spiWREN ; ///< enable write
extern SPICMD spiWRDI; ///< disable write
extern SPICMD spiRDSR; ///< read status
extern SPICMD spiWRSR; ///< write status
extern SPICMD spiREAD; ///< read
extern SPICMD spiFREAD; ///< fast read
extern SPICMD spiPP; ///< page program
extern SPICMD spiSE; ///< sector erase
extern SPICMD spiBE; ///< block erase
extern SPICMD spiCE; ///< chip erase
extern SPICMD spiDP; ///< deep powerdown
extern SPICMD spiRDID; ///< read ID
extern SPICMD spiREMS; ///< read Mfg EM & ID
extern SPICMD spiRES; ///< release from powerdown and read electo sig

/// GenericSPI memory initialisation
void spiMemInit (void);

/// Generic SPI read/write operation with additional command sending
/// @param SPI_BASE_ADR_local  the SPI interface that should be used
/// @param cmd                 the command to be sent to the peripheral
/// @param n_rw                the byte count of the read/write transaction
/// @param wr_ptr              pointer to the write buffer
/// @param rd_ptr              pointer to the read buffer
/// @param pt                  initial endianness of the buffer
void spiMemExec(u32 SPI_BASE_ADR_local, SPICMD *cmd, u32 n_rw, u8 *wr_ptr, u8 *rd_ptr, pointer_type pt );


/// Wrapper to Write Enable Command
static inline void SpiMemWREN()
{
    spiMemExec( SPI1_BASE_ADR, &spiWREN,0,0,0,be_pointer);
}


/// Wrapper to Write Disable Command
static inline void SpiMemWRDI()
{
    spiMemExec( SPI1_BASE_ADR,&spiWRDI,0,0,0,be_pointer);
}


/// Wrapper to Read Status Register Command
/// @returns  the status flags of the spi flash peripheral
static inline u32 SpiMemRDSR()
{
    u32 _srr;
    spiMemExec( SPI1_BASE_ADR,&spiRDSR,1,0,(u8*)&_srr,be_pointer);
    return _srr>>24;
}


/// Wraper to Write Status Register Command
/// @param x status flags that should be written
static inline void SpiMemWRSR(u32 x)
{
    u32 _srw = x>>24;
    spiMemExec( SPI1_BASE_ADR,&spiWRSR,1,(u8*)&_srw,0,be_pointer);
}


/// Wrapper to Read Command
/// @param a   address on the flash peripheral from where the data should be read
/// @param n   size in bytes of the data
/// @param buf pointer to the data buffer
/// @param pt  initial endianness of the buffer
static inline void SpiMemREAD(u32 a, u32 n, u8 *buf, pointer_type pt)
{
    spiREAD.addr[0]=(u8)((a)>>16);
    spiREAD.addr[1]=(u8)((a)>>8);
    spiREAD.addr[2]=(u8)(a);
    spiMemExec( SPI1_BASE_ADR, &spiREAD, n, 0, buf, pt);

}


/// Wrapper to Fast Read Command
/// @param a   address on the flash peripheral from where the data should be read
/// @param n   size in bytes of the data
/// @param buf pointer to the data buffer
/// @param pt  initial endianness of the buffer
static inline void SpiMemFASTREAD(u32 a, u32 n, u8 *buf, pointer_type pt)
{
    spiFREAD.addr[0]=(u8)((a)>>16);
    spiFREAD.addr[1]=(u8)((a)>>8);
    spiFREAD.addr[2]=(u8)(a);
    spiMemExec( SPI1_BASE_ADR,&spiFREAD,n,0,buf, pt);
}


/// Wrapper to Page Program Command
/// @param a   address on the flash peripheral where the data should be written
/// @param n   size in bytes of the data
/// @param buf pointer to the data buffer
/// @param pt  initial endianness of the buffer
static inline void SpiMemPP(u32 a, u32 n, u8 *buf, pointer_type pt)
{
#ifndef SPI_FLASH_WRITE_ONE_BYTE_PER_COMMAND
    spiPP.addr[0]=(u8)((a)>>16);
    spiPP.addr[1]=(u8)((a)>>8);
    spiPP.addr[2]=(u8)(a);
    spiMemExec( SPI1_BASE_ADR,&spiPP,n,buf,0, pt);
#else
    int i;
    for (i=0;i<n;i++) {
        if (i!=0) {
            u32 _srr;
            do {
                _srr = SpiMemRDSR();
            } while( _srr & 1 );

            SpiMemWREN();
        }
        spiPP.addr[0]=(u8)((a)>>16);
        spiPP.addr[1]=(u8)((a)>>8);
        spiPP.addr[2]=(u8)(a);
        spiMemExec( SPI1_BASE_ADR,&spiPP,1,buf,0, pt);
        a += 1;
        buf += 1;
    }
#endif
}


/// Wrapper to Sector Erase command
/// Erase sector on the flash (reset to 0xFF)
/// @param a address on the flash to the sector that should be deleted
static inline void SpiMemSE(u32 a)
{
    spiSE.addr[0]=(u8)((a)>>16);
    spiSE.addr[1]=(u8)((a)>>8);
    spiSE.addr[2]=(u8)(a);
    spiMemExec( SPI1_BASE_ADR,&spiSE,0,0,0,be_pointer);
}

/// Wrapper to Block Erase command
/// Erase block on the flash (reset to 0xFF)
/// @param a address on the flash to the block that should be deleted
static inline void SpiMemBE(u32 a)
{
    spiBE.addr[0]=(u8)((a)>>16);
    spiBE.addr[1]=(u8)((a)>>8);
    spiBE.addr[2]=(u8)(a);
    spiMemExec( SPI1_BASE_ADR,&spiBE,0,0,0,be_pointer);
}

/// Wrapper to Chip Erase command
/// Erase all memory on the flash (reset to 0xFF)
static inline void SpiMemCE()
{
    spiMemExec( SPI1_BASE_ADR,&spiCE,0,0,0,be_pointer);
}

/// Wrapper to Deep Powerdown command
/// Shut down the flash chip and lower it's power usage.
/// Can be started using #SpiMemRES
static inline void SpiMemDP()
{
    spiMemExec( SPI1_BASE_ADR,&spiDP,0,0,0,be_pointer);
}

/// Wrapper to  Read Device Identification command
/// @returns  flash device ID
static inline u32 SpiMemRDID()
{
    u32 _rdid;
    spiMemExec( SPI1_BASE_ADR,&spiRDID,3,0, (u8*)&_rdid,be_pointer);
    return (_rdid>>8);
}

/// Wrapper to Read Electronic Manufacturer & Device Identification command
/// @returns  flash manufacturer and device ID
static inline u32 SpiMemREMS()
{
    u32 _rems;
    spiREMS.addr[2]=0;
    spiMemExec( SPI1_BASE_ADR,&spiREMS,2,0, (u8*)&_rems,be_pointer);
    return (_rems>>16);
}

/// Wrapper to Release from Deep Power-down, and Read Electronic Signature command
/// @returns  electronic signature of the device
static inline u32 SpiMemRES()
{
    u32 _res;
    spiMemExec(SPI1_BASE_ADR, &spiRES,1,0, (u8*)&_res,be_pointer);
    return (_res>>24);
}

/// @}
#endif /* SPIMEMLOWLEVELAPI_H_ */
