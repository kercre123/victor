/*
 * MINIMAL driver for ov2686 camera
 *
 * Daniel Casner <daniel@anki.com>
 */

#ifndef __OV2686_H
#define __OV2686_H
#include <media/v4l2-subdev.h>

struct ov2686_platform_data {
  int (*s_xclk) (struct v4l2_subdev *s, u32 on);
};

struct OVRegSettings
{
  u16 reg;
  u8  val;
  u8  mask;
};

#endif
