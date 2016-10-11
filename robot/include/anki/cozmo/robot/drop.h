/** I2SPI drop's transfers between the Espressif and the RTIP.
 * Each transfer is a fixed length based on the time available on the RTIP.
 * 
 * @author Daniel Casner <daniel@anki.com>
 *
 *
 */
 
#ifndef _DROP_H_
#define _DROP_H_

//#include <stdint.h>

#include "anki/cozmo/robot/ctassert.h"

/// Data date for exchanges
#define I2SPI_FREQUENCY (10000000)
/// Number of raw (not usable) bytes on the I2SPI bus
#define I2SPI_RAW_BYTES_PER_SECOND (I2SPI_FREQUENCY/8)
/// The frequency which which drops are exchanged
#define DROPS_PER_SECOND (7440)
/// The estimated spacing between drop starts in bytes. We start looking at this location
#define DROP_BYTE_SPACING (I2SPI_RAW_BYTES_PER_SECOND / DROPS_PER_SECOND)


/** Number of bytes on the I2SPI bus between drops.
 * This is a quasi-magic number derrived from Nathans JPEG encoding logic.
 */
#define DROP_SPACING (168)
ASSERT_IS_MULTIPLE_OF_TWO(DROP_SPACING);
/// Number of usable bytes on the I2SPI bus for drops from RTIP to WiFi
#define DROP_TO_WIFI_SIZE (100)
/// Number of usable bytes on the I2SPI bus for drops from WiFi to the RTIP
#define DROP_TO_RTIP_SIZE (DROP_PREAMBLE_SIZE + MAX_AUDIO_BYTES_PER_DROP + 1 + MAX_SCREEN_BYTES_PER_DROP + DROP_TO_RTIP_MAX_VAR_PAYLOAD + 1)
/// Number of bytes of drop prefix
#define DROP_PREAMBLE_SIZE sizeof(preambleType)
/// Preamble for drops from WiFi to RTIP
/// Preamble for drops from RTIP to WiFi

/// Number of samples of audio data delivered to the RTIP each drop.
#define MAX_AUDIO_BYTES_PER_DROP 3
/// Number of bytes of SCREEN data to the RTIP each drop
#define MAX_SCREEN_BYTES_PER_DROP 4

/// What fraction of the time to send screen data
#define MAX_TX_CHAIN_COUNT 2

/// Maximum variable payload to RTIP
// This is as much as we have time for without interfering with I2C timing on the RTIP
#define DROP_TO_RTIP_MAX_VAR_PAYLOAD (53)

enum DROP_PREAMBLE {
  TO_RTIP_PREAMBLE = 0x5452,
  TO_WIFI_PREAMBLE = 0x6957,
};

typedef uint16_t preambleType;

/// Drop structure for transfers from the WiFi to the RTIP
typedef struct
{
  preambleType preamble; ///< Synchronization Preamble indicating drop destination
  uint8_t  audioData[MAX_AUDIO_BYTES_PER_DROP];   ///< Isochronous audio data
  uint8_t  screenData[MAX_SCREEN_BYTES_PER_DROP]; ///< Isochronous SCREEN write data
  uint8_t  payloadLen;                            ///< Number of data bytes in payload
  uint8_t  payload[DROP_TO_RTIP_MAX_VAR_PAYLOAD]; ///< Variable format "message" data
  uint8_t  droplet; ///< Drop flags
} DropToRTIP;

ct_assert(sizeof(DropToRTIP) == DROP_TO_RTIP_SIZE);

ASSERT_IS_MULTIPLE_OF_TWO(DROP_TO_RTIP_SIZE);

/// Message receive buffer size on the RTIP
#define RTIP_MSG_BUF_SIZE (128)
/// The frequency with which we are allowed to send RTIP_MSG_BUF_SIZE bytes of data in miliseconds
#define RTIP_MSG_INTERVAL_MS (7)

#define DROP_TO_WIFI_MAX_PAYLOAD (DROP_TO_WIFI_SIZE - DROP_PREAMBLE_SIZE - 2)

/// Drop structure for transfers from RTIP to WiFi
typedef struct
{
  preambleType preamble;
  uint8_t payloadLen;  ///< Number of bytes of message data following JPEG data
  uint8_t droplet; ///< Drop flags and bit fields
  uint8_t payload[DROP_TO_WIFI_MAX_PAYLOAD]; ///< Variable payload for message
} DropToWiFi;

ct_assert(sizeof(DropToWiFi) == DROP_TO_WIFI_SIZE);

ASSERT_IS_MULTIPLE_OF_TWO(DROP_TO_WIFI_SIZE);

/// Droplet bit masks and flags.
typedef enum
{
// To WiFi drop fields
  jpegLenMask       = ((1<<5)-1), ///< Mask for JPEG length data, legth is in 4 byte words
  jpegEOF           = 1<<5,       ///< Flags this drop as containing the end of a JPEG frame
  oledWatermark     = 1<<6,       ///< RTIP internal OLED buffer has reached 50%
// To RTIP drop fields
  audioDataValid    = 1<<0,    ///< Bytes in the iscochronous audio field are valid
  screenDataValid   = 1<<1,    ///< Bytes in the iscochronous screen field are valid
  screenRectData    = 1<<2,    ///< Bytes in the iscochronous screen field are bounding data
} Droplet;

#define JPEG_LENGTH(i) (((i+3) >> 2)&jpegLenMask)
#define GET_JPEG_LENGTH(i) ((i&jpegLenMask) << 2)

#define RTIP_MAX_CLAD_MSG_SIZE (253)
#define RTIP_RX_MAX_BUFFER (192)
#define RTIP_RX_FLUSH_PER_DROP (3)

/// RTIP to WiFi state update message
typedef struct
{
  uint32_t status;
} RTIPState;

#endif
