/** @file Defines the structure of parameters stored in flash
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __ESP_NV_PARAMS_H_
#define __ESP_NV_PARAMS_H_

/// Header used to indicate we have real stored parameters not random data
#define NV_PARAMS_PREFIX 0xC020f3fa

typedef struct
{
  uint32_t PREFIX;
  char     ssid[32];
  char     pkey[64];
  uint8_t  wifiOpMode;
  uint8_t  wifiChannel;
  uint8_t  PADDING[2];
} NVParams;


#endif
