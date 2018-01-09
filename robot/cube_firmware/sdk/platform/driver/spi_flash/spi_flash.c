/**
 ****************************************************************************************
 *
 * @file spi_flash.c
 *
 * @brief flash driver over spi interface.
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


#include "spi_flash.h"

// local copy of FLASH setup parameters
int16_t spi_flash_device_index;
const SPI_FLASH_DEVICE_PARAMETERS_BY_JEDEC_ID_t *spi_flash_detected_device;
uint32_t spi_flash_size;         
uint32_t spi_flash_page_size;

const SPI_FLASH_DEVICE_PARAMETERS_BY_JEDEC_ID_t SPI_FLASH_KNOWN_DEVICES_PARAMETERS_LIST[] = 
{
	{W25X10_JEDEC_ID, W25X10_JEDEC_ID_MATCHING_BITMASK, W25X10_TOTAL_FLASH_SIZE, W25X10_PAGE_SIZE, W25x_MEM_PROT_BITMASK, W25x10_MEM_PROT_NONE},
	{W25X20_JEDEC_ID, W25X20_JEDEC_ID_MATCHING_BITMASK, W25X20_TOTAL_FLASH_SIZE, W25X20_PAGE_SIZE, W25x_MEM_PROT_BITMASK, W25x20_MEM_PROT_NONE},
	{AT25Dx011_JEDEC_ID, AT25Dx011_JEDEC_ID_MATCHING_BITMASK, AT25Dx011_TOTAL_FLASH_SIZE, AT25Dx011_PAGE_SIZE, AT25Dx011_MEM_PROT_BITMASK, AT25Dx011_MEM_PROT_NONE},
	{MX25V1006E_JEDEC_ID, MX25V1006E_JEDEC_ID_MATCHING_BITMASK, MX25V1006E_TOTAL_FLASH_SIZE, MX25V1006E_PAGE_SIZE, MX25V1006E_MEM_PROT_BITMASK, MX25V1006E_MEM_PROT_NONE},
};

/**
 ****************************************************************************************
 * @brief Initialize SPI Flash
 * @param[in ]spi_flash_size_param:         Flash Size
 * @param[in] spi_flash_page_size_param:    Flash Page Size
 ****************************************************************************************
 */
void spi_flash_init(uint32_t spi_flash_size_param, uint32_t spi_flash_page_size_param)
{
	spi_flash_size = spi_flash_size_param;
	spi_flash_page_size = spi_flash_page_size_param;
}

/**
 ****************************************************************************************
 * @brief Detect the SPI flash device, based on the JEDEC manufacturer id and the		
 *		  manufacturer-defined data(2 bytes) which is retrieved when the command 9Fh
 *		  is issued. If the device is successfully identified, the total memory size
 *		  and page size are retrieved from a lookup table.
 * @return the index of the device in the SPI_FLASH_KNOWN_DEVICES_PARAMETERS_LIST or
 *         (SPI_FLASH_AUTO_DETECT_NOT_DETECTED) if the device is not found
 ****************************************************************************************
 */
int8_t spi_flash_auto_detect(void)
{
	uint32_t jedec_id;
	uint16_t i;
	
	jedec_id = spi_read_flash_jedec_id();
	for (i=0; i<SPI_FLASH_DEVICES_SUPPORTED_COUNT; i++)
	{
		if ( (jedec_id & SPI_FLASH_KNOWN_DEVICES_PARAMETERS_LIST[i].jedec_id_matching_bitmask) ==\
				 (SPI_FLASH_KNOWN_DEVICES_PARAMETERS_LIST[i].jedec_id & SPI_FLASH_KNOWN_DEVICES_PARAMETERS_LIST[i].jedec_id_matching_bitmask) )
		{
			spi_flash_device_index = i;
			spi_flash_size = SPI_FLASH_KNOWN_DEVICES_PARAMETERS_LIST[i].flash_size;
			spi_flash_page_size = SPI_FLASH_KNOWN_DEVICES_PARAMETERS_LIST[i].page_size;
			spi_flash_detected_device = &SPI_FLASH_KNOWN_DEVICES_PARAMETERS_LIST[spi_flash_device_index];
			return i;
		}	
	}
	// SPI flash device not recognized
	spi_flash_detected_device = 0;	
	return SPI_FLASH_AUTO_DETECT_NOT_DETECTED;
}

/**
 ****************************************************************************************
 * @brief Read Status Register
 * @return  Status Register value
 ****************************************************************************************
 */
uint8_t spi_flash_read_status_reg(void)
{
	//no 'add spi_flash_wait_till_ready()' here
	spi_set_bitmode(SPI_MODE_16BIT);                          // set SPI bitmode to 16-bit      
	return spi_transaction((uint16_t)(READ_STATUS_REG<<8));
}

/**
 ****************************************************************************************
 * @brief Wait till flash is ready for next action 
* @return  Success : ERR_OK
*          Failure : ERR_TIMEOUT 
 ****************************************************************************************
 */
int8_t spi_flash_wait_till_ready (void)
{
	uint32_t statusReadCount;
	for (statusReadCount = 0; statusReadCount < MAX_READY_WAIT_COUNT; statusReadCount++)
	{
		if ((spi_flash_read_status_reg() & STATUS_BUSY) == 0)
			return ERR_OK;
	}
	return ERR_TIMEOUT;
}

/**
 ****************************************************************************************
 * @brief Issue a Write Enable Command  
 * @return error code or success (ERR_OK)  
 ****************************************************************************************
 */  
int8_t spi_flash_set_write_enable(void)
{
	uint32_t commandSendCount;
	uint32_t statusReadCount;
	uint8_t status;
	if (spi_flash_wait_till_ready() == ERR_OK)
	{
		spi_set_bitmode(SPI_MODE_8BIT);           // set SPI bitmode to 8-bit               
		for (commandSendCount = 0; commandSendCount < MAX_COMMAND_SEND_COUNT; commandSendCount++)   
		{        
			spi_transaction(WRITE_ENABLE);      // send instruction              
			for (statusReadCount = 0; statusReadCount < MAX_READY_WAIT_COUNT; statusReadCount++)
			{
				status = spi_flash_read_status_reg();
				if  ( ((status & STATUS_BUSY) == 0) && ((status & STATUS_WEL) != 0) ) 
					return ERR_OK;    
			}
		}
	}
	return ERR_TIMEOUT;
}

/**
 ****************************************************************************************
 * @brief Issue a Write Enable Volatile Command  
 * @return error code or success (ERR_OK)  
 ****************************************************************************************
 */  
int8_t spi_flash_write_enable_volatile(void)
{
	uint32_t commandSendCount;
	uint32_t statusReadCount;
	uint8_t status;
	if (spi_flash_wait_till_ready() == ERR_OK)
	{
		spi_set_bitmode(SPI_MODE_8BIT);           // set SPI bitmode to 8-bit               
		for (commandSendCount = 0; commandSendCount < MAX_COMMAND_SEND_COUNT; commandSendCount++)   
		{        
			spi_transaction(WRITE_ENABLE_VOL);          // send instruction              
			for (statusReadCount = 0; statusReadCount < MAX_READY_WAIT_COUNT; statusReadCount++)
			{
				status = spi_flash_read_status_reg();
				if  ( ((status & STATUS_BUSY) == 0) && ((status & STATUS_WEL) != 0) ) 
					return ERR_OK;    
			}
		}
	}
	return ERR_TIMEOUT;    
}

/**
 ****************************************************************************************
 * @brief Issue a Write Disable Command  
 * @return error code or success (ERR_OK)  
 ****************************************************************************************
 */  
int8_t spi_flash_set_write_disable(void)
{
	uint32_t commandSendCount;
	uint32_t statusReadCount;
	uint8_t status;
	if (spi_flash_wait_till_ready() == ERR_OK)
	{
		spi_set_bitmode(SPI_MODE_8BIT);           	// set SPI bitmode to 8-bit               
		for (commandSendCount = 0; commandSendCount < MAX_COMMAND_SEND_COUNT; commandSendCount++)   
		{
			spi_transaction(WRITE_DISABLE);         // send instruction              
			for (statusReadCount = 0; statusReadCount < MAX_READY_WAIT_COUNT; statusReadCount++)
			{
				status = spi_flash_read_status_reg();
				if ( ((status & STATUS_BUSY) == 0) && ((status & STATUS_WEL) == 0) ) 
					return ERR_OK;    
			}
		}
	}
	return ERR_TIMEOUT;    
}

/**
 ****************************************************************************************
 * @brief Write Status Register
 * @param[in] dataToWrite:   Value to be written to Status Register
 * @return error code or success (ERR_OK)
 ****************************************************************************************
 */
int32_t spi_flash_write_status_reg(uint8_t dataToWrite)
{
	int8_t spi_flash_status;
	spi_flash_status = spi_flash_wait_till_ready();
	if (spi_flash_status != ERR_OK)
		return spi_flash_status; // an error has occured        
  
	spi_set_bitmode(SPI_MODE_16BIT);    
	spi_transaction((WRITE_STATUS_REG<<8) | dataToWrite);     // send  Write Status Register-1 instruction
	return spi_flash_wait_till_ready();
}

/**
 ****************************************************************************************
 * @brief Read data from a given starting address (up to the end of the flash)
 *
 * @param[in] *rd_data_ptr:  Points to the position the read data will be stored
 * @param[in] address:       Starting address of data to be read
 * @param[in] size:          Size of the data to be read
 * 
 * @return  Number of read bytes or error code
 ****************************************************************************************
 */
int32_t spi_flash_read_data (uint8_t *rd_data_ptr, uint32_t address, uint32_t size)
{
	int8_t spi_flash_status;
	uint32_t bytes_read, i, temp_size;
	
	// check that all bytes to be retrieved are located in valid flash memory address space
	if (size + address > spi_flash_size)
	{
		temp_size = spi_flash_size - address;
		bytes_read = temp_size;
	}
	else
	{
		temp_size = size;
		bytes_read = size;
	}
    
	spi_flash_status = spi_flash_wait_till_ready();
	if (spi_flash_status != ERR_OK)
		return spi_flash_status; 						// an error has occured     

	spi_set_bitmode(SPI_MODE_32BIT);    
	spi_cs_low();            			            	// pull CS low    
	spi_access( (READ_DATA<<24) | address);             // Command for sequencial reading from memory		
	spi_set_bitmode(SPI_MODE_8BIT);   
	for(i=0; i<temp_size; i++)
	{
		*rd_data_ptr++ = (uint8_t)spi_access(0x0000);   // bare SPI transaction
	}
	spi_cs_high();               			            // push CS high
	return bytes_read;
}


/**
 ****************************************************************************************
 * @brief Program page (up to <SPI Flash page size> bytes) starting at given address
 *
 * @param[in] *wr_data_ptr:  Pointer to the data to be written
 * @param[in] address:       Starting address of data to be written
 * @param[in] size:          Size of the data to be written (should not be larger than SPI Flash page size)
 * @return error code or success (ERR_OK)
 ****************************************************************************************
 */
int32_t spi_flash_page_program(uint8_t *wr_data_ptr, uint32_t address, uint16_t size)
{
	int8_t spi_flash_status;
	uint16_t temp_size = size;
    	
	if (temp_size > spi_flash_page_size)                // check for max page size
		temp_size = spi_flash_page_size;
	
	spi_flash_status = spi_flash_wait_till_ready();
	if (spi_flash_status != ERR_OK)
		return spi_flash_status; 						// an error has occured   
  
	spi_flash_status = spi_flash_set_write_enable();    // send [Write Enable] instruction
	if (spi_flash_status != ERR_OK)
		return spi_flash_status; 						// an error has occured       
    
	spi_set_bitmode(SPI_MODE_32BIT);
	spi_cs_low();            			            	// pull CS low
	spi_access( (PAGE_PROGRAM<<24) | address);        	// Command for page programming
	spi_set_bitmode(SPI_MODE_8BIT);           
	while(temp_size>0)                                  // Write data bytes
	{
		spi_access(*wr_data_ptr++);
		temp_size--;
	}
	spi_cs_high();                                      // push CS high  
 	return spi_flash_wait_till_ready();
}


/**
 ****************************************************************************************
 * @brief Issue a command to Erase a given address
 *
 * @param[in] address:  Address that belongs to the block64/block32/sector range
 * @param[in] spiEraseModule: BLOCK_ERASE_64, BLOCK_ERASE_32, SECTOR_ERASE
 * @return error code or success (ERR_OK)
 ****************************************************************************************
 */
int8_t spi_flash_block_erase(uint32_t address, SPI_erase_module_t spiEraseModule)
{
	if (spi_flash_set_write_enable() != ERR_OK)         // send [Write Enable] instruction
		return ERR_TIMEOUT;

	spi_set_bitmode(SPI_MODE_32BIT);
	spi_transaction( (spiEraseModule<<24) | address);   // Command for erasing a sector    
	return spi_flash_wait_till_ready();                 
 }

/**
 ****************************************************************************************
 * @brief Erase chip
 * @note In order for the erasure to succeed, all locking options must be disabled.
 * @return error code or success (ERR_OK)
 ****************************************************************************************
 */
int8_t spi_flash_chip_erase(void)
{
	uint8_t status;
	
	if (spi_flash_set_write_enable() != ERR_OK)      // send [Write Enable] instruction
		return ERR_TIMEOUT;
	
	spi_set_bitmode(SPI_MODE_8BIT);
	spi_transaction(CHIP_ERASE);                    // Command for Chip Erase
	status = spi_flash_wait_till_ready();
	
	return status;
}


/**
 ****************************************************************************************
 * @brief Get Manufacturer / Device ID
 * @return  Manufacturer/Device ID (0 in case of time-out)
 ****************************************************************************************
 */
int16_t spi_read_flash_memory_man_and_dev_id(void)
{
	int8_t spi_flash_status;
	uint16_t idWord = 0;

	spi_flash_status = spi_flash_wait_till_ready();
	if (spi_flash_status == ERR_OK)
	{
		spi_set_bitmode(SPI_MODE_16BIT);
		spi_cs_low();            			// pull CS low
		spi_access(MAN_DEV_ID<<8);			// SPI transaction to send command
		spi_access(0x0000);         		// dummy   SPI transaction to send (A23-A0)
		idWord = spi_access(0x0000);		// SPI transaction to read Manufacturer Id, Device ID
		spi_cs_high();               		// push CS high
	}

	return idWord;
}



/**
 ****************************************************************************************
 * @brief Get Unique ID Number
 * @return  Unique ID Number (0 in case of time-out)
 ****************************************************************************************
 */
uint64_t spi_read_flash_unique_id(void)
{
	int8_t spi_flash_status;
	uint64_t unique_id = 0;
              
	spi_flash_status = spi_flash_wait_till_ready();
	if (spi_flash_status == ERR_OK)
	{
		spi_set_bitmode(SPI_MODE_8BIT);
		spi_cs_low();            				  // pull CS low
		spi_access(READ_UNIQUE_ID);               // SPI access to send [Read Unique ID] command
		spi_set_bitmode(SPI_MODE_32BIT);           // dummy transaction for the 4 dummy bytes
		spi_access(0x0000);                      // dummy bare SPI transaction
		unique_id = ((uint64_t)spi_access(0x0000) << 32);     // SPI access to get the high part of unique id
		unique_id |= (uint64_t)spi_access(0x0000);          // bare SPI access to get the high part of unique id
		spi_cs_high();                           // push CS high
	}

	return unique_id;	
}


/**
 ****************************************************************************************
 * @brief Get JEDEC ID
 * @return  JEDEC ID
 ****************************************************************************************
 */
int32_t spi_read_flash_jedec_id(void)
{
	int8_t spi_flash_status;   
	uint32_t jedec_id;
	
	spi_flash_status = spi_flash_wait_till_ready();
	if (spi_flash_status != ERR_OK)
		return spi_flash_status; // an error has occured      
    
	spi_set_bitmode(SPI_MODE_8BIT);   
	spi_cs_low();            		   	  // pull CS low
	spi_access(JEDEC_ID);                 //  SPI accsss to send [Read Unique ID] command   
	jedec_id = spi_access(0x0000) << 16;  //  SPI accsss to get the JEDEC Manufacturer ID
	jedec_id |= spi_access(0x0000) << 8;  //  SPI accsss to get device information byte 1
	jedec_id |= spi_access(0x0000);       //  SPI accsss to get device information byte 2 
	spi_cs_high();                        // push CS high  
	return jedec_id;	   
}

/**
 ****************************************************************************************
 * @brief Write data to flash across page boundaries and at any starting address
 *
 * @param[in] *wr_data_ptr:  Pointer to the data to be written
 * @param[in] address:       Starting address of page to be written (must be a multiple of SPI Flash page size)
 * @param[in] size:          Size of the data to be written (can be larger than SPI Flash page size)
 * 
 * @return  Number of bytes actually written
 ****************************************************************************************
 */
int32_t spi_flash_write_data (uint8_t *wr_data_ptr, uint32_t address, uint32_t size)
{
	uint32_t bytes_written; 
	uint32_t feasible_size = size;
	uint32_t currentAddress = address;
	uint32_t currentEndOfPage = (currentAddress / spi_flash_page_size + 1) * spi_flash_page_size - 1;
	uint32_t bytes_left_to_send;

	spi_set_bitmode(SPI_MODE_8BIT);
    
  	// limit to the maximum count of bytes that can be written to a (SPI_FLASH_SIZE x 8) flash
	if (size > spi_flash_size - address)
		feasible_size = spi_flash_size - address;
 
	bytes_left_to_send = feasible_size;
	bytes_written = 0;
	
	while (bytes_written < feasible_size)
	{
		// limit the transaction to the upper limit of the current page
		if (currentAddress + bytes_left_to_send > currentEndOfPage)
			bytes_left_to_send = currentEndOfPage - currentAddress + 1;             
		if (spi_flash_page_program(wr_data_ptr + bytes_written, currentAddress, bytes_left_to_send) != ERR_OK) //write the current page data
			return ERR_TIMEOUT;
		bytes_written += bytes_left_to_send;                                                     
		currentAddress = currentEndOfPage + 1;  //address points to the first memory position of the next page
		currentEndOfPage += spi_flash_page_size;
		bytes_left_to_send = feasible_size - bytes_written;
	}
	return bytes_written;
}


/**
 ****************************************************************************************
 * @brief Sends the Power-Down instruction
 * Remark: The function spi_flash_release_from_powerdown() is used to enable the IC again
 * The power-down state will be entered tDP (3uS for W25X10CL) after CS is returned to high.
 ****************************************************************************************
 */
int32_t spi_flash_power_down(void)
{
	int8_t spi_flash_status;
    
	spi_flash_status = spi_flash_wait_till_ready();
	if (spi_flash_status != ERR_OK)
		return spi_flash_status;    // an error has occured   
    
	spi_set_bitmode(SPI_MODE_8BIT);    
	spi_transaction(POWER_DOWN);    // SPI transaction to Power-down the SPI Flash IC 
    
	return ERR_OK;
}

/**
 ****************************************************************************************
 * @brief Sends the Release from Power-Down instruction
 * Remark: This function is used to restore the IC from power-down mode
 * You must ensure that the CS line will stay high after this instruction is sent for 
 * at least tRES1 DP (3uS for W25X10CL).
 ****************************************************************************************
 */
int32_t spi_flash_release_from_power_down(void)
{
	spi_set_bitmode(SPI_MODE_8BIT);    
	spi_transaction(REL_POWER_DOWN);            // SPI transaction to Power-down the SPI Flash IC 
	return ERR_OK;    
}

/**
 ****************************************************************************************
 * @brief Selects the memory protection configuration
 * @param[in] SPI_flash_memory_protection_setting
 * @return error code or success (ERR_OK)
 ****************************************************************************************
 */
int32_t spi_flash_configure_memory_protection(uint8_t spi_flash_memory_protection_setting)
{
	if (spi_flash_detected_device == 0)
		return ERR_UNKNOWN_FLASH_VENDOR;        // cannot configure memory protection for an unknown device	
	
	if (spi_flash_set_write_enable() != ERR_OK) // send [Write Enable] instruction
		return ERR_TIMEOUT;   
	
	return spi_flash_write_status_reg((spi_flash_read_status_reg() & (~spi_flash_detected_device->memory_protection_bitmask)) |\
		((spi_flash_memory_protection_setting)&(spi_flash_detected_device->memory_protection_bitmask)));        
}

/**
 ****************************************************************************************
 * @brief Erase chip, removing all memory areas protection prior to erasure
 * @param forced: Protection for the whole memory array is removed prior to chip erasure
 * @return error code or success (ERR_OK)
 ****************************************************************************************
 */
int8_t spi_flash_chip_erase_forced(void)
{
	int32_t result;
	
	// Memory protection for the entire memory array is removed prior to chip erasure
	result = spi_flash_configure_memory_protection(spi_flash_detected_device->memory_protection_unprotected);

    // if memory protection has not been removed(e.g. JEDEC id not recognized), return with error code
	if (result != ERR_OK)
		return result;

	return spi_flash_chip_erase();
}

/**
 ****************************************************************************************
 * @brief Fill memory page (up to <SPI Flash page size> bytes) with a given 1-byte value
 *        starting at given address
 *
 * @param[in] value:         Value used to fill memory
 * @param[in] address:       Starting address
 * @param[in] size:          Size of the area to be filled (should not be larger than SPI Flash page size)
 * @return error code or success (ERR_OK)
 ****************************************************************************************
 */
int8_t spi_flash_page_fill(uint8_t value, uint32_t address, uint16_t size)
{
	int8_t spi_flash_status;
	uint16_t temp_size = size;
    	
	if (temp_size > spi_flash_page_size)                // check for max page size
		temp_size = spi_flash_page_size;
	
	spi_flash_status = spi_flash_wait_till_ready();
	if (spi_flash_status != ERR_OK)
		return spi_flash_status; 						// an error has occured   

	spi_flash_status = spi_flash_set_write_enable();    // send [Write Enable] instruction
	if (spi_flash_status != ERR_OK)  
		return spi_flash_status; // an error has occured       
	
	spi_set_bitmode(SPI_MODE_32BIT);
	spi_cs_low();            			            	// pull CS low
	spi_access( (PAGE_PROGRAM<<24) | address);          // Command for page programming
	spi_set_bitmode(SPI_MODE_8BIT);           
	while(temp_size>0)                                  // Write data bytes
  	{
		spi_access(value);
		temp_size--;
	}
	spi_cs_high();                                      // push CS high  
	return spi_flash_wait_till_ready();
}

/**
 ****************************************************************************************
 * @brief Fill memory with a 1-byte value, across page boundaries and at any starting address
 *
 * @param[in] value:    The value with which memory will be filled
 * @param[in] address:  Starting address of page to be written (must be a multiple of SPI Flash page size)
 * @param[in] size:     Size of the area to be filled (can be larger than SPI Flash page size)
 * 
 * @return  Number of bytes actually written
 ****************************************************************************************
 */
int32_t spi_flash_fill (uint8_t value, uint32_t address, uint32_t size)
{
	uint32_t bytes_written; 
	uint32_t feasible_size = size;
	uint32_t currentAddress = address;
	uint32_t currentEndOfPage = (currentAddress / spi_flash_page_size + 1) * spi_flash_page_size - 1;
	uint32_t bytes_left_to_send;

	spi_set_bitmode(SPI_MODE_8BIT);
    
	// limit to the maximum count of bytes that can be written to a (SPI_FLASH_SIZE x 8) flash
	if (size > spi_flash_size - address)
		feasible_size = spi_flash_size - address;
 
	bytes_left_to_send = feasible_size;
	bytes_written = 0;
    
	while (bytes_written < feasible_size)
	{
		// limit the transaction to the upper limit of the current page
		if (currentAddress + bytes_left_to_send > currentEndOfPage)
			bytes_left_to_send = currentEndOfPage - currentAddress + 1;             
		if (spi_flash_page_fill(value, currentAddress, bytes_left_to_send) != ERR_OK) //write the current page data
			return ERR_TIMEOUT;
		bytes_written += bytes_left_to_send;                                                     
		currentAddress = currentEndOfPage + 1;  //address points to the first memory position of the next page
		currentEndOfPage += spi_flash_page_size;
		bytes_left_to_send = feasible_size - bytes_written;
	}
	return bytes_written;
}

/**
 ****************************************************************************************
 * @brief Initializes spi and spi_flash drivers, discovers jedec id and releases from power down
 *
 * @param[in] cs_port:  Chip select port
 * @param[in] cs_pin:   Chip select pin
 * 
 * @return  Number of bytes actually written
 ****************************************************************************************
*/
int8_t spi_flash_enable(GPIO_PORT cs_port, GPIO_PIN cs_pin)
{
    int8_t detected_spi_flash_device_index;
    SPI_Pad_t cs_pad_param;
    
    cs_pad_param.pin = cs_pin;
    cs_pad_param.port = cs_port;
    
    spi_init(&cs_pad_param, SPI_MODE_8BIT, SPI_ROLE_MASTER, SPI_CLK_IDLE_POL_LOW, SPI_PHA_MODE_0, SPI_MINT_DISABLE, SPI_XTAL_DIV_8);
    
    spi_flash_release_from_power_down();
    
    detected_spi_flash_device_index = spi_flash_auto_detect();
    
    return detected_spi_flash_device_index;
}
