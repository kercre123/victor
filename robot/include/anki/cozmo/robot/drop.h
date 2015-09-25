/** I2SPI drop's transfers between the Espressif and the RTIP.
 * Each transfer is a fixed length based on the time available on the RTIP.
 * 
 * @author Daniel Casner <daniel@anki.com>
 *
 *
 */
 
#ifndef _DROP_H_
#define _DROP_H_

// ct_assert is a compile time assertion, useful for checking sizeof() and other compile time knowledge
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }
#define ASSERT_IS_POWER_OF_TWO(e) ct_assert((e & (e-1)) == 0)
#define ASSERT_IS_MULTIPLE_OF_TWO(e) ct_assert((e % 2) == 0)

/// Number of drops exchanged per second
#define DROPS_PER_SECOND (7500)

/// Number of bytes in each DMAed transaction
#define DROP_SIZE (98)
ASSERT_IS_MULTIPLE_OF_TWO(DROP_SIZE);

/// Number of samples of audio data delivered to the RTIP each drop.
#define AUDIO_SAMPLES_PER_DROP 4

/// Maximum variable payload to RTIP
#define DROP_TO_RTIP_MAX_VAR_PAYLOAD (DROP_SIZE - AUDIO_SAMPLES_PER_DROP - 5)

/// Drop structure for transfers from the WiFi to the RTIP
typedef struct
{
  uint8_t  audioData[AUDIO_SAMPLES_PER_DROP]; ///< Isochronous audio data
  uint8_t  payload[DROP_TO_RTIP_MAX_VAR_PAYLOAD]; ///< Variable format "message" data
  uint8_t  tag;     ///< ToRTIPPayloadTag
  uint8_t  droplet; ///< Droplet enum
  uint8_t  tail[3]; ///< 0xff at the end of the droplet because of running clock and stopping data
} DropToRTIP;

ct_assert(sizeof(DropToRTIP) == DROP_SIZE);

typedef enum
{
  ToRTIP_empty = 0x00, ///< No payload here
  ToRRIP_msg   = 0x01, ///< Message to main loop dispatch on RTIP
  ToRTIP_i2c   = 0x02, ///< I2C write data command
} ToRTIPPayloadTag;

/// Message receive buffer size on the RTIP
#define RTIP_MSG_BUF_SIZE (128)
/// The frequency with which we are allowed to send RTIP_MSG_BUF_SIZE bytes of data in miliseconds
#define RTIP_MSG_INTERVAL_MS (7)

#define DROP_TO_WIFI_MAX_PAYLOAD (DROP_SIZE - 5)

/// Drop structure for transfers from RTIP to WiFi
typedef struct 
{
  uint8_t payload[DROP_TO_WIFI_MAX_PAYLOAD]; ///< Variable payload for message
  uint8_t msgLen;  ///< Number of bytes of message data following JPEG data
  uint8_t droplet; ///< Droplet enum / length field
  uint8_t tail[3]; ///< Guard bytes at end of packet
} DropToWiFi;

ct_assert(sizeof(DropToWiFi) == DROP_SIZE);

/// Droplet bit masks and flags.
typedef enum
{
  jpegLenMask = ((1<<5)-1), /// Mask for JPEG length data
  jpegEOF     = 1<<5,       /// Flags this drop as containing the end of a JPEG frame
  fromWiFi    = 1<<6,       /// 1 if the message is from the WiFi or 0 if it's from the RTIP
  rtipUpdate  = 1<<7        /// Message contents are an RTIPState update to the WiFi firmware
} Droplet;

#define JPEG_LENGTH(i) ((i >> 2)&jpegLenMask)

/// RTIP to WiFi state update message
typedef struct
{
  uint32_t status;
} RTIPState;

#endif
