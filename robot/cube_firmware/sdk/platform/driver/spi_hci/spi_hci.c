/**
 ****************************************************************************************
 *
 * @file spi_hci.c
 *
 * @brief SPI Driver for HCI over SPI operation.
 *
 * $Rev: 4 $
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */
 
 /**
 ****************************************************************************************
 * @addtogroup SPI
 * @{
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */
#include "spi_hci.h"        // 5-wire SPI driver header file
#include "reg_spi.h"        // SPI register access functions
#include <stdint.h>
#include <core_cm0.h>
#include "arch.h"
#include "datasheet.h"
#ifdef CFG_SPI_RX_FLOW_CONTROL
#ifdef CFG_GTL_SPI
#include "gtl_env.h"
#endif
#ifdef CFG_HCI_SPI
#include "hcic.h"
#endif
#endif

/*
 * STRUCT DEFINITIONS
 *****************************************************************************************
 */
/* TX and RX channel class holding data used for asynchronous read and write data
 * transactions
 */
/// SPI TX RX Channel
struct spi_txrxchannel
{
    /// size
    uint32_t  size;
    /// buffer pointer
    uint8_t  *bufptr;
    /// call back function pointer
    void (*callback) (uint8_t);
};

/// SPI environment structure
struct spi_env_tag
{
    /// tx channel
    struct spi_txrxchannel tx;
    /// rx channel
    struct spi_txrxchannel rx;
};

/*
 * GLOBAL VARIABLE DEFINITIONS
 ****************************************************************************************
 */
/// SPI environment structure
static struct spi_env_tag spi_env __attribute__((section("retention_mem_area0"),zero_init)); //@RETENTION MEMORY

/*
 * LOCAL FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Serves the receive data interrupt requests. It reads incoming data from SPI
 *        while SPI_INT_BIT is 1, and saves them to the receive buffer pointer.
 *
 ****************************************************************************************
 */
static void spi_receive_data_isr(void)
{
    void (*callback) (uint8_t) = NULL;
    
    // Read available received data to SPI environment buffer pointer
    while (spi_data_rdy_getf())
    {
        // Read the received byte in the FIFO
        *spi_env.rx.bufptr = spi_read_byte();
        
        // Update RX parameters
        spi_env.rx.size--;
        spi_env.rx.bufptr++;
        
        // Check if all expected data have been received
        if (spi_env.rx.size == 0)
        {
            // Disable TX interrupt
            NVIC_DisableIRQ(SPI_IRQn);
            
            // Retrieve callback pointer
            callback = spi_env.rx.callback;
            
            if(callback != NULL)
            {
                // Clear callback pointer
                spi_env.rx.callback = NULL;

#ifdef CFG_SPI_RX_FLOW_CONTROL
                /**
                 * Assert dready to prevent the master from sending more bytes
                 * while the callback has not been completed, if the reception
                 * of the header has been completed and the payload size is 
                 * larger than 8 bytes.
                 */
#ifdef CFG_GTL_SPI
                if ( (gtl_env.rx_state == GTL_STATE_RX_HDR) &&
                     ( (*(spi_env.rx.bufptr-1)<<8) + (*(spi_env.rx.bufptr-2) > 8) ) )
#endif
#ifdef CFG_HCI_SPI
                if ( (hci_env.rx_state == HCI_STATE_RX_HDR) &&
                     ( *(spi_env.rx.bufptr-1) > 8  ) )
#endif
                {
                    spi_dready_high();
                }
#endif
                                
                // Reset RX parameters
                spi_env.rx.bufptr = NULL;
                
                // Call handler
                callback(SPI_STATUS_OK);
                
#ifdef CFG_SPI_RX_FLOW_CONTROL
                // De-assert dready to continue the current transmission
                spi_dready_low();
#endif
            }
            else
            {
                ASSERT_ERR(0);
            }
            
            // Exit loop
            break;
        }
    }
}


/**
 ****************************************************************************************
 * @brief Serves the transmit data requests. It asserts the DREADY signal to request a
 *        transmit to the master and when the master provides a clock signal the data
 *        transaction is performed. Upon completion, it de-asserts the DREADY signal.
 *
 ****************************************************************************************
 */
static void spi_transmit_isr(void)
{
    uint8_t tmp;
    void (*callback) (uint8_t) = NULL;
    
    while(!spi_cs_getf());          // Polling CS to detect if data is being received
    
    NVIC_DisableIRQ(SPI_IRQn);      // Disable SPI interrupt to CPU
    
    spi_dready_high();            // Assert dready to request transmit
    
    do {
        spi_wait_for_transfer();      // Wait for SPI transaction from SPI master device
        tmp = spi_rxtxreg_read();     // Get byte from SPI
    } while (tmp != DREADY_ACK);      // If DREADY is not acknowledged, try again
    
    spi_int_bit_clear();          // Clear pending flag
    
    // Write bytes to be transmitted
    while(spi_env.tx.size)
    {
        // Put a byte in the TX FIFO
        spi_write_byte(*spi_env.tx.bufptr);
        
        // Update TX parameters
        spi_env.tx.size--;
        spi_env.tx.bufptr++;
        
        if (spi_env.tx.size == 0)
        {
            while (GetBits16(SPI_CTRL_REG1, SPI_BUSY) == 1) // Wait while SPI is busy
            {
                if (spi_data_rdy_getf())  // Still data in the SPI RX FIFO
                {
                    spi_rxtxreg_read();     // Get byte from SPI RX FIFO
                    spi_int_bit_clear();    // Clear pending flag
                }
            }

            spi_dready_low();           // De-assert DREADY signal, transmit completed
            
            // Empty SPI Receive FIFO
            while(spi_data_rdy_getf())  // Still data in the SPI RX FIFO
            {
                spi_rxtxreg_read();     // Get byte from SPI RX FIFO
                spi_int_bit_clear();    // Clear pending flag
            }
            spi_int_bit_clear();        // Clear pending flag
            			
            // Reset TX parameters
            spi_env.tx.bufptr = NULL;
            
            // Retrieve callback pointer
            callback = spi_env.tx.callback;
            
            if(callback != NULL)
            {
                // Clear callback pointer
                spi_env.tx.callback = NULL;
                
                // Call handler
                callback(SPI_STATUS_OK);
            }
            else
            {
                ASSERT_ERR(0);
            }
            
            // Exit loop
            break;
        }
    }
    NVIC_ClearPendingIRQ(SPI_IRQn); // clear interrupt requests while disabled
    NVIC_EnableIRQ(SPI_IRQn);       // Enable SPI interrupt to CPU
}

/**
 ****************************************************************************************
 * @brief Initialization function of the SPI module. 
 *
 * Initializes SPI module as slave and enables SPI FIFO mode.
 ****************************************************************************************
 */
void spi_hci_slave_init(void)
{
    SetBits16(CLK_PER_REG, SPI_DIV, 0);           // Set SPI internal clock divider
    SetBits16(CLK_PER_REG, SPI_ENABLE, 1);        // Enable clock for SPI
    
    // Initialize SPI module
    SetBits16(SPI_CTRL_REG,SPI_ON,0);             // Close SPI module, if opened
    SetBits16(SPI_CTRL_REG,SPI_WORD,0);           // Set to 8-bit mode
    SetBits16(SPI_CTRL_REG,SPI_SMN, 0x01);        // Set SPI in SLAVE mode
    SetBits16(SPI_CTRL_REG,SPI_POL, 0x0);         // Mode 3: SPI_POL = 0
    SetBits16(SPI_CTRL_REG,SPI_PHA, 0x0);         //     and SPI_PHA = 0
    SetBits16(SPI_CTRL_REG,SPI_MINT, 0x1);        // Enable SPI Maskable Interrupt to CPU
    SetBits16(SPI_CTRL_REG1,SPI_FIFO_MODE,0x00);  // Enable SPI RX and TX FIFOs
    SetBits16(SPI_CTRL_REG,SPI_EN_CTRL,1);        // Enable SPI EN pin for slave mode
    SetBits16(SPI_CTRL_REG,SPI_ON,1);             // Enable SPI module
    
    // Configure SPI environment
    spi_env.rx.bufptr = NULL;
    spi_env.rx.size = 0;
    spi_env.tx.bufptr = NULL;
    spi_env.tx.size = 0;
    NVIC_SetPriority(SPI_IRQn,0);
}

/**
 ****************************************************************************************
 * @brief Flow off function of the SPI external interface API. 
 *
 * Calls spi_hci_set_flow() with on=1, clears pending SPI IRQs and enables SPI interrupts.
 ****************************************************************************************
 */
void spi_hci_flow_on_func(void)
{
    uint8_t tmp;
       
    // Wait if transmission is ongoing
    while(!spi_cs_getf());
    
    NVIC_DisableIRQ(SPI_IRQn);    // Disable SPI interrupt to CPU
    
    spi_dready_high();            // Assert dready to request transmit
    
    do {
        spi_wait_for_transfer();      // Wait for SPI transaction from SPI master device
        tmp = spi_rxtxreg_read();     // Get byte from SPI
    } while (tmp != DREADY_ACK);      // If DREADY is not acknowledged, try again
    
    spi_int_bit_clear();          // Clear pending flag
    
    while (spi_txfifo_full_getf());   // Wait if SPI TX FIFO is full
    
    spi_rxtxreg_write(FLOW_ON_BYTE);   // Send flow on byte

    spi_wait_for_transfer();   // Wait for byte to be transmitted
    spi_rxtxreg_read();        // Read received byte in SPI Rx FIFO
    spi_int_bit_clear();       // Clear pending flag
    while (spi_busy_getf());   // Wait while SPI is busy
    spi_dready_low();          // De-assert DREADY signal
    
    NVIC_ClearPendingIRQ(SPI_IRQn);  // Clear interrupt requests while disabled
    NVIC_EnableIRQ(SPI_IRQn);        // Disable SPI interrupt to CPU, if MINT is '1'
}

/**
 ****************************************************************************************
 * @brief Flow off function of the SPI external interface API. 
 *
 * Calls spi_hci_set_flow() with on=0.
 ****************************************************************************************
 */
bool spi_hci_flow_off_func(void)
{
    uint8_t tmp;
       
    // First check if no transmission is ongoing
    if((spi_cs_getf()==0))
    {
        return false;
    }
    
    NVIC_DisableIRQ(SPI_IRQn);    // Disable SPI interrupt to CPU
    
    spi_dready_high();            // Assert dready to request transmit
    
    do {
        spi_wait_for_transfer();      // Wait for SPI transaction from SPI master device
        tmp = spi_rxtxreg_read();     // Get byte from SPI
    } while (tmp != DREADY_ACK);      // If DREADY is not acknowledged, try again
    
    spi_int_bit_clear();          // Clear pending flag
    
    while (spi_txfifo_full_getf());   // Wait if SPI TX FIFO is full
    
    spi_rxtxreg_write(FLOW_OFF_BYTE);  // Send flow off byte
       
    spi_wait_for_transfer();   // Wait for byte to be transmitted
    spi_rxtxreg_read();        // Read received byte in SPI Rx FIFO
    spi_int_bit_clear();       // Clear pending flag
    while (spi_busy_getf());   // Wait while SPI is busy
    spi_dready_low();          // De-assert DREADY signal
    
    return true;
}

/**
 ****************************************************************************************
 * @brief Read function of the SPI external interface API. 
 *
 * @param[in] *bufptr:   Pointer to the memory location the read data will be stored.
 * @param[in] size:      Size of the data to be read in bytes.
 * @param[in] *callback: Pointer to the callback function.
 ****************************************************************************************
 */
void spi_hci_read_func(uint8_t *bufptr, uint32_t size,void (*callback) (uint8_t))
{
    // Sanity check
    ASSERT_ERR(bufptr != NULL);
    ASSERT_ERR(size != 0);
    ASSERT_ERR(spi_env.rx.bufptr == NULL);
    
    // Prepare RX parameters
    spi_env.rx.size = size;
    spi_env.rx.bufptr = bufptr;
    spi_env.rx.callback = callback; 

    // Enable SPI interrupt
    NVIC_EnableIRQ(SPI_IRQn);
      
}

/**
 ****************************************************************************************
 * @brief Write function of the SPI external interface API. 
 *
 * @param[in] *bufptr:   Pointer to the memory location of the data to be written.
 * @param[in] size:      Size of the data to be written in bytes.
 * @param[in] *callback: Pointer to the callback function.
 ****************************************************************************************
 */
void spi_hci_write_func(uint8_t *bufptr, uint32_t size, void (*callback) (uint8_t))
{
    // Sanity check
    ASSERT_ERR(bufptr != NULL);
    ASSERT_ERR(size != 0);
    ASSERT_ERR(spi_env.tx.bufptr == NULL);	
    
    // Prepare TX parameters
    spi_env.tx.size = size;
    spi_env.tx.bufptr = bufptr;
    spi_env.tx.callback = callback; 
    
    /* start data transaction */
    
    while(!spi_hci_flow_off_func());    // Send flow off byte
    spi_transmit_isr();         // Send data over SPI
    spi_hci_flow_on_func();     // Send flow on byte
}

/**
 ****************************************************************************************
 * @brief Exported function for SPI interrupt handler. 
 ****************************************************************************************
 */
void SPI_Handler(void)
{
    spi_receive_data_isr();
}
