#ifndef _BRINGUP_SABRE_HSDIO_DEF_H_
#define _BRINGUP_SABRE_HSDIO_DEF_H_

/* ****************************************************************************************** 
                                       HSDIO  block size
******************************************************************************************  */
//bit 15
#define D_HSDIO_BLOCK_4K_SUPPORT    (0x8000)
//bit 12:14
#define D_HSDIO_BLOCK_S_BOUND_4K    (0x0000)
#define D_HSDIO_BLOCK_S_BOUND_8K    (0x1000)
#define D_HSDIO_BLOCK_S_BOUND_16K   (0x2000)
#define D_HSDIO_BLOCK_S_BOUND_32K   (0x3000)
#define D_HSDIO_BLOCK_S_BOUND_64K   (0x4000)
#define D_HSDIO_BLOCK_S_BOUND_128K  (0x5000)
#define D_HSDIO_BLOCK_S_BOUND_256K  (0x6000)
#define D_HSDIO_BLOCK_S_BOUND_512K  (0x7000)
// bit 11:0
#define D_HSDIO_BLOCK_SIZE_MASK     (0x0FFF)

/* ****************************************************************************************** 
                                       HSDIO  transfer mode reg
******************************************************************************************  */
// bit 15:9
//bit 8
#define D_HSDIO_TRMODE_BOOT_EN    (0x0100)
//bit 7
#define D_HSDIO_TRMODE_SPI_MODE   (0x0080)
// bit 6
//bit 5
#define D_HSDIO_TRMODE_MULT_BLOC  (0x0020)
// bit 4
#define D_HSDIO_TRMODE_DIR_WR  (0x0000)   // host to card
#define D_HSDIO_TRMODE_DIR_RD  (0x0010)   // card to host
// bit 3
// bit 2
#define D_HSDIO_TRMODE_CMD12_EN   (0x0004)
// bit 1
#define D_HSDIO_TRMODE_BLOCK_COUNT_EN (0x0002)
// bit 0
#define D_HSDIO_TRMODE_DMA_EN     (0x0001)

/* ****************************************************************************************** 
                                       HSDIO  command register
  the documentation defines the register as 16bit wide, but since reading/writing words 
  is the normal way of operation the values have been shifted 16 bits.
******************************************************************************************  */
// bit 15:14
//bit 13:8 
#define D_HSDIO_COMM_COMMAND_MASK (0x3F000000)
// bit 7:6
#define D_HSDIO_COMM_NORMAL  (0x00000000)
#define D_HSDIO_COMM_SUSPEND (0x00400000)
#define D_HSDIO_COMM_RESUME  (0x00800000)
#define D_HSDIO_COMM_ABORT   (0x00C00000)
// bit 5
#define D_HSDIO_COMM_DATA_PRESENT   (0x00200000)
// bit 4 
#define D_HSDIO_COMM_CMD_IDX_CHK_EN (0x00100000)
// bit 3
#define D_HSDIO_COMM_CMD_CRC_CHK_EN (0x00080000)
// bit 2
// bit 1:0
#define D_HSDIO_COMM_R_TYPE_NONE           (0x00000000)
#define D_HSDIO_COMM_R_TYPE_LEN_136        (0x00010000)
#define D_HSDIO_COMM_R_TYPE_LEN_48         (0x00020000)
#define D_HSDIO_COMM_R_TYPE_LEN_48_CHK_BSY (0x00030000)
// commands ------------------------------------------------------------------------------------
#define SD_CMD_0 (0x00000000)   // go idle , -
#define SD_CMD_1 (0x01000000)   // send op cond, MMC
#define SD_CMD_2 (0x02000000)   // all send cid, R2
#define SD_CMD_3 (0x03000000)   // rel adr, r6
#define SD_CMD_5 (0x05000000)   // r1
#define SD_CMD_7 (0x07000000)   // r1
#define SD_CMD_8 (0x08000000)   // r7
#define SD_CMD_9 (0x09000000)   // r2
#define SD_CMD_10 (0x0A000000)  //r2
#define SD_CMD_11 (0x0B000000)  // r1
#define SD_CMD_12 (0x0C000000)  // r1b
#define SD_CMD_13 (0x0D000000)  // r1
#define SD_CMD_15 (0x0F000000)  // -
// block read
#define SD_CMD_16 (0x10000000) // r1
#define SD_CMD_17 (0x11000000) // r1
#define SD_CMD_18 (0x12000000) // r1
// block write
#define SD_CMD_24 (0x18000000) // r1
#define SD_CMD_25 (0x19000000) // r1
#define SD_CMD_27 (0x1B000000) // r1
//write protection
#define SD_CMD_28 (0x1C000000) // r1b
#define SD_CMD_29 (0x1D000000) // r1b
#define SD_CMD_30 (0x1E000000) //r1
// erase 
#define SD_CMD_32 (0x20000000) // r1
#define SD_CMD_33 (0x21000000) // r1
#define SD_CMD_38 (0x26000000) // r1
// application specific 
#define SD_CMD_55 (0x37000000) // r1 
#define SD_CMD_56 (0x38000000) // r1
// app specific
#define SD_ACMD_6  (0x06000000) // r1
#define SD_ACMD_13 (0x0D000000) // r1
#define SD_ACMD_22 (0x16000000) // r1
#define SD_ACMD_23 (0x17000000) // r1
#define SD_ACMD_41 (0x29000000) // r3
#define SD_ACMD_42 (0x2A000000) // r1
#define SD_ACMD_51 (0x33000000) // r1




/* ****************************************************************************************** 
                                       HSDIO present state
******************************************************************************************  */
// bit 31:29
// bit 28:25
#define D_HSDIO_STAT_DAT7 (0x10000000)
#define D_HSDIO_STAT_DAT6 (0x08000000)
#define D_HSDIO_STAT_DAT5 (0x04000000) 
#define D_HSDIO_STAT_DAT4 (0x02000000)
// bit 24
// bit 23:20
#define D_HSDIO_STAT_DAT3 (0x00800000)
#define D_HSDIO_STAT_DAT2 (0x00400000)
#define D_HSDIO_STAT_DAT1 (0x00200000)
#define D_HSDIO_STAT_DAT0 (0x00100000)
// bit 19
#define D_HSDIO_STAT_WP                (0x00080000)
// bit 18
#define D_HSDIO_STAT_CARD_DETECT       (0x00040000)
// bit 17
#define D_HSDIO_STAT_CARD_STATE_STABLE (0x00020000)
// bit 16
#define D_HSDIO_STAT_CARD_INSERT       (0x00010000)
// bit 15:12
// bit 11
#define D_HSDIO_STAT_BUFFER_RD_EN    (0x00000800)
// bit 10
#define D_HSDIO_STAT_BUFFER_WR_EN    (0x00000400)
// bit 9
#define D_HSDIO_STAT_RD_TR_ACTIVE    (0x00000200)
// bit 8
#define D_HSDIO_STAT_WR_TR_ACTIVE    (0x00000100)
// bit 7:3
// bit 2
#define D_HSDIO_STAT_DAT_LINE_ACTIVE (0x00000004)
// bit 1
#define D_HSDIO_STAT_COMM_INHIB_DAT  (0x00000002)
// bit 0
#define D_HSDIO_STAT_COMM_INHIB_COM  (0x00000001)

/* ****************************************************************************************** 
                HSDIO control register + power control + block gap ctrl + wakeup_ctrl
******************************************************************************************  */
// bit 7
#define D_HSDIO_CTRL_CARD_DETECT_SIG      (0x0080)   //  0 - is SDCD# is selected
// bit 6
#define D_HSDIO_CTRL_CARD_DETECT_TEST_LVL (0x0040)   //  0 - no card
// bit 5
#define D_HSDIO_CTRL_SD8BIT     (0x0020)
// bit 4:3
#define D_HSDIO_CTRL_SDMA       (0x0000)
#define D_HSDIO_CTRL_32_ADMA1   (0x0008) 
#define D_HSDIO_CTRL_32_ADMA2   (0x0010)
#define D_HSDIO_CTRL_64_ADMA2   (0x0018)   
// bit 2
#define D_HSDIO_CTRL_HIGH_SPEED (0x0004)
// bit 1
#define D_HSDIO_CTRL_4BIT_MODE  (0x0002)
#define D_HSDIO_CTRL_1BIT_MODE  (0x0000)
// bit 0
#define D_HSDIO_CTRL_LED_ON     (0x0001) 
// --------------------------------------------------------------------
// bit 15:12
// bit 11:9
#define D_HSDIO_POW_BUS_VOLT_3_3   (0x0E00)
#define D_HSDIO_POW_BUS_VOLT_3_0   (0x0C00)
#define D_HSDIO_POW_BUS_VOLT_1_8   (0x0A00)
// bit 8
#define D_HSDIO_POW_BUS_PWR_ON     (0x0100)
// --------------------------------------------------------------------
// bit 23:20
// bit 19
#define D_HSDIO_GAPCTRL_INT_AT_BLOCK_GAP (0x80000)
// bit 18
#define D_HSDIO_GAPCTRL_READ_WAIT        (0x40000)
// bit 17
#define D_HSDIO_GAPCTRL_CON_REQ          (0x20000)
// bit 16
#define D_HSDIO_GAPCTRL_STOP_AT_BLK_GAP_RQ (0x10000)
// --------------------------------------------------------------------
// bit 31:27
// bit 26
#define D_HSDIO_WAKEUP_CARD_REMOVE (0x04000000)
// bit 25
#define D_HSDIO_WAKEUP_CARD_INSERT (0x02000000)
// bit 24
#define D_HSDIO_WAKEUP_CARD_INT    (0x01000000)


/* ****************************************************************************************** 
                HSDIO  clk control + timeout + reset
******************************************************************************************  */
// bit 15:8
#define D_HSDIO_CLK_PASS_TROUGH    (0x0000)
#define D_HSDIO_CLK_DIV_256        (0x8000)
#define D_HSDIO_CLK_DIV_128        (0x4000) 
#define D_HSDIO_CLK_DIV_64         (0x2000) 
#define D_HSDIO_CLK_DIV_32         (0x1000) 
#define D_HSDIO_CLK_DIV_16         (0x0800) 
#define D_HSDIO_CLK_DIV_8          (0x0400) 
#define D_HSDIO_CLK_DIV_4          (0x0200) 
#define D_HSDIO_CLK_DIV_2          (0x0100) 
// bit 7:3
// bit 2
#define D_HSDIO_CLK_EN             (0x0004)
//bit 1
#define D_HSDIO_CLK_STABLE         (0x0002)
// bit 0
#define D_HSDIO_CLK_INTERNAL_EN    (0x0001)
// ----------------------------------------------------------------------------
// bit 23:20
// bit 19:16
#define D_HSDIO_TOUT_2_27 (0xE0000)
#define D_HSDIO_TOUT_2_26 (0xD0000)
#define D_HSDIO_TOUT_2_25 (0xC0000)
#define D_HSDIO_TOUT_2_24 (0xB0000)
#define D_HSDIO_TOUT_2_23 (0xA0000)
#define D_HSDIO_TOUT_2_22 (0x90000)
#define D_HSDIO_TOUT_2_21 (0x80000)
#define D_HSDIO_TOUT_2_20 (0x70000)
#define D_HSDIO_TOUT_2_19 (0x60000)
#define D_HSDIO_TOUT_2_18 (0x50000)
#define D_HSDIO_TOUT_2_17 (0x40000)
#define D_HSDIO_TOUT_2_16 (0x30000)
#define D_HSDIO_TOUT_2_15 (0x20000)
#define D_HSDIO_TOUT_2_14 (0x10000)
#define D_HSDIO_TOUT_2_13 (0x00000)
// ----------------------------------------------------------------------------
// bit 26
#define D_HSDIO_RST_DAT (0x04000000)
// bit 25
#define D_HSDIO_RST_CMD (0x02000000)
// bit 23
#define D_HSDIO_RST_ALL (0x01000000)

/* ****************************************************************************************** 
                HSDIO   register defines
******************************************************************************************  */
#define SDIOH_SDMA                      (0x000) 
#define SDIOH_BLOCK_SIZE                (0x004) 
#define SDIOH_ARGUEMENT                 (0x008) 
#define SDIOH_TRANS_MODE                (0x00c) 
#define SDIOH_RESPONSE_REG1             (0x010) 
#define SDIOH_RESPONSE_REG2             (0x014) 
#define SDIOH_RESPONSE_REG3             (0x018) 
#define SDIOH_RESPONSE_REG4             (0x01c) 
#define SDIOH_BUF_DATA_PORT             (0x020) 
#define SDIOH_PRESENT_STATE             (0x024) 
#define SDIOH_HOST_CTRL                 (0x028) 
#define SDIOH_CLK_CTRL                  (0x02c) 
#define SDIOH_INT_STATUS                (0x030) 
#define SDIOH_INT_ENABLE                (0x034) 
#define SDIOH_INT_SIG_ENABLE            (0x038) 
#define SDIOH_AUTO_CMD                  (0x03c) 
#define SDIOH_CAPABILITIES_REG1         (0x040) 
#define SDIOH_CAPABILITIES_REG2         (0x044) 
#define SDIOH_MAX_CURRENT_CAP_REG1      (0x048) 
#define SDIOH_MAX_CURRENT_CAP_REG2      (0x04c) 
#define SDIOH_FORCE_EVENT               (0x050) 
#define SDIOH_ADMA_ERR_STAT             (0x054) 
#define SDIOH_ADMA_ADD_REG1             (0x058) 
#define SDIOH_ADMA_ADD_REG2             (0x05c) 
#define SDIOH_BOOT_DATA_CTRL            (0x060) 
#define SDIOH_SPI_INT                   (0x0f0) 
#define SDIOH_SLOT_INT_STATUS           (0x0fc) 

/* ****************************************************************************************** 
                HSDIO  defines
******************************************************************************************  */
#define DRV_HSDIO_SLOT_0 (0x0000)
#define DRV_HSDIO_SLOT_1 (0x0001)


/* ****************************************************************************************** 
                SDIO  OCR defines
******************************************************************************************  */

#define D_SDIO_VDD_16_17                 (1u << 4)
#define D_SDIO_VDD_17_18                 (1u << 5)
#define D_SDIO_VDD_18_19                 (1u << 6)
#define D_SDIO_VDD_19_20                 (1u << 7)
#define D_SDIO_VDD_20_21                 (1u << 8)
#define D_SDIO_VDD_21_22                 (1u << 9)
#define D_SDIO_VDD_22_23                 (1u << 10)
#define D_SDIO_VDD_23_24                 (1u << 11)
#define D_SDIO_VDD_24_25                 (1u << 12)
#define D_SDIO_VDD_25_26                 (1u << 13)
#define D_SDIO_VDD_26_27                 (1u << 14)
#define D_SDIO_VDD_27_28                 (1u << 15)
#define D_SDIO_VDD_28_29                 (1u << 16)
#define D_SDIO_VDD_29_30                 (1u << 17)
#define D_SDIO_VDD_30_31                 (1u << 18)
#define D_SDIO_VDD_31_32                 (1u << 19)
#define D_SDIO_VDD_32_33                 (1u << 20)
#define D_SDIO_VDD_33_34                 (1u << 21)
#define D_SDIO_VDD_34_35                 (1u << 22)
#define D_SDIO_VDD_35_36                 (1u << 23)
#define D_SDIO_CARD_BUSY                 (1u << 31)

#define D_SDIO_HOST_VOLT_RANGE    (D_SDIO_VDD_27_28  |\
                                   D_SDIO_VDD_28_29  |\
                                   D_SDIO_VDD_29_30  |\
                                   D_SDIO_VDD_30_31  |\
                                   D_SDIO_VDD_31_32  |\
                                   D_SDIO_VDD_32_33)


#endif // _BRINGUP_SABRE_HSDIO_DEF_H_
