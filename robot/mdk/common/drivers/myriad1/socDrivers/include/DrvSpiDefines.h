// Baud Rates (clkdiv factors must be an even value between 2 and 65534)
#define DV_MIN    2
#define DV_MAX    4
#define DV_INC    2

// SPI Commands
/* 1. Common EEPROM and SRAM */
#define WRSR     0x1                    /* Write Status Register*/
#define WRITE    0x2                    /* Write */
#define READ     0x3                    /* Read */
#define RDSR     0x5                    /* Read Status Register */

/* 2. EEPROM only */
#define WRDI     0x4                    /* Reset the write enable latch (disable write operations)*/
#define WREN     0x6                    /* Set the write enable latch (enable write operations) */
#define PE      0x42                    /* Page Erase  erase one page in memory array */
#define SE      0xD8                    /* Sector Erase  erase one sector in memory array */
#define CE      0xC7                    /* Chip Erase  erase all sectors in memory array */
#define RDID    0xAB                    /* Release from Deep power-down and read electronic signature */
#define DPD     0xB9                    /* Deep Power-Down mode */

/* 2. SRAM Status register bits */
#define MBYTE     0x00                  /* Byte Mode */
#define MSEQ      0x40                  /* Sequencial mode */
#define MPAGE     0x80                  /* Page mode */
#define HOLD_EN   0x00                  /* Hold pin enable */
#define HOLD_DIS  0x01                  /* Hold pin disable */


// Event register bits
#define NF     0x0100                   /* Transmit queue is not full */
#define NE     0x0200                   /* Receive queue is not empty */
#define NME    0x0400                   /* Multiple master error */
#define UN     0x0800                   /* Underrun(underflow for the transmit queue) */
#define OV     0x1000                   /* Overrun(Overflow for the receive queue) */
#define LT     0x4000                   /* Last character transmited */

// Mask register bits
#define NFE     0x0100                  /* Transmit queue is not full enable */
#define NEE     0x0200                  /* Receive queue is not empty enable */
#define NMEE    0x0400                  /* Multiple master error enable */
#define UNE     0x0800                  /* Underrun(underflow for the transmit queue) enable */
#define OVE     0x1000                  /* Overrun(Overflow for the receive queue) enable */
#define LTE     0x4000                  /* Last character transmited enable */

// Command register
#define LST   0x400000                  /* Set this bit for the core to set the LT bit when the last character is transmitted */

// Slave select register
#define SLV_DES_ALL     0x0             /* Deselect all slaves  */
#define SLV_SEL_1       0x1             /* Select slave 1 */
#define SLV_SEL_2       0x2             /* Select slave 2 */
#define SLV_SEL_3       0x3             /* Select slave 3 */
#define SLV_SEL_4       0x4             /* Select slave 4 */
#define SLV_SEL_5       0x5             /* Select slave 5 */
#define SLV_SEL_6       0x6             /* Select slave 6 */
#define SLV_SEL_7       0x7             /* Select slave 7 */
