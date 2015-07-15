#ifndef _I2SPI_DATA_H_
#define _I2SPI_DATA_H_

#include "anki/common/robot/utilities_c.h"

/// Number of bytes in each DMAed transaction
#define I2SPI_TRANSFER_SIZE (512)
/// Maximum transfer payload length
#define I2SPI_MAX_PAYLOAD (I2SPI_TRANSFER_SIZE-4)

/// Tags to identify different transaction payload types
typdef enum
{
  TagImgData,   ///< Payload contains images data from the RTIP to the WIFI chip
  TagAudioData, ///< Payload contains Audio data from the WiFi chip to the RTIP
  TagCommand,   ///< Command data from WiFi... @TODO replace with real stuff
  TagState,     ///< State data from RTIP...   @TODO replace with real stuff
} I2SPIPayloadTag;

/// Source flags to indicate which side has sent this buffer
typedef enum
{
  fromWiFi = 0x0A; ///< From the WiFi chip to the RTIP
  fromRTIP = 0xFA; ///< From the RTIP to the WiFi
} I2SPIFrom;

/// Maximum bytes of image data payload
#define I2SPI_MAX_IMAGE_PAYLOAD (I2SPI_MAX_PAYLOAD-3)
/// Image data from the RTIP to the WiFi. @TODO This should be coming from CLAD etc.
typedef struct
{
  uint16_t dataLength; ///< Number of bytes of payload
  uint8_t  data[I2SPI_MAX_IMAGE_PAYLOAD]; ///< Actual payload data
  uint8_t  eof; ///< Flag for end of frame. 0 for all transfers except the final transfer for a given frame.
} ImgPayload;

/// Audio data from WiFi to the RTIP. @TODO This should be coming from CLAD etc.
typedef struct
{
  int16_t predictor;    ///< Initalization / state
  uint8_t samples[400]; ///< Audio sample data. @TODO Make length dependent on shared messaging include, clad, etc.
} AudioPayload;

/// Payload union type
typedef union
{
  ImgPayload   image;
  AudioPayload audio;
  uint32_t     raw[I2SPI_MAX_PAYLOAD/4]; ///< Interface on both sides works on int32s
} I2SPIPayload;

ct_assert(sizeof(I2SPIPayload) == I2SPI_MAX_PAYLOAD);

/// Struct for overall transaction including payload and footer.
typedef struct
{
  I2SPIPayload payload; ///< Transfer payload data
  uint16_t     seqNo;   ///< Sequence number so we can detect if we get out of sync. At 10MHz this will roll over in about 26 seconds
  uint8_t      tag;     ///< What kind of data is in the payload
  uint8_t      from;    ///< Which side is the data from
} I2SPITransfer;

ct_assert(sizeof(I2SPITransfer) == I2SPI_TRANSFER_SIZE);

/** Check for sequential sequence number.
 * This function may be suitable for asserting durring development on or causing I2SPI bus reset in production.
 * @NOTE This implementation is safe against rollover
 * @param seqNo The sequence number of the currently received buffer
 * @param previous The previous sequence number received from this source
 * @return true if the buffer is sequential, false if it is not indicating something has slipped.
 */
static inline bool i2spiSequential(uint16_t seqNo, uint16_t previous) { return (seqNo - previous) == 1; }

#endif
