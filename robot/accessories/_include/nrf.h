// Register definitions for radio
#ifndef NRF_H__
#define NRF_H__

/* nRF24 Instruction Definitions */
#define W_REGISTER         0x20U  /* Register write command */
#define WREG               0x20U
#define R_RX_PAYLOAD       0x61U  /* Read RX payload command */
#define W_TX_PAYLOAD       0xA0U  /* Write TX payload command */
#define FLUSH_TX           0xE1U  /* Flush TX register command */
#define FLUSH_RX           0xE2U  /* Flush RX register command */
#define REUSE_TX_PL        0xE3U  /* Reuse TX payload command */
#define ACTIVATE           0x50U  /* Activate features */
#define R_RX_PL_WID        0x60U  /* Read RX payload command */
#define W_ACK_PAYLOAD      0xA8U  /* Write ACK payload command */
#define W_TX_PAYLOAD_NOACK 0xB0U  /* Write ACK payload command */
#define NOP                0xFFU  /* No Operation command, used for reading status register */

/* Register Memory Map */
#define CONFIG        0x00U  /* nRF24 config register */
#define EN_AA         0x01U  /* nRF24 enable Auto-Acknowledge register */
#define EN_RXADDR     0x02U  /* nRF24 enable RX addresses register */
#define SETUP_AW      0x03U  /* nRF24 setup of address width register */
#define SETUP_RETR    0x04U  /* nRF24 setup of automatic retransmission register */
#define RF_CH         0x05U  /* nRF24 RF channel register */
#define RF_SETUP      0x06U  /* nRF24 RF setup register */
#define STATUS        0x07U  /* nRF24 status register */
#define OBSERVE_TX    0x08U  /* nRF24 transmit observe register */
#define CD            0x09U  /* nRF24 carrier detect register */
#define RX_ADDR_P0    0x0AU  /* nRF24 receive address data pipe0 */
#define RX_ADDR_P1    0x0BU  /* nRF24 receive address data pipe1 */
#define TX_ADDR       0x10U  /* nRF24 transmit address */
#define RX_PW_P0      0x11U  /* nRF24 \# of bytes in rx payload for pipe0 */
#define RX_PW_P1      0x12U  /* nRF24 \# of bytes in rx payload for pipe1 */
#define FIFO_STATUS   0x17U  /* nRF24 FIFO status register */
#define DYNPD         0x1CU  /* nRF24 Dynamic payload setup */
#define FEATURE       0x1DU  /* nRF24 Exclusive feature setup */

/* CONFIG register bit definitions */
#define MASK_RX_DR    (1<<6)     /* CONFIG register bit 6 */
#define MASK_TX_DS    (1<<5)     /* CONFIG register bit 5 */
#define MASK_MAX_RT   (1<<4)     /* CONFIG register bit 4 */
#define EN_CRC        (1<<3)     /* CONFIG register bit 3 */
#define CRCO          (1<<2)     /* CONFIG register bit 2 */
#define PWR_UP        (1<<1)     /* CONFIG register bit 1 */
#define PRIM_RX       (1<<0)     /* CONFIG register bit 0 */


/* RF_SETUP register bit definitions */
#define CONT_WAVE     (1<<7)     /* RF_SETUP register bit 7 */
#define PLL_LOCK      (1<<4)     /* RF_SETUP register bit 4 */
#define RF_DR         (1<<3)     /* RF_SETUP register bit 3 */
#define RF_PWR1       (1<<2)     /* RF_SETUP register bit 2 */
#define RF_PWR0       (1<<1)     /* RF_SETUP register bit 1 */
#define LNA_HCURR     (1<<0)     /* RF_SETUP register bit 0 */


/* STATUS 0x07 */
/* STATUS register bit definitions */
#define RX_DR         (1<<6)     /* STATUS register bit 6 */
#define TX_DS         (1<<5)     /* STATUS register bit 5 */
#define MAX_RT        (1<<4)     /* STATUS register bit 4 */
#define TX_FULL       (1<<0)     /* STATUS register bit 0 */


/* FIFO_STATUS 0x17 */
/* FIFO_STATUS register bit definitions */
#define TX_REUSE      (1<<6)     /* FIFO_STATUS register bit 6 */
#define TX_FIFO_FULL  (1<<5)     /* FIFO_STATUS register bit 5 */
#define TX_EMPTY      (1<<4)     /* FIFO_STATUS register bit 4 */
#define RX_FULL       (1<<1)     /* FIFO_STATUS register bit 1 */
#define RX_EMPTY      (1<<0)     /* FIFO_STATUS register bit 0 */

#endif