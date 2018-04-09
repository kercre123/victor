 /*
 ****************************************************************************************
 *
 * @file DA14580_examples.c
 *
 * @brief DA14580 Engineering Examples for DA14580 SDK
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
#include <stdio.h>
#include "global_io.h"

#include "programmer.h"
#include "peripherals.h"
#include "spi.h"
#include "spi_flash.h"
#include "i2c_eeprom.h"
#include "user_periph_setup.h"
#include "uart.h"
#include "otpc.h"
#include "version.h"
   
#define GPIO_REG								0x50003000
#define MAX_PACKET_SIZE     		8192
#define ACTION_READY        		0x5A

#define NO_ACTION               0
#define ACTION_READ_VERSION     0x10

#define ACTION_UART_BAUD				0x30
#define ACTION_UART_GPIOS				0x31

#define ACTION_VPP_ENABLE				0x50

#define ACTION_READ             0x80
#define ACTION_WRITE            0x81

#define ACTION_OTP_READ         0x80
#define ACTION_OTP_WRITE        0x81

#define ACTION_CONTENTS         0x82
#define ACTION_OK               0x83
#define ACTION_ERROR            0x84
#define ACTION_DATA             0x85
#define ACTION_INVALID_COMMAND  0x86
#define ACTION_INVALID_CRC      0x87

#define ACTION_SPI_READ         0x90
#define ACTION_SPI_WRITE        0x91
#define ACTION_SPI_ERASE        0x92
#define ACTION_SPI_ID           0x93
#define ACTION_SPI_ERASE_BLOCK  0x94
#define ACTION_SPI_GPIOS				0x95

#define ACTION_EEPROM_READ     	0xA0
#define ACTION_EEPROM_WRITE    	0xA1
#define ACTION_EEPROM_ERASE    	0xA2
#define ACTION_I2C_GPIOS			 	0xA3

// OTP definitons  
#define OTP_BASE        0x40000
#define TARGET_MEMORY   (void *)0x20000000
#define BASE_MEMORY     0x80000
#define ACTION_OFFSET   0x1FE0
#define SIZE_OFFSET     0x1FE2
#define ADDRESS_OFFSET  0x1FE4
#define RESULT_OFFSET   0x1FE8

extern _uart_sel_pins uart_sel_pins;
extern _spi_sel_pins spi_sel_pins;
extern _i2c_sel_pins i2c_sel_pins;
bool vpp_en = true;
/**
 ****************************************************************************************
  @brief handler for communication errors
  
 ****************************************************************************************
 */
void communication_error(void)
{
    while(1);
}

#ifdef USE_UART

/**
 ****************************************************************************************
  @brief Put a  byte into transmittion buffer 
  
 ****************************************************************************************
 */
uint8_t* put_uint8(uint8_t value, uint8_t* buffer)
{
    *buffer++ = value;
    return buffer;
}

/**
 ****************************************************************************************
  @brief Put a  short into transmittion buffer 
  
 ****************************************************************************************
 */
uint8_t* put_uint16(uint16_t value, uint8_t* buffer)
{
    buffer = put_uint8((value >> 8) & 0xff, buffer);
    buffer = put_uint8((value >> 0) & 0xff, buffer);
    return buffer;
}

/**
 ****************************************************************************************
  @brief Put an integer into transmittion buffer 
  
 ****************************************************************************************
 */
uint8_t* put_uint32(uint32_t value, uint8_t* buffer)
{
    buffer = put_uint16((value >> 16) & 0xffff, buffer);
    buffer = put_uint16((value >>  0) & 0xffff, buffer);
    return buffer;
}

/**
 ****************************************************************************************
  @brief Transmit a byte to UART 
  
 ****************************************************************************************
 */
void send_byte(char ch)
{
    uart_putc(ch);
}

/**
 ****************************************************************************************
  @brief Transmit a short to UART 
  
 ****************************************************************************************
 */
void send_uint16(uint16_t value)
{
    send_byte((value >> 8) & 0xff);
    send_byte((value >> 0) & 0xff);
}

/**
 ****************************************************************************************
  @brief Transmit a string to UART 
  
 ****************************************************************************************
 */
void send_str(char *str)
{
    while (*str)
        send_byte(*str++);
}

/**
 ****************************************************************************************
  @brief Transmit an integer to UART 
  
 ****************************************************************************************
 */
void send_uint32(uint32_t value)
{
    send_uint16((value >> 16) & 0xffff);
    send_uint16((value >>  0) & 0xffff);
}

/**
 ****************************************************************************************
  @brief Receive a byte from UART 
  
 ****************************************************************************************
*/ 
uint8_t receive_byte()
{
    return uart_getc();
}

/**
 ****************************************************************************************
  @brief Receive a short from UART 
  
 ****************************************************************************************
*/ 
uint16_t receive_uint16()
{
    uint8_t b1 = receive_byte();
    uint8_t b0 = receive_byte();
    return (b1 << 8) | b0;
}

/**
 ****************************************************************************************
  @brief Receive an integer from UART 
  
 ****************************************************************************************
*/ 
uint32_t receive_uint32()
{
    uint16_t w1 = receive_uint16();
    uint16_t w0 = receive_uint16();
    return (w1 << 16) | w0;
}

/**
 ****************************************************************************************
  @brief Extract a short from receive buffer 
  
 ****************************************************************************************
*/ 
uint16_t get_uint16(uint8_t* buffer)
{
    uint8_t b1 = buffer[0];
    uint8_t b0 = buffer[1];
    return (b1 << 8) | b0;
}

/**
 ****************************************************************************************
  @brief Extract an integer from receive buffer 
  
 ****************************************************************************************
*/ 
uint32_t get_uint32(uint8_t* buffer)
{
    uint16_t w1 = get_uint16(buffer + 0);
    uint16_t w0 = get_uint16(buffer + 2);
    return (w1 << 16) | w0;
}

/**
 ****************************************************************************************
  @brief Extract a revert short from receive buffer 
  
 ****************************************************************************************
*/ 
uint16_t get_uint16_rev(uint8_t* buffer)
{
    uint8_t b1 = buffer[0];
    uint8_t b0 = buffer[1];
    return (b0 << 8) | b1;
}

/**
 ****************************************************************************************
  @brief Extract an revert integer from receive buffer 
  
 ****************************************************************************************
*/ 
uint32_t get_uint32_rev(uint8_t* buffer)
{
    uint16_t w1 = get_uint16_rev(buffer + 0);
    uint16_t w0 = get_uint16_rev(buffer + 2);
    return (w0 << 16) | w1;
}

/**
 ****************************************************************************************
  @brief receive buffer
  
 ****************************************************************************************
*/ 
uint16_t receive_packet(uint8_t *buffer)
{
    uint16_t length = receive_uint16();
    uint32_t crc_remote = receive_uint32();
    uint32_t crc_local;
    size_t i;
    
    for (i = 0; i < length; i++)
        buffer[i] = receive_byte();

    crc_local = crc32(0, buffer, length);
     if (crc_local != crc_remote)
        communication_error();
    return length;
}
 
/**
 ****************************************************************************************
 * @brief Send pachet to uart  
 * 
 ****************************************************************************************
 */
void send_packet(uint8_t *buffer, uint16_t length)
{
    uint32_t crc = crc32(0, buffer, length);
    send_uint16(length);
    send_uint32(crc);
    for (size_t i = 0; i < length; i++)
        send_byte(buffer[i]);
}
#endif

/****************************************************************************************
   ****************************** PROTOCOL FUNCTIONS*****************************
 ****************************************************************************************/
void response_action(uint8_t* buffer, uint32 result, uint8_t port_sel)
{
#ifdef USE_UART
    uint8_t* p = buffer;
	
    uart_pads(port_sel);
	
    if (result == ERR_OK)
        p = put_uint8(ACTION_OK, p);
    else {
        p = put_uint8(ACTION_ERROR, p);
    }
    send_packet(buffer, p - buffer);
#else
    if (result == ERR_OK) {
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_OK;
    }else {
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_ERROR;
    }
#endif	
}

void response_erase_action_result(uint8_t* buffer, uint32 result, uint8_t port_sel)
{
#ifdef USE_UART
    uint8_t* p= buffer;
    uart_pads(port_sel);
    if (result == ERR_OK)
        p = put_uint8(ACTION_OK, p);
    else {
        p = put_uint8(ACTION_ERROR, p);
        p = put_uint32(result, p);
    }
    send_packet(buffer, p - buffer);
#else
    if (result == ERR_OK) {
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_OK;
    } else {
        *(uint32_t *)(BASE_MEMORY + RESULT_OFFSET + 4) = result;
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_ERROR;
    }
#endif
}

void response_read_action_result(uint8_t* buffer, uint16_t size, uint8_t port_sel)
{
#ifdef USE_UART
    uart_pads(port_sel);
    send_packet(buffer, size);
#else            
    *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_CONTENTS;
#endif
}


void response_action_error(uint8_t* buffer,	uint32 result, uint8_t port_sel)
{
#ifdef USE_UART
    uint8_t* p= buffer;
    uart_pads(port_sel);
    if (result == ERR_OK) {
        p = put_uint8(ACTION_OK, p);
        send_packet(buffer, p - buffer);
        buffer[0]=0;
    } else {
        p = put_uint8(ACTION_ERROR, p);
        p = put_uint32(result, p);
        send_packet(buffer, p - buffer);
    }
#else

    if (result == ERR_OK) {
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_OK;
    } else {
        *(uint32_t *)(BASE_MEMORY + RESULT_OFFSET + 4) = result;
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_ERROR;
    }
#endif
}

void response_write_action_result(uint8_t* buffer,	 uint32 result, uint8_t port_sel)
{
#ifdef USE_UART
    uint8_t* p= buffer;
    uart_pads(port_sel);
    if (result == ERR_OK) {
        p = buffer;
        p = put_uint8(ACTION_OK, p);
        send_packet(buffer, p - buffer);
    } else {
        p = buffer;
        p = put_uint8(ACTION_ERROR, p);
        p = put_uint32(result, p);
        send_packet(buffer, p - buffer);
    }
#else
    if (result == ERR_OK) {
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_OK;
    } else {
        *(uint32_t *)(BASE_MEMORY + RESULT_OFFSET + 4) = result;
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_ERROR;
    }
#endif
}

void response_id_action_result(uint8_t* buffer, int result , uint8_t port_sel)
{
#ifdef USE_UART
    uint8_t* p= buffer;
    uart_pads(port_sel);
    p = buffer; 
    if (result <0)
        p = put_uint8(ACTION_ERROR, p);
    else 
        p = put_uint8(ACTION_CONTENTS, p);
            
    p = put_uint32((uint32)result, p);
    send_packet(buffer, p-buffer);
#else
    if (result <0) {
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_ERROR;
        *(uint32_t *)(BASE_MEMORY + RESULT_OFFSET + 4) = (uint32)result;
    } else {
        *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_CONTENTS;
        *(uint32_t *)(BASE_MEMORY + RESULT_OFFSET + 4) = (uint32)result;
    }
#endif
}

uint8_t* get_write_position(uint8_t* buffer)
{
#ifdef USE_UART
    return &buffer[7];      // set pointer to SYSRAM
#else
    return TARGET_MEMORY;
#endif
}

uint8_t* get_read_position(uint8_t* buffer)
{
    uint8_t* p;
#ifdef USE_UART
    p = buffer;
    p = put_uint8(ACTION_CONTENTS, p); 
#else
    p = TARGET_MEMORY;
#endif
    return p;
}

/****************************************************************************************
  **************************************************************************************** 
  ****************************** FLASH MANUPULATION FUNCTIONS*****************************
  **************************************************************************************** 
  ****************************************************************************************/

#define SPI_SECTOR_SIZE 4096
SPI_Pad_t spi_FLASH_CS_Pad;
/**
 ****************************************************************************************
 * @brief SPI and SPI flash Initialization fucntion
 * 
 ****************************************************************************************
 */
static void spi_flash_peripheral_init()
{
    int8_t auto_detect_result;
	
    spi_FLASH_CS_Pad.pin = (GPIO_PIN) spi_sel_pins.spi_cs_pin;
    spi_FLASH_CS_Pad.port = (GPIO_PORT) spi_sel_pins.spi_cs_port;
    // Enable SPI & SPI FLASH
    spi_init(&spi_FLASH_CS_Pad, SPI_MODE_8BIT,	 SPI_ROLE_MASTER,  SPI_CLK_IDLE_POL_LOW,SPI_PHA_MODE_0,SPI_MINT_DISABLE,SPI_XTAL_DIV_8);

    spi_flash_release_from_power_down();
    
    // automatically detect flash parameters
    auto_detect_result =  spi_flash_auto_detect();
    
    // if automatic detection failed then use default flash parameters
    if (auto_detect_result == SPI_FLASH_AUTO_DETECT_NOT_DETECTED)
    {
        spi_flash_init(SPI_FLASH_DEFAULT_SIZE, SPI_FLASH_DEFAULT_PAGE);
    }
}

 /**
 ****************************************************************************************
 * @brief Erase a sector
  * 
 ****************************************************************************************
 */
static int8_t erase_flash_sectors(uint32_t starting_address, uint32_t size)
{
    int i;
    uint32_t starting_sector;
    uint32_t sector;
    int8_t ret=-1;

    // starting_sector = the starting offset of the sector that contains starting_address
    starting_sector = (starting_address/SPI_SECTOR_SIZE) * SPI_SECTOR_SIZE;

    sector=(size/SPI_SECTOR_SIZE);
    if (size%SPI_SECTOR_SIZE) 
        sector++;
    if (!sector) sector++;

    for (i=0;i<sector;i++)
    {
        ret = spi_flash_block_erase(starting_sector, SECTOR_ERASE);
        starting_sector+=SPI_SECTOR_SIZE;
        if (ret != ERR_OK) break;
    }
    return ret;
}

int16_t read_data(uint8* buffer)
{
    #ifdef USE_UART
        return receive_packet(buffer);
    #else 
        return 0;
    #endif
}

uint8_t get_action(uint8* buffer)
{
    #ifdef USE_UART
            return buffer[0];
    #else
            return *(uint8_t *)(BASE_MEMORY + ACTION_OFFSET);
    #endif
}

uint32_t get_address(uint8* buffer)
{
    #ifdef USE_UART
            return (uint32_t) get_uint32(buffer + 1);
    #else
            return *(uint32_t *)(BASE_MEMORY + ADDRESS_OFFSET);
    #endif
}

uint16_t get_size(uint8* buffer)
{
    #ifdef USE_UART
            return (uint16_t) get_uint16(buffer + 5);
    #else
            return   *(uint16_t *)(BASE_MEMORY + SIZE_OFFSET);
    #endif
}

unsigned char  Headerbuffer[8]={0x70,0x50,0x00,0x00,0x00,0x00,0x00,0x02};
unsigned char  Headerbuffer32[32]={0x80,0x50,0x00,0x00,0x00,0x00,0x00,0x01};
#ifdef USE_UART
#define ALLOWED_DATA 0x8000
uint8 buffer[ALLOWED_DATA]  __attribute__((at(0x20002200)));
#else
uint8 *buffer=NULL;
#define ALLOWED_DATA 0xA800
#endif

#ifdef USE_UART
    #pragma arm section zidata = "non_init"
    uint8_t gpio_port;// __attribute__ ((section ("non_init")));
    uint8_t gpio_pin;// __attribute__ ((section ("non_init")));
    uint8_t port_sel;// __attribute__ ((section ("non_init")));
    #pragma arm section zidata
#else
    uint8_t gpio_port __attribute__((at(0x81fd0)));
    uint8_t gpio_pin 	__attribute__((at(0x81fd1)));
    uint8_t port_sel 	__attribute__((at(0x81fd2)));
#endif

int main(void)
{
    uint8_t *p;
    int headersize =0;
    uint32_t starting_address;
    int total;
    int current;
    char crc_code;
    char *code;
    int result=-1, i;
    uint8_t action;
    uint32_t address;
    uint16_t size;
    uint32_t bytes_written = 0;
    uint32_t bytes_read;

#ifndef USE_UART
    *(uint8_t *)(BASE_MEMORY + ACTION_OFFSET) = 0;
#endif

		periph_init();

#ifdef USE_UART
    SetWord16(CLK_PER_REG, UART1_ENABLE);           // enable  clock for UART1
    #ifdef DEVELOPMENT_MODE // To use the debugger, define it and set the UART_PIN_PAIR according to board configuration (periph_setup.h) 
			port_sel = UART_PIN_PAIR;  
    #endif

    switch (port_sel)
    {
				case 0:
						uart_init(port_sel, UART_BAUD_57600);
						break;
        case 2:
						uart_init(port_sel, UART_BAUD_115200);
						break;
				case 4:
						uart_init(port_sel, UART_BAUD_57600);
						break;
        case 6:
						uart_init(port_sel, UART_BAUD_9600);
            break;
        default:
						uart_init(port_sel, UART_BAUD_57600);
            break;
    }		 
		
    uart_print("Hello");
    memset(buffer, 0, 32192);
#else    
    memset(TARGET_MEMORY, 0, ALLOWED_DATA);
    *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_READY;
#endif

    while(1) {

#ifdef USE_UART
        while(read_data(buffer)<=0) {
            response_action_error(buffer, ACTION_INVALID_CRC, port_sel);
        }
#endif
        
        action = get_action(buffer);
        address = get_address(buffer);
        size = get_size(buffer);        
        starting_address = (uint32_t)address;
        
#ifndef USE_UART        
        if(action)
            *(uint8_t *)(BASE_MEMORY + ACTION_OFFSET) = 0;
#endif        
        switch(action) 
        {
            case NO_ACTION:
                break;
             
            case ACTION_READ_VERSION:
                p = get_read_position(buffer);
                memcpy(p,	FLASH_PROGRAMMER_SW_VERSION, sizeof(DA14580_SW_VERSION));
                response_read_action_result(buffer, sizeof(FLASH_PROGRAMMER_SW_VERSION) + 1, port_sel);
                break;
						
						case ACTION_UART_BAUD:
								result = ERR_OK;
								if ((buffer[1] != UART_BAUD_9600) &&
										(buffer[1] != UART_BAUD_19200) &&
										(buffer[1] != UART_BAUD_57600) &&
										(buffer[1] != UART_BAUD_115200) &&
                                        (buffer[1] != UART_BAUD_1M))
									result = -1;
								
								response_action(buffer, result, port_sel);
								for (volatile int i=0; i<200; i++){}	
								if (result == ERR_OK)
									uart_init(port_sel, buffer[1]);							
								break;
						
						case ACTION_VPP_ENABLE:
								result = ERR_OK;
								if (buffer[1] > true)									
									result = -1;
																	
								if (result == ERR_OK)
									vpp_en = buffer[1];
								
								response_action(buffer, result, port_sel);
								break;
								
						case ACTION_UART_GPIOS:
								result = ERR_OK;
								
								if ((buffer[1] == GPIO_PORT_0) || (buffer[1] == GPIO_PORT_3)) {
									if (buffer[2] > GPIO_PIN_7) 
                                        result = -1;
								}							
								if (buffer[1] == GPIO_PORT_1) {
									if (buffer[2] > GPIO_PIN_5)
                                        result = -1;
								}
								if (buffer[1] == GPIO_PORT_2) {
									if (buffer[2] > GPIO_PIN_9)
                                        result = -1;
								}

                                if ((buffer[3] == GPIO_PORT_0) || (buffer[3] == GPIO_PORT_3)) {
									if (buffer[4] > GPIO_PIN_7) 
                                        result = -1;
								}							
								if (buffer[3] == GPIO_PORT_1) {
									if (buffer[4] > GPIO_PIN_5)
                                        result = -1;
								}
								if (buffer[3] == GPIO_PORT_2) {
									if (buffer[4] > GPIO_PIN_9)
                                        result = -1;
								}
								
								response_action(buffer, result, port_sel);
								for (volatile int i=0; i<50; i++){}	
								if (result == ERR_OK) {
									port_sel = 10; // something different than 0,2,4,6 the default uart pin pairs.
									GPIO_ConfigurePin( (GPIO_PORT) uart_sel_pins.uart_port_tx, (GPIO_PIN) uart_sel_pins.uart_tx_pin, INPUT, PID_GPIO, false);
									GPIO_ConfigurePin( (GPIO_PORT) uart_sel_pins.uart_port_rx, (GPIO_PIN) uart_sel_pins.uart_rx_pin, INPUT, PID_GPIO, false);
									uart_sel_pins.uart_port_tx = buffer[1];
									uart_sel_pins.uart_tx_pin = buffer[2];
                                    uart_sel_pins.uart_port_rx = buffer[3];
									uart_sel_pins.uart_rx_pin = buffer[4];
									set_pad_uart();
								}
								break;						
								
            case ACTION_SPI_GPIOS:
            {
                const uint8_t MAX_GPIO_PIN[] = {GPIO_PIN_7, GPIO_PIN_5, GPIO_PIN_9, GPIO_PIN_7};
                #ifdef USE_UART
                // uart
                uint8_t * pin_buffer = &buffer[1];
                #else
                // jtag
                uint8_t * pin_buffer = TARGET_MEMORY;
                #endif

                result = ERR_OK;

                // check that all provided GPIOS are valid
                // note: the action buffer contains 4 port-pin pairs
                for(int idx = 0; idx < 8; idx += 2)
                {
                    uint8_t port = pin_buffer[idx];
                    uint8_t pin = pin_buffer[idx + 1];
                    
                    if(port > GPIO_PORT_3)
                    {
                        result = -1;
                        break;
                    }
                    else
                    {
                        if(pin > MAX_GPIO_PIN[port])
                        {
                            result = -1;
                            break;
                        }
                    }
                }

                if (result == ERR_OK) {
                    spi_sel_pins.spi_cs_port  = pin_buffer[0];
                    spi_sel_pins.spi_cs_pin   = pin_buffer[1];
                    spi_sel_pins.spi_clk_port = pin_buffer[2];
                    spi_sel_pins.spi_clk_pin  = pin_buffer[3];
                    spi_sel_pins.spi_do_port  = pin_buffer[4];
                    spi_sel_pins.spi_do_pin   = pin_buffer[5];
                    spi_sel_pins.spi_di_port  = pin_buffer[6];
                    spi_sel_pins.spi_di_pin   = pin_buffer[7];
                }

                response_action(buffer, result, port_sel);
            }
            break;

            case ACTION_I2C_GPIOS:
            {
                const uint8_t MAX_GPIO_PIN[] = {GPIO_PIN_7, GPIO_PIN_5, GPIO_PIN_9, GPIO_PIN_7};
                #ifdef USE_UART
                // uart
                uint8_t * pin_buffer = &buffer[1];
                #else
                // jtag
                uint8_t * pin_buffer = TARGET_MEMORY;
                #endif

                result = ERR_OK;

                // check that all provided GPIOS are valid
                // note: the action buffer contains 2 port-pin pairs
                for(int idx = 0; idx < 4; idx += 2)
                {
                    uint8_t port = pin_buffer[idx];
                    uint8_t pin = pin_buffer[idx + 1];
                    
                    if(port > GPIO_PORT_3)
                    {
                        result = -1;
                        break;
                    }
                    else
                    {
                        if(pin > MAX_GPIO_PIN[port])
                        {
                            result = -1;
                            break;
                        }
                    }
                }

                if (result == ERR_OK) {
                    i2c_sel_pins.i2c_scl_port = pin_buffer[0];
                    i2c_sel_pins.i2c_scl_pin  = pin_buffer[1];
                    i2c_sel_pins.i2c_sda_port = pin_buffer[2];
                    i2c_sel_pins.i2c_sda_pin  = pin_buffer[3];
                }

                response_action(buffer, result, port_sel);
            }
            break;

            case ACTION_SPI_ERASE_BLOCK:
                set_pad_spi();
                spi_flash_peripheral_init();
                result = (uint32) erase_flash_sectors(address, size*SPI_SECTOR_SIZE);
                response_erase_action_result(buffer, result, port_sel);
                break;
             
            case ACTION_SPI_ERASE:
                set_pad_spi();
                spi_flash_peripheral_init();
                result = (uint32) spi_flash_chip_erase();
                response_erase_action_result(buffer, result, port_sel);
                break;
             
            case ACTION_SPI_READ:
                set_pad_spi();
                spi_flash_peripheral_init();
                starting_address = (uint32_t)address;
                p=get_read_position(buffer);
                result = spi_flash_read_data(p, (uint32_t)starting_address, (uint32_t)size);
                // if (result<0)
                //     response_action_error(buffer, (uint32)result,port_sel);
                // else
                response_read_action_result(buffer, size+1,port_sel);
                break;

            case ACTION_SPI_WRITE:  
                set_pad_spi();
                spi_flash_peripheral_init();
                p = get_write_position(buffer);
                result = 0;
                headersize = size+8;
                if (starting_address==8)
                {
                    Headerbuffer[6]= (unsigned char) (0xfF&(headersize>>8));
                    Headerbuffer[7]= (unsigned char)(0xfF&headersize);
                    spi_flash_write_data(Headerbuffer, 0, 8);
                    spi_flash_write_data(p,starting_address, size);
                }
                else
                {
                    spi_flash_write_data(p,(uint32_t)starting_address, size);
                }
                response_write_action_result(buffer, (uint32)result,port_sel); 
                break;

            case ACTION_SPI_ID:
                set_pad_spi();
                spi_flash_peripheral_init();
                result = spi_read_flash_jedec_id();
                response_id_action_result(buffer, result,port_sel);
                break;
            
            case ACTION_EEPROM_READ:
                set_pad_eeprom();
                i2c_eeprom_init(I2C_SLAVE_ADDRESS, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_2BYTES_ADDR);
                starting_address = (uint32_t)address;
                p=get_read_position(buffer);
                i2c_eeprom_read_data(p, (uint32_t)starting_address, (uint32_t)size, &bytes_read);
                response_read_action_result(buffer, size+1,port_sel);
                break;
            
            case ACTION_EEPROM_WRITE: 
                set_pad_eeprom();
                i2c_eeprom_init(I2C_SLAVE_ADDRESS, I2C_SPEED_MODE, I2C_ADDRESS_MODE, I2C_2BYTES_ADDR);
                starting_address = (uint32_t)address;	
                if (starting_address==32)
                {
                    //starting_address+=32;
                    result = 0;
                    headersize = size;//+12;
                    total =((headersize)%32);
                    if (total)
                        total = headersize+(32-total);
                    else
                        total = headersize;
                    code = (char*)get_write_position(buffer);
                    i2c_eeprom_write_data((uint8_t *)code, starting_address, total, &bytes_written);// headersize-3+64);
                    crc_code=0;
                    current=0;

                    do{
                        current+=32;                        // set to next page (32bytes per page)
                        for(i=0;i<32;i++)
                            crc_code^=code[i];              // update CRC
                        code+=32;
                    }while(current<total);                  // check if read all code
                    // EEPROM header (AN-B-001) 
                     
                    Headerbuffer[2]= 0xfF&(headersize>>8);
                    Headerbuffer[3]= 0xfF&headersize;
                    Headerbuffer[4]= crc_code;
                    i2c_eeprom_write_data(Headerbuffer, 0 , 5, &bytes_written);
                    response_write_action_result(buffer, 0,port_sel);
                } else {
                    code =  (char*)get_write_position(buffer);
                    i2c_eeprom_write_data((uint8_t *)code, starting_address, size, &bytes_written); 
                    response_write_action_result(buffer, 0,port_sel);
                }
                break;
                
            case ACTION_OTP_READ:
                otpc_clock_enable(); // Enable OTP clock
                // Before every read action, put the OTP back in MREAD mode.
                // In this way, we can handle the OTP as a normal
                // memory mapped block.
                SetBits32 (OTPC_MODE_REG, OTPC_MODE_MODE, OTPC_MODE_MREAD);
#ifdef USE_UART
                p = buffer;
                p = put_uint8(ACTION_CONTENTS, p);
                for (i = 0; i < size; i++)
                    p = put_uint8(((uint8_t*)address)[i], p);
                send_packet(buffer, p - buffer);
#else            
                memcpy((void *)TARGET_MEMORY, (void *)(OTP_BASE_ADDRESS+address), size);
                *(uint8_t *)(BASE_MEMORY + RESULT_OFFSET) = ACTION_CONTENTS;
#endif            
                otpc_clock_disable();
                break;
            
            case ACTION_OTP_WRITE:
            {
                uint32_t word2write;
                unsigned int target_address = (unsigned int) address;
                size_t i;
                result = 0;
                if (vpp_en == true) {
                    SetWord16(GPIO_REG+0x20*gpio_port+0x6+0x2*gpio_pin,0x300);
                    SetWord16(GPIO_REG+0x20*gpio_port+0x2, 1<<gpio_pin);
                }
                for (i = 0; i < size; i += 4) {
#ifdef USE_UART
                    word2write = get_uint32_rev(buffer + 7 + i);
#else                    
                    word2write = *((uint32_t *)((uint8_t *)TARGET_MEMORY + i));
#endif
                    /*
                     * OTP cell must be powered on, so we turn on the related clock.
                     * However, the OTP LDO will not be able to sustain an infinite 
                     * amount of writes. So we keep our writes limited. Turning off the
                     * LDO at the end of the operation ensures that we restart the 
                     * operation appropriately for the next write.
                     */
                    otpc_clock_enable();

                    /* From the manual: The destination address is a word aligned address, that represents the OTP memory
                    address. It is not an AHB bus address. The lower allowed value is 0x0000 and the maximum allowed
                    value is 0x1FFF (8K words space). */
                    result = otpc_write_fifo((unsigned int) (&word2write), target_address - OTP_BASE_ADDRESS, 1);
                    otpc_clock_disable();
                    target_address += 4;
                    if (result != 0)
                        break;
                }
                if (vpp_en == true) {
									SetWord16(GPIO_REG+0x20*gpio_port+0x4, 1<<gpio_pin);
								}
                response_write_action_result(buffer, result,port_sel);
                }
                break;

            default:
                response_action_error(buffer, ACTION_INVALID_COMMAND, port_sel);
        }
    }
}
