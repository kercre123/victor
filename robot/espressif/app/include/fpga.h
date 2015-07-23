/** @file Espressif Interface to the Lattice FPGA on the head board
 * @author Daniel Casner <daniel@anki.com>
 * Convenience functions for interfacing with the FPGA
 */
#ifndef FPGA_APP_H
#define FPGA_APP_H

#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"

/** Initalizes the interface with the FPGA
 * FPGA will be in reset state after init
 * Note the spi_master_init must also be called separately for everything to work
 */
int8_t fpgaInit(void);

/// Put the FPGA into reset state.
void fpgaDisable(void);

/// Allow the FPGA to boot into programming mode
void fpgaEnable(void);

/** Write program data to the FPGA
 * This function blocks until all the data has been written
 * @param data A pointer to SPI_FIFO_DEPTH 32 bit words of program data.
 * @return true on success or false if an error occured
 */
bool fpgaWriteBitstream(uint32* data);


#endif
