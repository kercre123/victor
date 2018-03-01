#ifndef DA14580_OTP_HEADER_H
#define DA14580_OTP_HEADER_H

#include <stdint.h>

//Dialog DA14580 OTP Memory Map:
#define OTP_ADDR_BASE                     0x040000 /*base address for OTP region*/
#define OTP_ADDR_HEADER                   0x047F00 /*address where the header lives*/
#define OTP_ADDR_END                      0x047FFF /*last address in the OTP region*/
#define OTP_SIZE                          0x8000   /*32kB*/
#define OTP_HEADER_SIZE                   0x0100   /*256B*/

//Valid application magic numbers
#define OTP_HEADER_APP_FLAG_EMPTY         0x00000000
#define OTP_HEADER_APP_FLAG_1_VALID       0x1234A5A5
#define OTP_HEADER_APP_FLAG_2_VALID       0xA5A51234
#define OTP_HEADER_APP_FLAGS_SIZE         8

//package type specifiers
#define OTP_HEADER_PACKAGE_WLCSP34        0x00000000  //WLCSP34
#define OTP_HEADER_PACKAGE_QFN40          0x000000AA  //QFN40
#define OTP_HEADER_PACKAGE_QFN48          0x00000055  //QFN48
#define OTP_HEADER_PACKAGE_KGD            0x00000099  //KGD
#define OTP_HEADER_PACKAGE_QFN40_0P5MM    0x00000011  //QFN40 0.5mm pitch
#define OTP_HEADER_PACKAGE_QFN40_FLASH    0x00000022  //QFN40 + flash (DA14583-00)
#define OTP_HEADER_PACKAGE_QFN56_SC14439  0x00000033  //QFN56 + SC14439 (DA14582)

//32k clock selection
#define OTP_HEADER_32K_SRC_XTAL32KHZ      0x00000000  //external 32kHz xtal
#define OTP_HEADER_32K_SRC_RC32KHZ        0x000000AA  //internal 32kHz rc osc

//calibration flags (bitfield)
#define OTP_HEADER_CAL_FLAG_PREFIX        0xA5A50000  //Bit[31:16]: 0xA5A5
#define OTP_HEADER_CAL_FLAG_TRIM_VCO      0x00000020  //Bit5: Trim_VCO_Cal
#define OTP_HEADER_CAL_FLAG_TRIM_XTAL16   0x00000010  //Bit4: Trim_XTAL16_Cal
#define OTP_HEADER_CAL_FLAG_TRIM_RC16     0x00000008  //Bit3: Trim_RC16_Cal
#define OTP_HEADER_CAL_FLAG_TRIM_BANDGAP  0x00000004  //Bit2: Trim_Bandgap_Cal
#define OTP_HEADER_CAL_FLAG_TRIM_RFIO     0x00000002  //Bit1: Trim_RFIO_Cal
#define OTP_HEADER_CAL_FLAG_TRIM_LNA      0x00000001  //Bit0: Trim_LNA_Cal

//customer code signature algorithm
#define OTP_HEADER_SIG_ALGORITHM_NONE     0x00000000
#define OTP_HEADER_SIG_ALGORITHM_MD5      0x000000AA
#define OTP_HEADER_SIG_ALGORITHM_SHA1     0x00000055
#define OTP_HEADER_SIG_ALGORITHM_CRC32    0x000000FF

//remapping flag
#define OTP_HEADER_REMAP_ADDR_0_SRAM      0x00000000  //SRAM remapped to 0
#define OTP_HEADER_REMAP_ADDR_0_OTP       0x000000AA  //OTP remapped to 0

//JTAG enable flag
#define OTP_HEADER_JTAG_ENABLED           0x00000000
#define OTP_HEADER_JTAG_DISABLED          0x000000AA

//BD Address (Dialog's BLE Stack reads from here)
#define OTP_HEADER_BDADDR_OFFSET          0xD4        /*aka dev_unique_id field*/
#define OTP_HEADER_BDADDR_LEN             6

//OTP Header format
//Note: fields beginning with "FAC_" are protected; burned by manufacturer. "Invalid value may destroy the chip"
typedef struct {
  uint32_t app_flag_1;        //ofs 0x00      0: Empty OTP, 0x1234A5A5: Application is in OTP
  uint32_t app_flag_2;        //ofs 0x04      0: Empty OTP, 0xA5A51234: Application is in OTP
  uint32_t FAC_rf_trim;       //ofs 0x08      Bits[31:16]=RF_TRIM1 ,Bits[15:0]=RF_TRIM2
  uint32_t FAC_crc;           //ofs 0x0C      CRC16 of the calibration data (fields: 0x08,78,7C,84,88,90) -> {FAC_rf_trim, cal_flags, FAC_trim_LNA, FAC_trim_bandgap, FAC_trim_RC16, FAC_trim_VCO}
  uint32_t FAC_reserved[13];  //ofs 0x10-0x40 Reserved 0
  uint32_t custom[8];         //ofs 0x44-0x60 Custom Fields
  uint32_t FAC_timestamp;     //ofs 0x64      "Byte3, Byte2, Byte1, Byte0"
  uint32_t FAC_tester;        //ofs 0x68      Bits[7:2]=Tester_ID, Bits[1:0]=Tester_Site
  uint32_t position;          //ofs 0x6C      Byte3: LOT, Byte2: Wafer, Byte1: Y coord, Byte0: X coord
  uint32_t package;           //ofs 0x70      0x00=WLCSP34, 0xAA=QFN40, 0x55=QFN48, 0x99=KGD, 0x11=QFN40 0.5mm pitch, 0x22=QFN40 + flash (DA14583-00), 0x33=QFN56 + SC14439 (DA14582)
  uint32_t src_32KHz;         //ofs 0x74      32kHz source selection - 0x00: XTAL32KHz, 0xAA: RC32KHz
  uint32_t cal_flags;         //ofs 0x78      Bit[31:16]: 0xA5A5, Bit5: Trim_VCO_Cal, Bit4: Trim_XTAL16_Cal, Bit3: Trim_RC16_Cal, Bit2: Trim_Bandgap_Cal, Bit1: Trim_RFIO_Cal, Bit0: Trim_LNA_Cal
  uint32_t FAC_trim_LNA;      //ofs 0x7C      LNA Trim Values
  uint32_t trim_RFIO;         //ofs 0x80      RFIO capacitance Trim Values
  uint32_t FAC_trim_bandgap;  //ofs 0x84      BandGap Trim Values
  uint32_t FAC_trim_RC16;     //ofs 0x88      RC16MHz Trim Values
  uint32_t trim_XTAL16;       //ofs 0x8C      XTAL16MHz Trim Values
  uint32_t FAC_trim_VCO;      //ofs 0x90      VCO Trim Values
  uint32_t code_signature[15];//ofs 0x94-0xCC Customer Code Signature - Hash Signature of Code
  uint32_t sig_algorithm;     //ofs 0xD0      Signature Algorithm - 0x00: None, 0xAA: MD5, 0x55: SHA-1, 0xFF: CRC32
  uint32_t dev_unique_id[2];  //ofs 0xD4-0xD8 Device unique ID - Device number (written as a string of bytes, i.e. left-most byte will be burned at 0x7FD4)
  uint32_t custom_field[6];   //ofs 0xDC-0xF0 Custom Fields
  uint32_t remap_flag;        //ofs 0xF4      Remapping Flag - 0x00: SRAM remapped to 0, 0xAA: OTP remapped to 0
  uint32_t dma_len;           //ofs 0xF8      DMA Length - Number of 32-bit words. < 0x2000 words. (>= APPLICATION LEN FOR OTP MIRRORING)
  uint32_t jtag_enable;       //ofs 0xFC      JTAG enable flag - 0x0: Enable, 0xAA: Disable
} da14580_otp_header_t;


#endif //DA14580_OTP_HEADER_H

