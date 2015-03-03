#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>

#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>

#include "ov2686.h"
#include "ov2686_reg_script_1600x1200_15fps_customized.h"
#include "ov2686_reg_script_800x600_30fps_customized.h"
#include "ov2686_code_timestamp.h"

/*** Register definitions */

static const u16 OV2686_REG_CHIP_ID_L = 0x300A; static const u8 OV2686_CHIP_ID_L_VALUE = 0x26;
static const u16 OV2686_REG_CHIP_ID_M = 0x300B; static const u8 OV2686_CHIP_ID_M_VALUE = 0x85;
static const u16 OV2686_REG_CHIP_ID_H = 0x300C; static const u8 OV2686_CHIP_ID_H_VALUE = 0x00;
static const u16 OV2686_REG_X_START_H = 0x3800;
static const u16 OV2686_REG_X_START_L = 0x3801;
static const u16 OV2686_REG_Y_START_H = 0x3802;
static const u16 OV2686_REG_Y_START_L = 0x3803;
static const u16 OV2686_REG_X_END_H   = 0x3804;
static const u16 OV2686_REG_X_END_L   = 0x3805;
static const u16 OV2686_REG_Y_END_H   = 0x3806;
static const u16 OV2686_REG_Y_END_L   = 0x3807;
static const u16 OV2686_REG_X_SIZE_H  = 0x3808;
static const u16 OV2686_REG_X_SIZE_L  = 0x3809;
static const u16 OV2686_REG_Y_SIZE_H  = 0x380A;
static const u16 OV2686_REG_Y_SIZE_L  = 0x380B;


/*** Camera format definitions */

#define OV2686_WINDOW_MAX_WIDTH  1600
#define OV2686_WINDOW_MIN_WIDTH   320
#define OV2686_WINDOW_MAX_HEIGHT 1200
#define OV2686_WINDOW_MIN_HEIGHT  240

static struct v4l2_mbus_framefmt FORMATS[] = {
  {
    .width  = OV2686_WINDOW_MAX_WIDTH,
    .height = OV2686_WINDOW_MAX_HEIGHT,
    .code   = V4L2_MBUS_FMT_SRGGB12_1X12,
    .colorspace = V4L2_COLORSPACE_SRGB,
    .field      = V4L2_FIELD_NONE,
  },
  {
    .width  = OV2686_WINDOW_MAX_WIDTH/2,
    .height = OV2686_WINDOW_MAX_HEIGHT/2,
    .code   = V4L2_MBUS_FMT_SRGGB12_1X12,
    .colorspace = V4L2_COLORSPACE_SRGB,
    .field      = V4L2_FIELD_NONE,
  }
};

static struct OVRegScript* FORMAT_SCRIPTS[] = {
  &REG_SCRIPT_1600x1200,
  &REG_SCRIPT_800x600
};

static const u32 NUM_FORMATS = ARRAY_SIZE(FORMATS);

/*** Type definitions */

struct ov2686_interval {
  const struct OVRegScript script;
  struct v4l2_fract interval;
};

struct ov2686_info {
  struct v4l2_subdev subdev;
  struct media_pad pad;
  struct ov2686_platform_data *pdata;
  const struct ov2686_interval *interval;
  struct v4l2_mbus_framefmt format;
  struct OVRegScript* format_script;
  struct v4l2_rect crop;
  struct mutex power_lock;
  int power_count;
  int streaming;
  int ident;
};

static const struct OVRegSettings OV2686_7p5fps_DATA[] = {
  {0x3086, 0x07, 0xff}
};

static const struct OVRegSettings OV2686_15fps_DATA[] = {
  {0x3086, 0x03, 0xff}
};

static const struct OVRegSettings OV2686_30fps_DATA[] = {
  {0x3086, 0x01, 0xff}
};

static const struct OVRegSettings OV2686_60fps_DATA[] = {
  {0x3086, 0x00, 0xff}
};

static const struct ov2686_interval ov2686_intervals[] = {
  {{1, OV2686_7p5fps_DATA}, { 7500, 1000000}},
  {{1, OV2686_15fps_DATA},  {15000, 1000000}},
  {{1,  OV2686_30fps_DATA},  {30000, 1000000}},
  {{1, OV2686_60fps_DATA},  {60000, 1000000}},
};

static const struct OVRegSettings OV2686_START_DATA = { 0x0100, 0x01, 0x01 };
static const struct OVRegScript OV2686_START = {1, &OV2686_START_DATA};

/*** Functions */

static inline struct ov2686_info *to_state(struct v4l2_subdev *subdev)
{
  return container_of(subdev, struct ov2686_info, subdev);
}

/* Low-level interface to clock/power */

static int __ov2686_set_power(struct ov2686_info *info, bool on)
{
  if (!on) {
    if (info->pdata->s_xclk) info->pdata->s_xclk(&info->subdev, 0);
    msleep(5);
  }
  else {
    if (info->pdata->s_xclk) info->pdata->s_xclk(&info->subdev, 1);
    msleep(5);
  }

  return 0;
}

static int ov2686_s_power(struct v4l2_subdev *sd, int on)
{
  struct ov2686_info *info = to_state(sd);
  int ret = 0;

  printk(KERN_INFO "ov2686_s_power %d\n", on);

  mutex_lock(&info->power_lock);

  /* If the power count is modified from 0 to != 0 or from != 0 to 0,
  * update the power state.
  */
  if (info->power_count == !on) {
    ret = __ov2686_set_power(info, !!on);
    if (ret < 0) goto out;
  }

  /* Update the power count. */
  info->power_count += on ? 1 : -1;
  WARN_ON(info->power_count < 0);

out:
  mutex_unlock(&info->power_lock);

  printk(KERN_INFO "\ts_power done\n");

  return ret;
}

static int ov2686_reg_read(struct i2c_client * client, u16 reg, u8* val) {
  int ret;
  struct i2c_msg msg[] = {
    {
      .addr	= client->addr,
      .flags	= 0,
      .len	= 2,
      .buf	= (u8 *)&reg,
    },
    {
      .addr	= client->addr,
      .flags	= I2C_M_RD,
      .len	= 1,
      .buf	= val,
    },
  };

  reg = swab16(reg);

  ret = i2c_transfer(client->adapter, msg, 2);
  if (ret != 2) {
    printk(KERN_WARNING "ov2686 failed reading register 0x%04x: %d\n", reg, ret);
    return ret;
  }

  return 0;
}

static int ov2686_reg_write(struct i2c_client *client, u16 reg, u8 val)
{
  struct i2c_msg msg;
  struct {
    u16 reg;
    u8 val;
  } __packed buf;
  int ret;

  reg = swab16(reg);

  buf.reg = reg;
  buf.val = val;

  msg.addr	= client->addr;
  msg.flags	= 0;
  msg.len		= 3;
  msg.buf		= (u8 *)&buf;

  ret = i2c_transfer(client->adapter, &msg, 1);
  if (ret < 0) {
    printk(KERN_WARNING "ov2686 failed writing %02x to register 0x%04x!\n", val, reg);
    return ret;
  }

  return 0;
}

static int ov2686_write_script(struct i2c_client *client, const struct OVRegScript* script)
{
  int ret, i;
  u8 val;
  printk(KERN_INFO "ov2686 writing script\n");
  for (i=0; i<script->len; ++i)
  {
    const struct OVRegSettings* rs = &script->script[i];
    if (rs->mask != 0xff) {
      ret = ov2686_reg_read(client, rs->reg, &val);
      if (ret < 0) return ret;
      val &= ~(rs->mask);
      val |= (rs->val & rs->mask);
    }
    else
    {
      val = rs->val;
    }
    ret = ov2686_reg_write(client, rs->reg, val);
    if (ret < 0) return ret;
  }

  return 0;
}

// Returns 1 if the register equals value, 0 if it is not equal or a negative error number
static int __ov2686_reg_equals(struct i2c_client *client, u16 reg, u8 value) {
  u8 data;
  int ret = ov2686_reg_read(client, reg, &data);
  if (ret < 0) {
    printk(KERN_WARNING "ov2686_reg_equals read failed\n");
    return ret;
  }
  else if (data == value) {
    return 1;
  }
  else {
    return 0;
  }
}

static int ov2686_s_stream(struct v4l2_subdev *subdev, int enable) {
  struct i2c_client *client;
  struct ov2686_info * info = to_state(subdev);
  int ret = 0;
  int crop_scale = 1;
  u16 writeVal;
  u8 readVal;

  printk(KERN_INFO "ov2686_s_stream %d\n", enable);

  client = v4l2_get_subdevdata(subdev);

  if (info->format_script == NULL) {
    printk(KERN_WARNING "ov2686_s_stream called with no format set\n");
    return -EINVAL;
  }

  if (info->power_count == 0) {
    printk(KERN_WARNING "ov2686_s_stream called with power count = %d\n", info->power_count);
  }
  if (enable == 0) {
    info->streaming = 0;
    ret = ov2686_reg_write(client, 0x0100, 0x00); // Software standby
  }
  else {
    printk(KERN_INFO "\tSending register script to OV2686\n");
    ov2686_write_script(client, info->format_script);

    printk(KERN_INFO "\tSending frame rate config to OV2686\n");
    ov2686_write_script(client, &info->interval->script);

    ret = 0;
    if (info->format.width < OV2686_WINDOW_MAX_WIDTH) crop_scale = 2; // If using skip scale crop by 1/2 as well
    printk(KERN_INFO "\tSending crop config to OV2686: [%d, %d, %d, %d], scale=%d\n",
    info->crop.left, info->crop.top, info->crop.width, info->crop.height, crop_scale);

    writeVal = info->crop.left*crop_scale;
    ret |= ov2686_reg_write(client, OV2686_REG_X_START_H, writeVal >> 8);
    ret |= ov2686_reg_write(client, OV2686_REG_X_START_L, writeVal & 0xff);

    writeVal = info->crop.top*crop_scale;
    ret |= ov2686_reg_write(client, OV2686_REG_Y_START_H, writeVal >> 8);
    ret |= ov2686_reg_write(client, OV2686_REG_Y_START_L, writeVal & 0xff);

    writeVal = (info->crop.width*crop_scale)+15;
    ret |= ov2686_reg_write(client, OV2686_REG_X_END_H, writeVal >> 8);
    ret |= ov2686_reg_write(client, OV2686_REG_X_END_L, writeVal & 0xff);

    writeVal = (info->crop.height*crop_scale)+15;
    ret |= ov2686_reg_write(client, OV2686_REG_Y_END_H, writeVal >> 8);
    ret |= ov2686_reg_write(client, OV2686_REG_Y_END_L, writeVal & 0xff);

    writeVal = info->crop.width;
    ret |= ov2686_reg_write(client, OV2686_REG_X_SIZE_H, writeVal >> 8);
    ret |= ov2686_reg_write(client, OV2686_REG_X_SIZE_L, writeVal & 0xff);

    writeVal = info->crop.height;
    ret |= ov2686_reg_write(client, OV2686_REG_Y_SIZE_H, writeVal >> 8);
    ret |= ov2686_reg_write(client, OV2686_REG_Y_SIZE_L, writeVal & 0xff);

    if (ret != 0) { // Using OR equals above so any error will flag though errno will be destroyed
      printk(KERN_WARNING "\tError writing crop configuration\n");
      return -EFAULT; // Just return a generic error
    }

    ret = ov2686_reg_read(client, 0x5080, &readVal);
    if (ret < 0) {
      printk(KERN_WARNING "\tCouldn't read back register 0x5080, err %d\n", ret);
      return ret;
    }
    else {
      printk(KERN_INFO "\tReg 0x5080 = 0x%02x\n", readVal);
    }

    ret = ov2686_write_script(client, &OV2686_START);
    if (ret < 0) {
      printk(KERN_WARNING "\tCouldn't enable streaming\n");
      return ret;
    }
    else {
      printk(KERN_INFO "\tStreaming enabled\n");
    }


    info->streaming = 1;
  }
  return ret;
}

static int ov2686_get_frame_interval(struct v4l2_subdev *subdev, struct v4l2_subdev_frame_interval *fi) {
  struct ov2686_info * info = to_state(subdev);
  fi->interval = info->interval->interval;
  return 0;
}

static int ov2686_set_frame_interval(struct v4l2_subdev *subdev, struct v4l2_subdev_frame_interval *fi) {
  struct ov2686_info * info = to_state(subdev);
  int i = 0;
  int fr_time = 0;
  unsigned int err = UINT_MAX;
  unsigned int min_err = UINT_MAX;
  const struct ov2686_interval *fiv = &ov2686_intervals[0];

  if (fi->interval.denominator == 0) {
    return -EINVAL;
  }

  printk(KERN_INFO "ov2686_s_frame_interval %d/%d\n", fi->interval.numerator, fi->interval.denominator);

  fr_time = fi->interval.numerator * 10000 / fi->interval.denominator;
  for (i = 0; i < ARRAY_SIZE(ov2686_intervals); i++) {
    const struct ov2686_interval *iv = &ov2686_intervals[i];
    err = abs((iv->interval.numerator * 10000/ iv->interval.denominator) - fr_time);
    if (err < min_err) {
      fiv = iv;
      min_err = err;
    }
  }

  info->interval = fiv;

  return 0;
}

static int ov2686_enum_mbus_code(struct v4l2_subdev *subdev,
                                 struct v4l2_subdev_fh *fh,
                                 struct v4l2_subdev_mbus_code_enum *code) {
  printk(KERN_INFO "ov2686_enum_mbus_code\n");

  if ((code->pad) || (code->index >= NUM_FORMATS)) {
    return -EINVAL;
  }

  code->code = FORMATS[code->index].code;


  return 0;
}

static int ov2686_enum_frame_size(struct v4l2_subdev *subdev,
                                  struct v4l2_subdev_fh *fh,
                                  struct v4l2_subdev_frame_size_enum *fse) {
  int i = NUM_FORMATS;
  printk(KERN_INFO "ov2686_enum_frame_size\n");

  if (fse->code > 0) return -EINVAL;

  while(--i) {
    if (fse->code == FORMATS[i].code) break;
  }
  fse->code = FORMATS[0].code;
  fse->min_width  = OV2686_WINDOW_MIN_WIDTH;
  fse->max_width  = OV2686_WINDOW_MAX_WIDTH;
  fse->min_height = OV2686_WINDOW_MIN_HEIGHT;
  fse->max_height = OV2686_WINDOW_MAX_HEIGHT;

  return 0;
}

static int ov2686_enum_frame_interval(struct v4l2_subdev *sd,
                                      struct v4l2_subdev_fh *fh,
                                      struct v4l2_subdev_frame_interval_enum *fie) {
  printk(KERN_INFO "ov2686_enum_frame_interval\n");

  // Function stubbed out to only return 30 fps

  if (fie->index > ARRAY_SIZE(ov2686_intervals)) {
    return -EINVAL;
  }

  v4l_bound_align_image(&fie->width, OV2686_WINDOW_MIN_WIDTH,
                        OV2686_WINDOW_MAX_WIDTH, 1,
                        &fie->height, OV2686_WINDOW_MIN_HEIGHT,
                        OV2686_WINDOW_MAX_HEIGHT, 1, 0);

  fie->interval = ov2686_intervals[fie->index].interval;

  return 0;
}

static int ov2686_get_format(struct v4l2_subdev *subdev,
                             struct v4l2_subdev_fh *fh,
                             struct v4l2_subdev_format *fmt) {
  struct ov2686_info * info = to_state(subdev);

  printk(KERN_INFO "ov2686_get_format\n");

  switch (fmt->which) {
    case V4L2_SUBDEV_FORMAT_TRY:
      fmt->format = *v4l2_subdev_get_try_format(fh, fmt->pad);
      printk(KERN_INFO "\tTry format\n");
      break;
    case V4L2_SUBDEV_FORMAT_ACTIVE:
      fmt->format = info->format;
      printk(KERN_INFO "\tActive format\n");
      break;
    default:
      printk(KERN_WARNING "ov2686_get_format invalid\n");
      return -EINVAL;
  }

  printk(KERN_INFO "ov2686 format(%d): %dx%d code=%x colorspace=%x field=%x\n", fmt->which, fmt->format.width,
         fmt->format.height, fmt->format.code, fmt->format.colorspace, fmt->format.field);

  return 0;
}

static int ov2686_set_format(struct v4l2_subdev *subdev,
                             struct v4l2_subdev_fh *fh,
                             struct v4l2_subdev_format *fmt) {
  struct ov2686_info * info = to_state(subdev);

  printk(KERN_INFO "ov2686_set_format\n");

  if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
  {
    if (info->streaming)
    {
      printk(KERN_WARNING "\tov2686 device busy\n");
      return -EBUSY;
    }
    else
    {
      if ((fmt->format.width > OV2686_WINDOW_MAX_WIDTH/2) || (fmt->format.height > OV2686_WINDOW_MAX_HEIGHT/2))
      {
        printk(KERN_INFO "\tsetting full sensor resolution\n");
        info->format = FORMATS[0];
        info->format.code = fmt->format.code; // Substitute the requested code.
        info->format_script = FORMAT_SCRIPTS[0];
        info->crop.width  = FORMATS[0].width; // Default crop to full frame
        info->crop.height = FORMATS[0].height;
        info->crop.left = 0;
        info->crop.top  = 0;
      }
      else
      {
        printk(KERN_INFO "\tsetting quarter scale resolution\n");
        info->format = FORMATS[1];
        info->format.code = fmt->format.code; // Substitute the requested code.
        info->format_script = FORMAT_SCRIPTS[1];
        info->crop.width  = FORMATS[1].width; // Default crop to full frame
        info->crop.height = FORMATS[1].height;
        info->crop.left = 0;
        info->crop.top  = 0;
      }
    }
  }

  return ov2686_get_format(subdev, fh, fmt);
}

static int ov2686_get_crop(struct v4l2_subdev *subdev,
                           struct v4l2_subdev_fh *fh,
                           struct v4l2_subdev_crop *crop) {
  struct ov2686_info * info = to_state(subdev);

  printk(KERN_INFO "ov2686_get_crop\n");

  // Don't support cropping so stubout TODO unstub if nessisary
  switch(crop->which) {
    case V4L2_SUBDEV_FORMAT_TRY:
      crop->rect = *v4l2_subdev_get_try_crop(fh, crop->pad);
      break;
    case V4L2_SUBDEV_FORMAT_ACTIVE:
      crop->rect = info->crop;
      break;
    default:
      return -EINVAL;
  }

  return 0;
}

static int ov2686_set_crop(struct v4l2_subdev *subdev,
                           struct v4l2_subdev_fh *fh,
                           struct v4l2_subdev_crop *crop) {
  struct ov2686_info* info = to_state(subdev);

  printk(KERN_INFO "ov2686_set_crop\n");

  info->crop.left   = clamp(ALIGN(crop->rect.left,        2), 0,                 OV2686_WINDOW_MAX_WIDTH);
  info->crop.top    = clamp(ALIGN(crop->rect.top,         2), 0,                 OV2686_WINDOW_MAX_HEIGHT);
  info->crop.width  = clamp(ALIGN(crop->rect.width,  2), OV2686_WINDOW_MIN_WIDTH,  OV2686_WINDOW_MAX_WIDTH);
  info->crop.height = clamp(ALIGN(crop->rect.height, 2), OV2686_WINDOW_MIN_HEIGHT, OV2686_WINDOW_MAX_HEIGHT);

  return ov2686_get_crop(subdev, fh, crop);
}



static int ov2686_registered(struct v4l2_subdev *subdev) {
  struct i2c_client *client;
  struct ov2686_info *info;
  int ret = 0;

  printk(KERN_INFO "ov2686_registered\n");

  client = v4l2_get_subdevdata(subdev);
  info   = to_state(subdev);

  // Power on
  ret = __ov2686_set_power(info, 1);
  if (ret < 0) {
    printk(KERN_WARNING "ov2686 register, couldn't power up chip\n");
    return ret;
  }

  // Read version information
  ret = __ov2686_reg_equals(client, OV2686_REG_CHIP_ID_L, OV2686_CHIP_ID_L_VALUE);
  if (ret == 1) {
    ret = __ov2686_reg_equals(client, OV2686_REG_CHIP_ID_M, OV2686_CHIP_ID_M_VALUE);
    if (ret == 1) {
      ret = __ov2686_reg_equals(client, OV2686_REG_CHIP_ID_H, OV2686_CHIP_ID_H_VALUE);
    }
  }
  if (ret == 1) {
    printk(KERN_INFO "ov2686 detected at address 0x%02x\n", client->addr);
  }
  else {
    printk(KERN_INFO "ov2686 not detected at address 0x%02x\n", client->addr);
    return -ENODEV;
  }

  // Power off chip
  ret = __ov2686_set_power(info, 0);
  if (ret < 0) {
    printk(KERN_WARNING "ov2686 register, couldn't power down chip\n");
    return ret;
  }

  return ret;
}

static int ov2686_open(struct v4l2_subdev *subdev, struct v4l2_subdev_fh *fh) {
  struct ov2686_info *info;

  printk(KERN_INFO "ov2686_open called\n");

  info = to_state(subdev);

  // Do stuff to set up camera, format, crop, etc.
  return ov2686_s_power(subdev, 1);
}

static int ov2686_close(struct v4l2_subdev *subdev, struct v4l2_subdev_fh *fh) {
  printk(KERN_INFO "ov2686_close called\n");
  return ov2686_s_power(subdev, 0);
}


/*** Static data */

static const struct v4l2_subdev_core_ops ov2686_subdev_core_ops = {
  .s_power      = ov2686_s_power,
};

static const struct v4l2_subdev_video_ops ov2686_subdev_video_ops = {
  .s_stream         = ov2686_s_stream,
  .g_frame_interval = ov2686_get_frame_interval,
  .s_frame_interval = ov2686_set_frame_interval,
};

static const struct v4l2_subdev_pad_ops ov2686_subdev_pad_ops = {
  .enum_mbus_code      = ov2686_enum_mbus_code,
  .enum_frame_size     = ov2686_enum_frame_size,
  .enum_frame_interval = ov2686_enum_frame_interval,
  .get_fmt             = ov2686_get_format,
  .set_fmt             = ov2686_set_format,
  .get_crop            = ov2686_get_crop,
  .set_crop            = ov2686_set_crop,
};

static struct v4l2_subdev_ops ov2686_subdev_ops = {
  .core	 = &ov2686_subdev_core_ops,
  .video = &ov2686_subdev_video_ops,
  .pad   = &ov2686_subdev_pad_ops,
};

static struct v4l2_subdev_internal_ops ov2686_subdev_internal_ops = {
  .registered = &ov2686_registered,
  .open       = &ov2686_open,
  .close      = &ov2686_close,
};

/*** I2C module functions */

static int ov2686_probe(struct i2c_client *client, const struct i2c_device_id *device) {
  struct ov2686_info *info;
  struct ov2686_platform_data *pdata;
  int ret = 0;

  printk(KERN_INFO "ov2686 i2c probe\n");

  pdata = client->dev.platform_data;
  if (pdata == NULL) {
    printk(KERN_WARNING "No platform data\n");
    return -EINVAL;
  }

  printk(KERN_INFO "Allocating driver memory\n");

  info = kzalloc(sizeof(struct ov2686_info), GFP_KERNEL);
  if (info == NULL) {
    printk(KERN_WARNING "Failed to allocate driver memory\n");
    return -ENOMEM;
  }

  info->pdata = pdata;

  // Do some initial i2c camera communication here if nessisary

  printk(KERN_INFO "Initalizing V4L2 I2C subdevice\n");

  mutex_init(&info->power_lock);
  v4l2_i2c_subdev_init(&info->subdev, client, &ov2686_subdev_ops);
  info->subdev.internal_ops = &ov2686_subdev_internal_ops;

  printk(KERN_INFO "Initalizing media controller\n");

  info->pad.flags = MEDIA_PAD_FL_SOURCE;
  ret = media_entity_init(&info->subdev.entity, 1, &info->pad, 0);
  if (ret < 0) {
    printk(KERN_WARNING "ov2686 media_entity_init failed: %d\n", ret);
    return ret;
  }

  info->interval = &ov2686_intervals[0];

  info->subdev.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

  // Do more setup here
  printk(KERN_INFO "i2c probe complete\n");

  return ret;
}

static int ov2686_remove(struct i2c_client *client)
{
  struct ov2686_info *info = i2c_get_clientdata(client);

  printk(KERN_INFO "ov2686 i2c remove\n");

  v4l2_device_unregister_subdev(&info->subdev);
  media_entity_cleanup(&info->subdev.entity);

  kfree(info);

  return 0;
}


/*** Module Interface */

static const struct i2c_device_id ov2686_id[] = {
  { "ov2686", 0 },
  { }
};

MODULE_DEVICE_TABLE(i2c, ov2686_id);

static struct i2c_driver ov2686_driver = {
  .driver = {
    .owner	= THIS_MODULE,
    .name	= "ov2686",
  },
  .probe		= ov2686_probe,
  .remove		= ov2686_remove,
  .id_table	= ov2686_id,
};

static __init int init_ov2686(void) {
  printk(KERN_INFO "ov2686() driver init. TS=" CODE_TIMESTAMP "\n");
  return i2c_add_driver(&ov2686_driver);
}

static __exit void exit_ov2686(void) {
  printk(KERN_INFO "exit_ov2686() called\n");
  i2c_del_driver(&ov2686_driver);
}


module_init(init_ov2686);
module_exit(exit_ov2686);


MODULE_AUTHOR("Daniel Casner <daniel@anki.com>");
MODULE_DESCRIPTION("MINIMAL driver for ov2686 sensor");
MODULE_LICENSE("GPL");
