/** I2SPI drop's transfers between the Espressif and the RTIP.
 * Each transfer is a fixed length based on the time available on the RTIP.
 * 
 * @author Daniel Casner <daniel@anki.com>
 *
 *
 */
 
#ifndef _I2SPI_DATA_H_
#define _I2SPI_DATA_H_

// ct_assert is a compile time assertion, useful for checking sizeof() and other compile time knowledge
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#define ASSERT_IS_POWER_OF_TWO(e) ct_assert((e & (e-1)) == 0)
#define ASSERT_IS_MULTIPLE_OF_FOUR(e) ct_assert((e % 4) == 0)

/// Number of bytes in each DMAed transaction
#define I2SPI_TRANSFER_SIZE (96)
ASSERT_IS_MULTIPLE_OF_FOUR(I2SPI_TRANSFER_SIZE);

/// Maximum variable payload to RTIP
#define I2SPI_TO_RTIP_MAX_VAR_PAYLOAD (I2SPI_TRANSFER_SIZE - 6)

/// Drop structure for transfers from the WiFi to the RTIP
typedef struct
{
  uint32_t audioData; ///< Isochronous audio data
  uint8_t  payload[I2SPI_TO_RTIP_MAX_VAR_PAYLOAD]; ///< Variable format "message" data
  uint8_t  tag;     ///< ToRTIPPayloadTag
  uint8_t  droplet; ///< Droplet enum
} DropToRTIP;

ct_assert(sizeof(DropToRTIP) == I2SPI_TRANSFER_SIZE);

typedef enum
{
  ToRTIP_empty = 0x00; ///< No payload here
  ToRRIP_msg   = 0x01; ///< Message to main loop dispatch on RTIP
  ToRTIP_i2c   = 0x02; ///< I2C write data command
} ToRTIPPayloadTag;

/// Message receive buffer size on the RTIP
#define I2S_RTIP_MSG_BUF_SIZE (128)
/// The frequency with which we are allowed to send I2S_RTIP_MSG_BUF_SIZE bytes of data in miliseconds
#define I2S_RTIP_MSG_INTERVAL_MS (7)

/// Drop structure for transfers from RTIP to WiFi
typedef struct 
{
  uint8_t payload[I2SPI_TRANSFER_SIZE-2]; ///< Variable payload for message
  uint8_t msgLen;  ///< Number of bytes of message data
  uint8_t droplet; ///< Droplet enum / length field
} DropToWiFi;

assert(sizeof(DropToWiFi) == I2SPI_TRANSFER_SIZE);

/// Droplet bit masks and flags.
typedef enum
{
  jpegLenMask = ((1<<5)-1), /// Mask for JPEG length data
  jpegEOF     = 1<<5,       /// Flags this drop as containing the end of a JPEG frame
  fromWiFi    = 1<<6,       /// 1 if the message is from the WiFi or 0 if it's from the RTIP
} Droplet;

#endif
