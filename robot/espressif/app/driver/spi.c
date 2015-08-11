#include "driver/spi.h"

#define MIN(a, b) (a < b ? a : b)

static uint8 winds[NUM_SPI_BUS]; ///< Which point in the write FIFO are we at for each bus
static uint8 rinds[NUM_SPI_BUS]; ///< Which point in the read FIFO are we at for each bus

#define W_FIFO_DEPTH 16
#define R_FIFO_DEPTH 16

uint32 ICACHE_FLASH_ATTR spi_master_init(const SPIBus spi_no, uint32 frequency)
{
	const uint32 CPU_HZ = 80000000; // Apparently this is 80MHz even if running at 160MHz ((uint32)system_get_cpu_freq()) * 1000000;
	uint32 clock_div_flag = 0;

	if(spi_no>1) return 0; //handle invalid input number

	winds[spi_no] = 0;
	rinds[spi_no] = 0;

	// Setup the SPI clock
	if (frequency > (CPU_HZ/2)) // If more than half the clock frequency, we use the system clock directly
	{
		frequency = CPU_HZ;
		clock_div_flag = 0x0001;
		WRITE_PERI_REG(SPI_CLOCK(spi_no), SPI_CLK_EQU_SYSCLK);
	}
	else // Calculate dividers
	{
		uint32 prediv = MIN((CPU_HZ/((4+1)*frequency))-1, SPI_CLKDIV_PRE); // Start with a cntdiv of 4, to make things work out
		uint32 cntdiv = MIN((CPU_HZ/((prediv + 1)*frequency))-1, SPI_CLKCNT_N);
		uint32 cntH   = cntdiv > 0 ? (cntdiv + 1)/2 - 1 : 0;
		uint32 cntL   = cntdiv;

		frequency = CPU_HZ / ((prediv + 1) * (cntdiv + 1));

		os_printf("SPI master frequency: %dHz [%04x %02x %02x %02x]\r\n", frequency, prediv, cntdiv, cntH, cntL);

		WRITE_PERI_REG(SPI_CLOCK(spi_no),
				  				 ((prediv & SPI_CLKDIV_PRE) << SPI_CLKDIV_PRE_S) |
									 ((cntdiv & SPI_CLKCNT_N)   << SPI_CLKCNT_N_S)   |
									 ((cntH   & SPI_CLKCNT_H)   << SPI_CLKCNT_H_S)   |
									 ((cntL   & SPI_CLKCNT_L)   << SPI_CLKCNT_L_S));
	}

	if (spi_no == SPI)
	{
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005|(clock_div_flag<<8)); //Set bit 8 if 80MHz sysclock required
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);
	}
	else if (spi_no == HSPI)
	{
		WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105|(clock_div_flag<<9)); // Set bit 9 if sysclock required
		//PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_HSPI); //GPIO12 is HSPI MISO pin (Master Data In), not used for SIO
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_HSPI); //GPIO13 is HSPI MOSI pin (Master Data Out)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_HSPI); //GPIO14 is HSPI CLK pin (Clock)
		PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_HSPI); //GPIO15 is HSPI CS pin (Chip Select / Slave Select)
		// TODO What is this doing?
		SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP|SPI_CS_HOLD|SPI_USR_COMMAND|SPI_USR_MISO);
		CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE);
	}

	// Big endian
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
	CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);

	return frequency;
}

void spi_mast_start_transaction(const SPIBus spi_no, const uint8 cmd_bits, const uint16 cmd_data, \
                                const uint8 addr_bits, const uint32 addr_data, \
                                const uint16 mosi_bits, const uint16 miso_bits, const uint16 dummy_bits)
{
	static uint32 userRegs[NUM_SPI_BUS] = {0};

	if (spi_no > 1) return; // Invalid bus
	while(spi_mast_busy(spi_no)); // Wait for SPI to be ready

	uint32 newUserReg = (cmd_bits   > 0 ? SPI_USR_COMMAND : 0) |
											(addr_bits  > 0 ? SPI_USR_ADDR    : 0) |
											(mosi_bits  > 0 ? SPI_USR_MOSI    : 0) |
											(miso_bits  > 0 ? SPI_USR_MISO    : 0) |
											(dummy_bits > 0 ? SPI_USR_DUMMY   : 0);

	if (newUserReg != userRegs[spi_no])
	{
		// Reset registers
		CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY);
		SET_PERI_REG_MASK(SPI_USER(spi_no), newUserReg);
		userRegs[spi_no] = newUserReg;
	}

	WRITE_PERI_REG(SPI_USER1(spi_no), ((addr_bits-1) &SPI_USR_ADDR_BITLEN)<<SPI_USR_ADDR_BITLEN_S | //Number of bits in Address
										  							((mosi_bits-1) &SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S | //Number of bits to Send
										  							((miso_bits-1) &SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S |  //Number of bits to receive
										  							((dummy_bits-1)&SPI_USR_DUMMY_CYCLELEN)<<SPI_USR_DUMMY_CYCLELEN_S); //Number of Dummy bits to insert

	if (cmd_bits)
	{
		uint16 command = cmd_data << (16-cmd_bits); //align command data to high bits
		command = ((command>>8)&0xff) | ((command<<8)&0xff00); //swap byte order
		WRITE_PERI_REG(SPI_USER2(spi_no), ((((cmd_bits-1)&SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S) | (command&SPI_USR_COMMAND_VALUE)));
	}

	if (addr_bits)
	{
		WRITE_PERI_REG(SPI_ADDR(spi_no), addr_data<<(32-addr_bits)); //align address data to high bits
	}

	// Fire off the SPI transaction
	SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);

	winds[spi_no] = 0;
	rinds[spi_no] = 0;
}

bool inline spi_mast_busy(const SPIBus spi_no)
{
	return READ_PERI_REG(SPI_CMD(spi_no)) & SPI_USR;
}

bool inline spi_mast_write(const SPIBus spi_no, const uint32 data)
{
	if (winds[spi_no] == W_FIFO_DEPTH)
	{
		return false;
	}
	else
	{
		WRITE_PERI_REG(SPI_FIFO(spi_no, winds[spi_no]), data);
		winds[spi_no]++;
		return true;
	}
}

uint32 inline spi_mast_read(const SPIBus spi_no)
{
	if (rinds[spi_no] == R_FIFO_DEPTH)
	{
		return 0;
	}
	else
	{
		return READ_PERI_REG(SPI_FIFO(spi_no, rinds[spi_no]++));
	}
}
