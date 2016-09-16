#ifndef __ESP_APP_IMAGE_HEADER_H_
#define __ESP_APP_IMAGE_HEADER_H_
/*******************************************************************************
 * @file Espressif application image header definition
 * @author Daniel Casner <daniel@anki.com>
 * See :robot/espressif_bootloader/README.md for details
 */

/// Offset from the start of the image area where the header can be found
#define APP_IMAGE_HEADER_OFFSET 0x10

/** Header placed at the start of each application image
 * Size must be a multiple of 4 to keep firmware image on word boundary.
 */
typedef struct
{
  int32_t size;        ///< Size of the image data in bytes including this structure
  int32_t imageNumber; ///< Incrementing number, left 0xFFFFffff in the OTA image
  int32_t evil;        ///< Must be set to 0 to indicate the image is complete, left 0xFFFFffff in the OTA image and written to 0 by application after download is complete
  uint8_t sha1[SHA1_DIGEST_LENGTH]; ///< The sha1 digest of the firmware image, not including this header.
} AppImageHeader;

#endif
