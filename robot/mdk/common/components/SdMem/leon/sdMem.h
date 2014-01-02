///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup sdMem sdMem component
/// @{
/// @brief     Component used for for accessing SD/MMC cards
///
/**
@file
This component is under development,
and has a number of known issues:

+ MMC cards are not tested at the moment.
+ due to FatFs accessing at most 4KiB at a time,
  the read/write performance is not the best it could be
  Please use the FatFs/diskio_wrapper/Coalescer component
  to speed up sequential file access when using FatFs with this component
+ high-speed mode (>25MHz clock) is currently disabled for stability reasons.
+ in the presence of errors on the communication lines, SdMem currently freezes.

You may choose the SdMem debugging level, by appending for example
CCOPT+=-DDRV_SDMEM_DEBUG=4 to your projects makefile:

+ Level 0: No Debug
+ Level 1: Critical Errors
+ Level 2: Warnings
+ Level 3: Notice
+ Level 4: Verbose (affects program flow significantly)
+ Level 5: Even more verbose

Releases should be done at level 0
Development should be done at level >= 2

SdMem and FatFs and their dependencies probably won't all fit
into the 32 kilobyte LRAM, especially if you enable debugging. 
The correct way to put SdMem, and FatFs into CMX is to use a custom
ldscript. For an example of how to do it see
testApps/components/SDHostFatFsTest/scripts/SDHostFatFsTest.ldscript

wich contains something like:

@code
SECTIONS {
    . = __CMX_PHYS;
    .cmx.drvCprText : {
        ./output/obj/drvCpr.o(.text.*)
    }
    .cmx.drvTimerText : {
        ./output/obj/drvTimer.o(.text.*)
    }
    .cmx.sdMemText : {
        ./output/obj/sdMem.o(.text.*)
    }
    .cmx.FatFsText : {
        ./output/obj/ff.o(.text.*)
    }
}

INCLUDE Myriad1_default_memory_map.ldscript
@endcode

Please note that the specified object filename must be exactly what
the 'sparc-elf-ld' receives on its command line, otherwise it thinks
it's a different file, and tries to link it twice.

*/

#ifndef SD_MEM_H
#define SD_MEM_H

#include "mv_types.h"
#include "swcLeonUtils.h"

#ifndef SDMEM_INTERRUPT_LEVEL
#define SDMEM_INTERRUPT_LEVEL 14
#endif

/// @brief Stricture to specify a range of pins that need to be configured with the same mode.
typedef struct {
    int start; ///< First GPIO pin in pin range
    int end;   ///< Last GPIO pin in pin range (inclusive)
    int mode;  ///< Mode to set the gpio, to operate as HSDIO.
} tySdMemPinRange;

/// @brief SDCard Slot configuration structure
typedef struct {
    int hostPeripheralIndex; ///< Which peripheral is this slot connected to (1..2)
    int slotIndex; ///< The slot index withing the peripheral (1..2).
    int availableBusBits; ///< Number of available data bus bits: 1, 4, or 8. note: 8-bit mmc is not yet implemented
    int cardDetectPinAvailabe; ///< Is the card detect pin connected?
    int writeProtectPinAvailable; ///< Is the write-protect ping connected?
    int maximumSdClockKhz; ///< Maximum SD clock frequency supported by this slot
    int maximumCurrentConsumption; ///< Maximum current consumption in mA
    int allowHighSpeedTiming; ///< Should the High Speed Enable bit
    ///< of the SD Host peripheral be used? If true, then CMD and DAT
    ///< lines will be output on the rising edge IF the card supports
    ///< high speed mode, and if the frequency is switched to > 25MHz
    ///< If you want to disable high speed mode completely, then set
    ///< maxmumumSdClockKHz <= 25000;
    u32 requiredVoltageWindow; ///< a bitmap of the voltage ranges that are required for the card to support.
    ///< see the macros D_SDIO_VDD_*_* in the file DrvHsdioDefines.h
    tySdMemPinRange pins[]; ///< List of pin ranges, to configure. The list ends with {-1,-1,-1}
} tySdMemSlotCfg;

/// @brief SDCard Slot State structure
typedef struct {
    const tySdMemSlotCfg *slotCfg; ///< A pointer to the slot configuration structure.
    u32 base; ///< Host peripheral base register address (used for drv_hsdio functions)
    u32 sbase; ///< Slot base register address (used for direct register access)
    u32 slot; ///< 0-based slot 0..1 (used for drv_hsdio functions)
    int hostPeripheralClockKhz; ///< Frequency of the host peripheral, after dividing down from the PLL.
    float systemToPeripheralClockRatio; ///< The ratio between the system clock , and the peripheral clock
    ///< Used to calculate sleep timeouts when they are specified in SDClock clock cycles.
    int identificationDivider; ///< The SD peripheral-internal divider value to achieve a frequency that
    ///< corresponds to the SDcard identification frequency
    int dataTransferDivider; ///< The SD peripheral-internal divider value that corresponds to a normal
    ///< data transfer speed
    int highSpeedDivider; ///< The SD peripheral-internal divider value that corresponds to the
    ///< high-speed mode data transfer frequency.
    int MMC20MHzDivider; ///< The SD peripheral-internal divider value that corresponds to
    ///< the MMC 20 MHz frequency limitation.
    int currentDivider; ///< What the SD peripheral-internal divider is currently configured to.
    int writeProtected; ///< Is the card write-protected?
    // ^ TODO: use this!
    int cardInitialized; ///< Is the SD card initialized?
    int noResponseToCMD8; ///< Some cards respond to CMD8, and some don't. This shows what kind of card we have initialized.
    int noResponseToACMD41; ///< MMC cards don't support ACMD41
    u32 OCR_reg; ///< The contents of the OCR register
    int sectorAccess; ///< Card Capacity Status (are sectors byte-addressed, or index-addressed)
    u32 stuffed_RCA; ///< Relative Card Address register, includeing the 16 leas-siginficant zero-stuffbits
    u32 CSD_reg[4]; ///< The Card-Specific Data register
    u32 sectorCount; ///< Total number of sectors the SDcard has
    u32 CID_reg[4]; ///< The Card IDentification register
    u32 SD_SCR[2]; ///< SCR register (SD CARD Configuration register)
    int cardSupports4bit; ///< Does the card support 4-bit bus mode?
    int SD_SPEC; ///< SD_SPEC from SD_SCR
    u32 supportedFunctions[6]; ///< CMD6 functions supported by the SD card
    int irq_number; ///< The IRQ number of the current peripheral.
    void (*irq_handler) (u32 source); ///< Function pointer to the IRQ handler.
    volatile int dataTransferCmdFinished; ///< Signaling from the interrupt handler to the looping read/write function.
} tySdMemSlotState;

/// @brief Slot configuration of the slot on the Mv0153 board
extern const tySdMemSlotCfg SdMemMv0153;
/// @brief Slot configuration of the slot on the Mv0117 board
extern const tySdMemSlotCfg SdMemMv0117; // TODO not yet tested

/// @brief Setup SD card slot.
///
/// @warning Before calling sdMemSetupSlot, clocks must already be set up by drvCpr!
/// @param[out] slotState slot state structure to intialize that will be given to all other sdMem functions
/// @param[in]  slotCfg slot configuration. For an example see #SdMemMv0153.
void sdMemSetupSlot(tySdMemSlotState *slotState, const tySdMemSlotCfg *slotCfg);

/// @brief Initialize SD card.
///
/// @param[in] slotState - slot that has been previously initialized with sdMemSetupSlot
/// @return    
/// +          0,  on success;
/// +          -1, on setup error;
/// +          -2, on no card;
/// +          -3, on unsupported response to CMD8;
/// +          -4, on unsupported card voltage range;
/// +          -5, on initialization timeout while waiting for OCR->busy to turn 1;
/// +          0x100 + CMDNR, if there was an unrecoverable error caused by command CMDNR;
/// +          0x200 + ACMDNR, if there was an unrecoverable error caused by app-specific command ACMDNR;
/// +          other non-zero value, on other error
int sdMemCardInit(tySdMemSlotState *slotState);

/// @brief Detect if there is a card in the slot
///
/// @param[in] slotState slot that has been previously initialized with sdMemSetupSlot
/// @return non-zero if card detect pin is available, and if the pin is stable, and if the slot is empty, 0 otherwise
int sdMemNoCard(tySdMemSlotState *slotState);

/// @brief Return the sector size of the sd card.
///
/// @param[in] slotState slot that has been previously initialized with sdMemSetupSlot
/// @return the size of a sector in bytes
u32 sdMemGetSectorSize(tySdMemSlotState *slotState);

/// @brief Returns the number of sectors on the SD card
///
/// @param[in] slotState slot that has been previously initialized with sdMemSetupSlot
/// @return 0 on error
u32 sdMemGetSectorCount(tySdMemSlotState *slotState);

/// @brief Get the erase block size
///
/// @param[in] slotState slot that has been previously initialized with sdMemSetupSlot
/// @return the size of a sector in bytes
u32 sdMemGetEraseBlockSize(tySdMemSlotState *slotState);

/// @brief Erase a sector
///
/// Not implemented
int sdMemEraseSector(tySdMemSlotState *slotState, u32 start_sector, u32 end_sector);

/// @brief Flush out cached data to the sd card.
///
/// At the moment this is a no-op.
/// @param[in] slotState slot that has been previously initialized with sdMemSetupSlot
int sdMemSync(tySdMemSlotState *slotState);

/// @brief Write a number of sectors to the SDcard.
///
/// @param[in] slotState slot that has been previously initialized with sdMemSetupSlot
/// @param[in] Buffer Pointer to the data that is being written
/// @param[in] SectorNumber The index of the first sector to write
/// @param[in] SectorCount The number of sectors to write
/// @param[in] endianness Endianness of the buffer
int sdMemWrite(tySdMemSlotState *slotState, const unsigned char *Buffer, u32 SectorNumber, u32 SectorCount, pointer_type endianness);

/// @brief Read a number of sectors from the SDcard.
///
/// @param[in] slotState slot that has been previously initialized with sdMemSetupSlot
/// @param[out] Buffer Pointer to the buffer where the data will be put
/// @param[in] SectorNumber The index of the first sector to read
/// @param[in] SectorCount The number of sectors to read
/// @param[in] endianness Endianness of the buffer
int sdMemRead(tySdMemSlotState *slotState, unsigned char *Buffer, u32 SectorNumber, u32 SectorCount, pointer_type endianness);

// TODO some way to detect plug/unplug


/// @}
#endif

