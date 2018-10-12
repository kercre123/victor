/**
 * File: camera_format.h
 *
 * Author: chapados / al
 * Created: 1/29/2018
 *
 * Description: Downsample de-Bayer algorithm for MIPI BGGR 10bit raw images
 *              Copied from mm-anki-camera in OS
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __anki_camera_debayer_h__
#define __anki_camera_debayer_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void bayer_mipi_bggr10_downsample(const uint8_t *bayer,
                                  uint8_t *out_rgb,
                                  int bayer_sx,
                                  int bayer_sy,
                                  int bpp);

#ifdef __cplusplus
}
#endif


#endif // __anki_camera_debayer_h__
