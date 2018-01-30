#ifndef HAL_SPI_H_
#define HAL_SPI_H_

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------
//empty

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

void    hal_spi_init(void);
void    hal_spi_set_cs(void);
void    hal_spi_clr_cs(void);
void    hal_spi_write(uint8_t dat);
uint8_t hal_spi_read(void);


#endif //HAL_SPI_H_

