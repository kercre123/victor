/*
 * OmniVision OV2686 Camera Driver
 *
 * Copyright (C) 2015 Anki Inc.
 *
 * Based on OmniVision OV2686 Camera Driver and OmniVision OV7670 Camera Driver
 *
 */

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

#if 1
#define SENSDBG(fmt, ...) printk(KERN_INFO fmt, ## __VA_ARGS__)
#else
#define SENSDBG(fmt, ...)
#endif


#define DBG_ENTRY(x...) SENSDBG("%s:%d entry\n",__FUNCTION__, __LINE__)
#define DBG_ENTRYP(x) SENSDBG("%s:%d entry " #x "=%d \n" ,__FUNCTION__, __LINE__,x)

/*
 * Basic window sizes.
 */
#define VGA_WIDTH	640
#define VGA_HEIGHT	480
#define QVGA_WIDTH	320
#define QVGA_HEIGHT	240
#define CIF_WIDTH	352
#define CIF_HEIGHT	288
#define QCIF_WIDTH	176
#define	QCIF_HEIGHT	144
#define UXGA_WIDTH  1600
#define UXGA_HEIGHT 1200

/* Sensor pixel array is 492x656 */
#define OV2686_PIXEL_ARRAY_WIDTH 1600
#define OV2686_PIXEL_ARRAY_HEIGHT 1200

#define OV2686_WINDOW_MIN_WIDTH 40
#define OV2686_WINDOW_MIN_HEIGHT 30
#define OV2686_WINDOW_MAX_WIDTH (UXGA_WIDTH)
#define OV2686_WINDOW_MAX_HEIGHT (UXGA_HEIGHT)
#define OV2686_VSTART_DEF 0xc
#define OV2686_HSTART_DEF 0x69
/*
 * The 2686 sits on i2c with ID 0x78 when SID is set to 0
 */
#define OV2686_I2C_ADDR 0x78

/* Registers */



/* Non-existent register number to terminate value lists */
#define REG_DUMMY       0xff
/* Forward declaration */
struct ov2686_format_struct;

/* Structure defining element of register values array */
struct regval_list {
  unsigned char reg_num;
  unsigned char value;
  unsigned char mask;
};
#define REGVAL_LIST_END { REG_DUMMY, REG_DUMMY,}
/* Structure describing frame interval */
struct ov2686_interval
{
  const struct regval_list *regs;
  struct v4l2_fract interval;
};

/*
 * Information we maintain about a known sensor.
 */

struct ov2686_info {
  struct v4l2_subdev sd;
  struct media_pad pad;
  struct ov2686_format_struct *fmt;  /* Descriptor of current format */
  struct v4l2_rect curr_crop;
  struct v4l2_ctrl_handler ctrls;
  const struct ov2686_interval *curr_fi;
  struct ov2686_platform_data *pdata;
  struct mutex power_lock;
  int power_count;
/* Brightness and contrast controls affect each other
 * To keep track driver holds on to a handle of brightness control
 * Editing values directly is a legitimate technique by
 * Documentation/video4linux/v4l2-controls.txt
 */
  int setup; /* Flag to indicate that v4l2-ctl is replaying setup into driver on powerup vs runtime adjustments */
  int brightness; /* Cached value of actual brightness */
  struct	v4l2_ctrl *bright_ctrl;
  int streaming; /* flag to indicate if video stream is active */
};

static inline struct ov2686_info *to_state(struct v4l2_subdev *sd)
{
  return container_of(sd, struct ov2686_info, sd);
}

static int ov2686_read(struct v4l2_subdev *sd, unsigned char reg,
    unsigned char *value)
{
  struct i2c_client *client = v4l2_get_subdevdata(sd);
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
  if (ret < 0) {
    dev_err(&client->dev, "Failed reading register 0x%04x!\n", reg);
    return ret;
  }

  return 0;
}

static int ov2686_write(struct v4l2_subdev *sd, unsigned char reg,
    unsigned char value)
{
  struct i2c_client *client = v4l2_get_subdevdata(sd);
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
    dev_err(&client->dev, "Failed writing register 0x%04x!\n", reg);
    return ret;
  }

  return 0;
}

static int ov2686_write_mask(struct v4l2_subdev *sd, unsigned char reg, unsigned char value, unsigned char mask)
{
  unsigned char val;
  int ret;

  ret = ov2686_read(sd, reg, &val);
  if (ret < 0) {
    SENSDBG("write-mask read of register %02x failed!\n", reg);
    return ret;
  }

  val = val & (~mask);
  val |= value & mask;

  ret = ov2686_write(sd, reg, val);
  if (ret < 0) {
    SENSDBG("write-mask write to register %02x failed!\n", reg);
    return ret;
  }

  return 0;
}


/*
 * Write a list of register settings; ff/ff ends the list
 */
static int ov2686_write_array(struct v4l2_subdev *sd, const struct regval_list *vals)
{
  while (vals->reg_num != REG_DUMMY || vals->value != REG_DUMMY) {
    int ret;
    if (vals->mask)
      ret=ov2686_write_mask(sd, vals->reg_num, vals->value, vals->mask);
    else
      ret = ov2686_write(sd, vals->reg_num, vals->value);
    if (ret < 0)
      return ret;
    vals++;
  }
  return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int ov2686_init(struct v4l2_subdev *sd)
{
  // For now just leave the sensor with default settings
  //return ov2686_write_array(sd, ov2686_default_regs);
  return 0;
}

static int ov2686_detect(struct v4l2_subdev *sd)
{
  // For prototype implementation, assume we have the right hardware

  /* It is right sensor, do things, which need to be done ASAP.
  ** Sensor comes out of both hard and soft reset with
  ** enabled outputs and pll.
  ** To avoid causing grief to the host, disable outputs immediately.
  ** Normal operation controls those at s_power and s_stream time as needed.
  */
  //ov2686_write_mask(sd, REG_REG0C, 0, ( REG0C_DATAOUT_ENABLE |  REG0C_CTLOUT_ENABLE));
  //ov2686_write_mask(sd, REG_PLL, 0x8,0x8);
  return 0;
}

static int ov2686_set_scaling( struct v4l2_subdev *sd)
{
  struct ov2686_info *info = to_state(sd);
  int ret = 0;
  /* Dissable for now
  ret =  ov2686_write(sd,REG_REGC8,(info->curr_crop.width >> 8) &3);
  if (ret >= 0) ret = ov2686_write(sd,REG_REGC9,info->curr_crop.width & 0xff);
  if (ret >= 0) ret = ov2686_write(sd,REG_REGCA,(info->curr_crop.height >> 8) & 3);
  if (ret >= 0) ret = ov2686_write(sd,REG_REGCB,info->curr_crop.height & 0xff);
  if (ret >= 0) ret = ov2686_write(sd,REG_REGCC,(info->fmt->format.width >> 8) & 3);
  if (ret >= 0) ret = ov2686_write(sd,REG_REGCD,info->fmt->format.width & 0xff);
  if (ret >= 0) ret = ov2686_write(sd,REG_REGCE,(info->fmt->format.height >> 8) & 3);
  if (ret >= 0) ret = ov2686_write(sd,REG_REGCF,info->fmt->format.height & 0xff);
  */
  return ret;
}


/*
 * Code for dealing with controls.
 */

/* Low-level interface to clock/power */
static int ov2686_power_on(struct ov2686_info *info)
{
  if (info->pdata->s_xclk)
    info->pdata->s_xclk(&info->sd,1);
  msleep(1);
  return 0;
}

static int ov2686_power_off(struct ov2686_info *info)
{
  if (info->pdata->s_xclk)
    info->pdata->s_xclk(&info->sd,0);
  return 0;
}
static int __ov2686_set_power(struct ov2686_info *info, bool on)
{
  int ret;

  if (!on) {
    ov2686_power_off(info);
    return 0;
  }
  ret = ov2686_power_on(info);
  if (ret < 0)
    return ret;

  return ret;
}
/* -----------------------------------------------------------------------------
 * V4L2 subdev video operations
 */

static int ov2686_s_stream(struct v4l2_subdev *sd, int enable)
{
  struct ov2686_info *info = to_state(sd);
  int ret;

  ret = 0;
  if (info->power_count == 0){
    return -EINVAL;
  }
  if (0 == enable) {
    info->streaming = 0;
  } else {
    info->streaming = 1;
  }
  return ret;
}

static int ov2686_g_frame_interval(struct v4l2_subdev *sd,
                                   struct v4l2_subdev_frame_interval *fi)
{

  struct ov2686_info *info = to_state(sd);

        fi->interval = info->curr_fi->interval;

        return 0;
}

static int ov2686_s_frame_interval(struct v4l2_subdev *sd,
                                   struct v4l2_subdev_frame_interval *fi)
{
  struct ov2686_info *info = to_state(sd);
  int i, fr_time;
  unsigned int err, min_err = UINT_MAX;
  const struct ov2686_interval *fiv = &ov2686_intervals[0];

  if (fi->interval.denominator == 0)
                return -EINVAL;

  fr_time = fi->interval.numerator * 10000 / fi->interval.denominator;
        for (i = 0; i < ARRAY_SIZE(ov2686_intervals); i++) {
    const struct ov2686_interval *iv = &ov2686_intervals[i];
    err = abs((iv->interval.numerator * 10000/ iv->interval.denominator) - fr_time);
                if (err < min_err) {
                        fiv = iv;
                        min_err = err;
    }
  }

  info->curr_fi = fiv;

        return 0;
}

/* -----------------------------------------------------------------------------
 * V4L2 subdev pad operations
 */

static int ov2686_enum_mbus_code(struct v4l2_subdev *sd,
                                  struct v4l2_subdev_fh *fh,
                                  struct v4l2_subdev_mbus_code_enum *code)
{

  if ((code->pad) || (code->index >= N_OV2686_FMTS ))
      return -EINVAL;

  code->code = ov2686_formats[code->index].format.code;
  return 0;

}
static int ov2686_enum_frame_size(struct v4l2_subdev *sd,
          struct v4l2_subdev_fh *fh,
          struct v4l2_subdev_frame_size_enum *fse)
{
  int i =  N_OV2686_FMTS;
  if (fse->index > 0) return -EINVAL;

  while(--i)
    if (fse->code == ov2686_formats[i].format.code)
      break;
  fse->code = ov2686_formats[i].format.code;

  fse->min_width = OV2686_WINDOW_MIN_WIDTH ;
  fse->max_width = OV2686_WINDOW_MAX_WIDTH ;
  fse->min_height = OV2686_WINDOW_MIN_HEIGHT;
  fse->max_height = OV2686_WINDOW_MAX_HEIGHT;

  return 0;
}

static struct v4l2_mbus_framefmt *
__ov2686_get_pad_format(struct ov2686_info *info, struct v4l2_subdev_fh *fh,
                         unsigned int pad, u32 which)
{
        switch (which) {
        case V4L2_SUBDEV_FORMAT_TRY:
                return v4l2_subdev_get_try_format(fh, pad);
        case V4L2_SUBDEV_FORMAT_ACTIVE:
                return &info->fmt->format;
        default:
                return NULL;
        }
}

static struct v4l2_rect *
__ov2686_get_pad_crop(struct ov2686_info *info, struct v4l2_subdev_fh *fh,
          unsigned int pad, u32 which)
{
  switch (which) {
  case V4L2_SUBDEV_FORMAT_TRY:
    return v4l2_subdev_get_try_crop(fh, pad);
  case V4L2_SUBDEV_FORMAT_ACTIVE:
    return &info->curr_crop;
  default:
    return NULL;
  }
}

static int ov2686_get_format(struct v4l2_subdev *sd,
                              struct v4l2_subdev_fh *fh,
                              struct v4l2_subdev_format *fmt)
{
  struct ov2686_info *info = to_state(sd);
  struct v4l2_mbus_framefmt *__format;

  __format = __ov2686_get_pad_format(info, fh, fmt->pad,
             fmt->which);
  if (__format == NULL) return -EINVAL;
  fmt->format = *__format;
  return 0;
}

static int ov2686_set_format(struct v4l2_subdev *sd,
                              struct v4l2_subdev_fh *fh,
                              struct v4l2_subdev_format *format)
{
  struct ov2686_info *info = to_state(sd);
  struct v4l2_mbus_framefmt *__format;
  struct v4l2_rect *__crop;
  unsigned int width;
  unsigned int height;
  int i =  ARRAY_SIZE(ov2686_formats);

  if ((format->which == V4L2_SUBDEV_FORMAT_ACTIVE) &&
      (info->streaming))
    return -EBUSY;

  /* match format against array of supported formats */
  while (--i)
    if (ov2686_formats[i].format.code == format->format.code) break;
  info->fmt =  &ov2686_formats[i];

  __crop = __ov2686_get_pad_crop(info, fh, format->pad, format->which);
  /* ov2686 can only scale size down, not up */
  width = clamp_t(unsigned int, ALIGN(format->format.width, 2),
                        OV2686_WINDOW_MIN_WIDTH, __crop->width);
  height = clamp_t(unsigned int, ALIGN(format->format.height, 2),
                        OV2686_WINDOW_MIN_HEIGHT, __crop->height);

  __format = __ov2686_get_pad_format(info, fh, format->pad,
             format->which);
  if (__format == NULL) return -EINVAL;
  __format->width = width;
  __format->height = height;
  format->format = *__format;
  return 0;
}

static int ov2686_get_crop(struct v4l2_subdev *sd,
         struct v4l2_subdev_fh *fh,
         struct v4l2_subdev_crop *crop)
{
  struct ov2686_info *info = to_state(sd);
  struct v4l2_rect *rect;

  rect = __ov2686_get_pad_crop(info, fh, crop->pad,
             crop->which);
  if (!rect)
    return -EINVAL;
  crop->rect = *rect;

  return 0;
}
static int ov2686_set_crop(struct v4l2_subdev *sd,
         struct v4l2_subdev_fh *fh,
         struct v4l2_subdev_crop *crop)
{
  struct ov2686_info *info = to_state(sd);
  struct v4l2_mbus_framefmt *__format;
  struct v4l2_rect *__crop;
  struct v4l2_rect rect;
  /* It is unclear how to control left corner of crop rectangle */
  rect.left =0;
  rect.top = 0;
  rect.width = clamp(ALIGN(crop->rect.width, 2),
                           OV2686_WINDOW_MIN_WIDTH,
                           OV2686_WINDOW_MAX_WIDTH);
        rect.height = clamp(ALIGN(crop->rect.height, 2),
                            OV2686_WINDOW_MIN_HEIGHT,
                            OV2686_WINDOW_MAX_HEIGHT);
  __crop = __ov2686_get_pad_crop(info, fh, crop->pad, crop->which);

  if (rect.width != __crop->width || rect.height != __crop->height) {
    /* Reset the output image size if the crop rectangle size has
                 * been modified.
                 */
    __format = __ov2686_get_pad_format(info, fh, crop->pad,
               crop->which);
                __format->width = rect.width;
                __format->height = rect.height;
        }

        *__crop = rect;
        crop->rect = rect;

  return 0;
}

static int ov2686_enum_frame_interval(struct v4l2_subdev *sd,
                              struct v4l2_subdev_fh *fh,
                              struct v4l2_subdev_frame_interval_enum *fie)
{

        if (fie->index > ARRAY_SIZE(ov2686_intervals))
                return -EINVAL;

        v4l_bound_align_image(&fie->width, OV2686_WINDOW_MIN_WIDTH,
                              OV2686_WINDOW_MAX_WIDTH, 1,
                              &fie->height, OV2686_WINDOW_MIN_HEIGHT,
                              OV2686_WINDOW_MAX_HEIGHT, 1, 0);

        fie->interval = ov2686_intervals[fie->index].interval;

        return 0;
}

/* -----------------------------------------------------------------------------
 * V4L2 subdev control operations
 */
/*
 * Hue also requires messing with the color matrix.  It also requires
 * trig functions, which tend not to be well supported in the kernel.
 * So here is a simple table of sine values, 0-90 degrees, in steps
 * of five degrees.  Values are multiplied by 1000.
 *
 * The following naive approximate trig functions require an argument
 * carefully limited to -180 <= theta <= 180.
 */
#define SIN_STEP 5
static const int ov2686_sin_table[] = {
     0,	 87,   173,   258,   342,   422,
   500,	573,   642,   707,   766,   819,
   866,	906,   939,   965,   984,   996,
  1000
};

static int ov2686_sine(int theta)
{
  int chs = 1;
  int sine;

  if (theta < 0) {
    theta = -theta;
    chs = -1;
  }
  if (theta <= 90)
    sine = ov2686_sin_table[theta/SIN_STEP];
  else {
    theta = 180 - theta;
    sine = ov2686_sin_table[theta/SIN_STEP];
  }
  return sine*chs;
}

static int ov2686_cosine(int theta)
{
  theta = 90 - theta;
  if (theta > 180)
    theta -= 360;
  else if (theta < -180)
    theta += 360;
  return ov2686_sine(theta);
}

static int ov2686_s_hue(struct v4l2_subdev *sd, int value)
{
#define HUE_MIN -180
#define HUE_MAX 180
#define HUE_STEP 5
  int ret = 0;
  /* Disable for now
  int sinth, costh;
  unsigned char sign_hue;
  if (value < -180 || value > 180)
    return -ERANGE;
  // 1.7 fixpoint format for sin and cos
  sinth = ov2686_sine(value);
  costh = ov2686_cosine(value);
  if (sinth < 0) sinth = -sinth;
  if (costh < 0) costh = -costh;
  sinth = ((sinth << 4)+124) / 125;	// x = x*128/1000;
  costh = ((costh << 4)+124) / 125;	// x = x*128/1000;
  */
  /* Sight bits go into bits 0:1:4:5 of reg 0xDC
   * 0.1.4.5
   * 1.0.0.0 := 0 <= theta < pi/2
   * 0.1.0.0 := -pi/2 <= theta < 0
   * 1.0.1.1 := pi/2 < theta
   * 0.1.1.1 := theta < -pi/2
   */
  /*
  if (value >= 0 ) {
    if (value < 90) sign_hue = 0x01;
    else sign_hue = 0x31;
  } else {
    if (value < -90) sign_hue = 0x32;
    else sign_hue = 0x02;
  }

  // Write values
  // enable SDE
  ret = ov2686_write_mask(sd, REG_REG81, 0x20, 0x20);
  // enable Hue side-control
  if (ret >= 0) ret = ov2686_write_mask(sd,REG_SDECTRL,SDECTRL_HUE_EN,SDECTRL_HUE_EN);
  // Write values and signs
  if (ret >= 0) ret = ov2686_write(sd,REG_REGD6, costh);
  if (ret >= 0) ret = ov2686_write(sd,REG_REGD7, sinth);
  if (ret >= 0) ret = ov2686_write_mask(sd,REG_REGDC,sign_hue,0x33);
  */
  return ret;
}

static int ov2686_s_brightness(struct v4l2_subdev *sd, int value)
{
#define BRIGHTNESS_MIN -255
#define BRIGHTNESS_MAX 255
  int ret = 0;
  /* Disable for now
  unsigned char v, sign_bright;
  struct ov2686_info *info = to_state(sd);

  if (value < -255 || value > 255)
    return -EINVAL;
  // enable SDE
  ret = ov2686_write_mask(sd, REG_REG81, 0x20, 0x20);

  if (value < 0) {
    sign_bright = 0x8;
    v = (unsigned char) ((-value));
  } else {
    sign_bright = 0;
    v = (unsigned char) (value);
  }
  // Write value, side-control and sign
  if (ret >= 0) ret = ov2686_write(sd, REG_REGD3, v);
  if (0 == info->setup)
    info->brightness = value;

  if (ret >= 0) ret = ov2686_write_mask(sd, REG_SDECTRL, SDECTRL_CONT_EN, SDECTRL_CONT_EN);
  if (ret >= 0) ret = ov2686_write_mask(sd, REG_REGDC, sign_bright, 0x8);
  */
  return ret;
}

static int ov2686_s_contrast(struct v4l2_subdev *sd, int value)
{
#define CONTRAST_MIN -4
#define CONTRAST_MAX 4
  // For some reason setting contrast sets brightness as well
  // If brightnesss is undesirable, set it after setting contrast
  int ret = 0;
  /* Disable for now
  unsigned char v, sign_cont, imply_brightness;
  struct ov2686_info *info = to_state(sd);

  static unsigned char brightness_list[] =
    { 0xd0, 0x80, 0x48, 0x20 };
  if (value < CONTRAST_MIN || value > CONTRAST_MAX)
    return -ERANGE;

  // enable SDE
  ret = ov2686_write_mask(sd, REG_REG81, 0x20, 0x20);
  if (value < 0) {
    sign_cont = 0x4;
    imply_brightness =
      brightness_list[ARRAY_SIZE(brightness_list) + value];
  } else {
    sign_cont = 0x0;
    imply_brightness = 0;
  }
  v = 0x20 + (value*4);
  if (ret >= 0) ret = ov2686_write(sd, REG_REGD5, 0x20);
  if (ret >= 0) ret = ov2686_write(sd, REG_REGD4, v);
  if (ret >= 0) {
    if (0 == info->setup) {
      ret = ov2686_write_mask(sd, REG_REGDC, 0, 0x8);
      ret = ov2686_write(sd, REG_REGD3, imply_brightness);
      info->bright_ctrl->cur.val=imply_brightness;
      info->brightness = imply_brightness;
    } else {
      // brightness setup will take care of setting
    }
  }
  if (ret >= 0) ret = ov2686_write_mask(sd, REG_REGDC, sign_cont, 0x4);
  if (ret >= 0) ret = ov2686_write_mask(sd, REG_SDECTRL, SDECTRL_CONT_EN, SDECTRL_CONT_EN);
  */
  return ret;
}


static int ov2686_s_hflip(struct v4l2_subdev *sd, int value)
{
  int ret = 0;
  /* Disable for now
  unsigned char v = (value) ? REG0C_MIRROR : 0;
  ret = ov2686_write_mask(sd,REG_REG0C, v, REG0C_MIRROR);
  */
  return ret;
}

static int ov2686_s_vflip(struct v4l2_subdev *sd, int value)
{
  int ret = 0;
  /* Disable for now
  unsigned char v = (value) ? REG0C_VFLIP : 0;
  ret = ov2686_write_mask(sd,REG_REG0C, v, REG0C_VFLIP);
  */
  return ret;
}
static int ov2686_s_sharpness(struct v4l2_subdev *sd, int value)
{
#define SHARPNESS_MIN -1
#define SHARPNESS_MAX 5
  int ret = 0;
  /* Disable for now
  if (-1 == value) { // Sharpness off
    ret = ov2686_write_mask(sd, REG_REGB4, 0x20,0x20);
    if (ret >= 0) ret = ov2686_write_mask(sd, REG_REGB6, 0,0x1f);
  } else if (0 == value) { // Sharpness Auto
    ret = ov2686_write_mask(sd, REG_REGB4, 0,0x20);
    if (ret >= 0) ret = ov2686_write_mask(sd, REG_REGB6, 2,0x1f);
    if (ret >= 0) ret = ov2686_write(sd, REG_REGB8, 9);
  } else {
    static unsigned char sharp_vals[SHARPNESS_MAX] =
      { 1,2,3,5,8 };
    if (value > ARRAY_SIZE(sharp_vals)) return -ERANGE;
    ret = ov2686_write_mask(sd, REG_REGB4, 0x20,0x20);
    if (ret >= 0) ret = ov2686_write_mask(sd, REG_REGB6, sharp_vals[value-1],0x1f);
  }
  */
  return ret;
}
static int ov2686_s_saturation(struct v4l2_subdev *sd, int value)
{
#define SATURATION_MIN -1
#define SATURATION_MAX 8
  int ret = 0;
  /* Disable for now
  if ((value < SATURATION_MIN) || (value > SATURATION_MAX)) return -ERANGE;
  if (-1 == value) {
    ret = ov2686_write_mask(sd, REG_SDECTRL, 0,SDECTRL_SAT_EN);
  } else {
    ret =  ov2686_write_mask(sd, REG_SDECTRL,SDECTRL_SAT_EN ,SDECTRL_SAT_EN);
    if (ret >= 0) {
      ret= ov2686_write(sd,REG_REGD8,value<<4);
      ret |= ov2686_write(sd,REG_REGD9,value<<4);
    }
  }
  */
  return ret;
}
static int ov2686_s_exposure(struct v4l2_subdev *sd, int value)
{
#define EXPOSURE_MIN -5
#define EXPOSURE_MAX 5
        struct {
                unsigned char WPT; /* reg 0x24 */
                unsigned char BPT; /* reg 0x25 */
                unsigned char VPT; /* reg 0x26 */
        } exposure_avg[1 + EXPOSURE_MAX - EXPOSURE_MIN] = {
    /* -1.7EV */ { 0x50 , 0x40 , 0x63 },
    /* -1.3EV */ { 0x58 , 0x48 , 0x73 },
    /* -1.0EV */ { 0x60 , 0x50 , 0x83 },
    /* -0.7EV */ { 0x68 , 0x58 , 0x93 },
                /* -0.3EV */ { 0x70 , 0x60 , 0xa3 },
                /* default */ { 0x78 , 0x68 , 0xb3 },
                /* 0.3EV */ { 0x80 , 0x70 , 0xc3 },
                /* 0.7EV */ { 0x88 , 0x78 , 0xd3 },
                /* 1.0EV */ { 0x90 , 0x80 , 0xd3 },
    /* 1.3EV */ { 0x98 , 0x88 , 0xe3 },
    /* 1.7EV */ { 0xa0 , 0x90 , 0xe3 },
        };
  int ret = 0;
  /* disable for now
  int	index = value - EXPOSURE_MIN;
  if ((index < 0) || (index > ARRAY_SIZE(exposure_avg) )) return -ERANGE;
  ret=ov2686_write(sd,REG_WPT,exposure_avg[index].WPT);
  ret=ov2686_write(sd,REG_BPT,exposure_avg[index].BPT);
  ret=ov2686_write(sd,REG_VPT,exposure_avg[index].VPT);
  */
  return ret;
}

static int ov2686_s_colorfx(struct v4l2_subdev *sd, int value)
{
  int ret = 0;
  /* Disable for now
  switch(value) {
  case V4L2_COLORFX_NONE:
    ret = ov2686_write_mask(sd,REG_REG28, 0, 0x80);
    ret |= ov2686_write_mask(sd, REG_SDECTRL, 0,SDECTRL_FIX_UV);
    break;
  case V4L2_COLORFX_BW:
    ret = ov2686_write_mask(sd,REG_REG28, 0, 0x80);
    ret |= ov2686_write_mask(sd, REG_SDECTRL, SDECTRL_FIX_UV,SDECTRL_FIX_UV);
    ret |= ov2686_write(sd,REG_REGDA, 0x80);
    ret |= ov2686_write(sd,REG_REGDB, 0x80);
    break;
  case V4L2_COLORFX_SEPIA:
    ret = ov2686_write_mask(sd,REG_REG28, 0, 0x80);
    ret |= ov2686_write_mask(sd, REG_SDECTRL, SDECTRL_FIX_UV,SDECTRL_FIX_UV);
    ret |= ov2686_write(sd,REG_REGDA, 0x40);
    ret |= ov2686_write(sd,REG_REGDB, 0xa0);
    break;
  case V4L2_COLORFX_NEGATIVE :
    ret = ov2686_write_mask(sd,REG_REG28, 0x80, 0x80);
    ret |= ov2686_write_mask(sd, REG_SDECTRL, 0,SDECTRL_FIX_UV);
    break;
  case V4L2_COLORFX_SKY_BLUE:
    ret = ov2686_write_mask(sd,REG_REG28, 0, 0x80);
    ret |= ov2686_write_mask(sd, REG_SDECTRL, SDECTRL_FIX_UV,SDECTRL_FIX_UV);
    ret |= ov2686_write(sd,REG_REGDA, 0xa0);
    ret |= ov2686_write(sd,REG_REGDB, 0x40);
    break;
  case V4L2_COLORFX_GRASS_GREEN:
    ret = ov2686_write_mask(sd,REG_REG28, 0, 0x80);
    ret |= ov2686_write_mask(sd, REG_SDECTRL, SDECTRL_FIX_UV,SDECTRL_FIX_UV);
    ret |= ov2686_write(sd,REG_REGDA, 0x60);
    ret |= ov2686_write(sd,REG_REGDB, 0x60);
    break;
  case V4L2_COLORFX_EMBOSS: // Reddish
    ret = ov2686_write_mask(sd,REG_REG28, 0, 0x80);
    ret |= ov2686_write_mask(sd, REG_SDECTRL, SDECTRL_FIX_UV,SDECTRL_FIX_UV);
    ret |= ov2686_write(sd,REG_REGDA, 0x80);
    ret |= ov2686_write(sd,REG_REGDB, 0xc0);
    break;

    break;
  default:
    return -EINVAL;
  }
  */
  return ret;
}

static int ov2686_test_pattern(struct v4l2_subdev *sd, int value)
{
/*
 * 0: None
 * 1: Color bar overlay
 * 2: Framecount intenciry color bar
 * 3: Solid color bar
 */
 /* Disable for now
  switch(value) {
  case 0:
    ov2686_write_mask(sd, REG_REG82, 0x0, 0xc);
    return ov2686_write_mask(sd, REG_REG0C, 0, REG0C_COLOR_BAR);
  case 1:
    ov2686_write_mask(sd, REG_REG82, 0x0, 0xc);
    return ov2686_write_mask(sd, REG_REG0C, REG0C_COLOR_BAR, REG0C_COLOR_BAR);
  case 2:
    ov2686_write_mask(sd, REG_REG0C, 0, REG0C_COLOR_BAR);
    return ov2686_write_mask(sd, REG_REG82, 0x8, 0xc);
  case 3:
    ov2686_write_mask(sd, REG_REG0C, 0, REG0C_COLOR_BAR);
    return ov2686_write_mask(sd, REG_REG82, 0xc, 0xc);
  }
  return -ERANGE;
  */
  return 0;
}

static int ov2686_s_ctrl(struct v4l2_ctrl *ctrl)
{
  struct ov2686_info *info =
    container_of(ctrl->handler, struct ov2686_info, ctrls);
  struct v4l2_subdev *sd = &info->sd;

  switch(ctrl->id) {
  case V4L2_CID_BRIGHTNESS:
    return ov2686_s_brightness(sd, ctrl->val);
  case V4L2_CID_CONTRAST:
    return ov2686_s_contrast(sd, ctrl->val);
  case V4L2_CID_HUE:
    return ov2686_s_hue(sd, ctrl->val);
  case V4L2_CID_VFLIP:
    return ov2686_s_vflip(sd, ctrl->val);
  case V4L2_CID_HFLIP:
    return ov2686_s_hflip(sd, ctrl->val);
  case V4L2_CID_SHARPNESS:
    return ov2686_s_sharpness(sd, ctrl->val);
  case V4L2_CID_SATURATION:
    return ov2686_s_saturation(sd, ctrl->val);
  case V4L2_CID_EXPOSURE:
    return ov2686_s_exposure(sd, ctrl->val);
  case V4L2_CID_COLORFX:
    return ov2686_s_colorfx(sd, ctrl->val);
  case V4L2_CID_TEST_PATTERN:
    return ov2686_test_pattern(sd, ctrl->val);
  default:
    return -EINVAL;
  }

  SENSDBG("%s: did not honor entry CID= %x %s",__FUNCTION__,ctrl->id,
    v4l2_ctrl_get_name(ctrl->id) ?  v4l2_ctrl_get_name(ctrl->id) : "???");

  return -EINVAL;

}
static struct v4l2_ctrl_ops ov2686_ctrl_ops = {
        .s_ctrl = ov2686_s_ctrl,
};

static const char *ov2686_test_pattern_menu[] = {
  "Disabled",
  "Overlay Vertical Color Bars",
  "Gradual Vertical Color Bars",
  "Solid Vertical Color Bars ",
};

static const struct v4l2_ctrl_config ov2686_ctrls[] = {
        {
                .ops            = &ov2686_ctrl_ops,
                .id             = V4L2_CID_TEST_PATTERN,
                .type           = V4L2_CTRL_TYPE_MENU,
                .name           = "Test Pattern",
                .min            = 0,
                .max            = ARRAY_SIZE(ov2686_test_pattern_menu) - 1,
                .step           = 0,
                .def            = 0,
                .flags          = 0,
                .menu_skip_mask = 0,
                .qmenu          = ov2686_test_pattern_menu,
        }
};



/* -----------------------------------------------------------------------------
 * V4L2 subdev core operations
 */

static int ov2686_s_power(struct v4l2_subdev *sd, int on)
{
  struct ov2686_info *info = to_state(sd);
  int ret = 0;

  mutex_lock(&info->power_lock);

  /* If the power count is modified from 0 to != 0 or from != 0 to 0,
   * update the power state.
   */
  if (info->power_count == !on) {
    ret = __ov2686_set_power(info, !!on);
    if (ret < 0)
      goto out;
  }

  /* Update the power count. */
  info->power_count += on ? 1 : -1;
  WARN_ON(info->power_count < 0);

out:
  mutex_unlock(&info->power_lock);
  return ret;
}

static int ov2686_g_chip_ident(struct v4l2_subdev *sd,
    struct v4l2_dbg_chip_ident *chip)
{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
  SENSDBG("%s id:%d none:%d\n",
         __FUNCTION__, chip->ident, V4L2_IDENT_NONE);
//	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_OV2686, 0);
  chip->ident =  V4L2_IDENT_OV2686;
  chip->revision = 0;

  return 0;
}

#ifdef CONFIG_VIDEO_ADV_DEBUG

static int ov2686_g_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
  struct i2c_client *client = v4l2_get_subdevdata(sd);
  unsigned char val = 0;
  int ret;

  if (!capable(CAP_SYS_ADMIN))
    return -EPERM;
  if (!v4l2_chip_match_i2c_client(client, &reg->match))
    return -EINVAL;
  ret = ov2686_read(sd, reg->reg & 0xff, &val);
  reg->val = val;
  reg->size = 1;
  return ret;
}

static int ov2686_s_register(struct v4l2_subdev *sd, struct v4l2_dbg_register *reg)
{
  struct i2c_client *client = v4l2_get_subdevdata(sd);

  if (!capable(CAP_SYS_ADMIN))
    return -EPERM;
  if (!v4l2_chip_match_i2c_client(client, &reg->match))
    return -EINVAL;
  ov2686_write(sd, reg->reg & 0xff, reg->val & 0xff);
  return 0;
}
#endif

/* fall-through for ioctls, which were not handled by the v4l2-subdev */
static   long ov2686_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg){
  switch (cmd) {
  case VIDIOC_QUERYCAP:
  {
    struct v4l2_capability *cap = (struct v4l2_capability *)arg;
    struct i2c_client *client = v4l2_get_subdevdata(sd);

    cap->version           = KERNEL_VERSION(0, 0, 1);
    strlcpy(cap->driver, "ov2686", sizeof(cap->driver));
    strlcpy(cap->card, "ov2686", sizeof(cap->card));
    snprintf(cap->bus_info, sizeof(cap->bus_info),
       "media i2c 0x%02x",client->addr);
    cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING;

    break;
  }
  default:
    SENSDBG("%s rejecting ioctl 0x%08x\n",__FUNCTION__,cmd);
    return -EINVAL;
  }
  return 0;
}


/* -----------------------------------------------------------------------
 * V4L2 subdev internal operations
 */
static int ov2686_registered(struct v4l2_subdev *sd)
{
          struct i2c_client *client = v4l2_get_subdevdata(sd);
          struct ov2686_info *info = to_state(sd);
          int ret;

          ret = ov2686_power_on(info);
          if (ret < 0) {
                  dev_err(&client->dev, "OV2686 power up failed\n");
                  return ret;
          }
          /* Read out the chip version register */
    ret = ov2686_detect(sd);
          if (ret < 0) {
                  dev_err(&client->dev, "OV2686 not detected, wrong manufacturer or version \n");
                  return -ENODEV;
          }

          ov2686_power_off(info);

          dev_info(&client->dev, "OV2686 detected at address 0x%02x\n",
                   client->addr);
          return ret;
}

static int ov2686_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
  struct v4l2_mbus_framefmt *format;
  struct v4l2_rect *crop;
  struct ov2686_info *info = to_state(sd);

  crop = v4l2_subdev_get_try_crop(fh, 0);
  crop->left = 0;
  crop->top = 0;
  crop->width = VGA_WIDTH;
  crop->height = VGA_HEIGHT;

  format = v4l2_subdev_get_try_format(fh, 0);
  format->width = VGA_WIDTH;
  format->height = VGA_HEIGHT;
  format->field = V4L2_FIELD_NONE;
  format->colorspace = V4L2_COLORSPACE_JPEG;;
  format->code =  V4L2_MBUS_FMT_YUYV8_2X8;

  return ov2686_s_power(sd, 1);

}

static int ov2686_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
  return ov2686_s_power(sd, 0);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops ov2686_subdev_core_ops = {
  .g_chip_ident = ov2686_g_chip_ident,
  .s_power = ov2686_s_power,
#ifdef CONFIG_VIDEO_ADV_DEBUG
  .g_register = ov2686_g_register,
  .s_register = ov2686_s_register,
#endif
  .ioctl = ov2686_ioctl,

};

static const struct v4l2_subdev_video_ops ov2686_subdev_video_ops = {
  .s_stream = ov2686_s_stream,
  .g_frame_interval = ov2686_g_frame_interval,
  .s_frame_interval = ov2686_s_frame_interval,
};

static struct v4l2_subdev_pad_ops ov2686_subdev_pad_ops = {
        .enum_mbus_code = ov2686_enum_mbus_code,
        .enum_frame_size = ov2686_enum_frame_size,
        .get_fmt = ov2686_get_format,
        .set_fmt = ov2686_set_format,
        .get_crop = ov2686_get_crop,
  .set_crop = ov2686_set_crop,
  .enum_frame_interval = ov2686_enum_frame_interval,
};

static const struct v4l2_subdev_ops ov2686_ops = {
  .core = &ov2686_subdev_core_ops,
  .video = &ov2686_subdev_video_ops,
  .pad = &ov2686_subdev_pad_ops,
};
static const struct v4l2_subdev_internal_ops ov2686_subdev_internal_ops = {
  .registered = ov2686_registered,
  .open = ov2686_open,
  .close = ov2686_close,
};
/* ----------------------------------------------------------------------- */

static int ov2686_probe(struct i2c_client *client,
      const struct i2c_device_id *id)
{
  struct ov2686_info *info;
  int ret;
  int i;
  struct ov2686_platform_data *pdata = client->dev.platform_data;
  struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
  if (pdata == NULL) {
    dev_err(&client->dev, "No platform data\n");
  }

  if ( !i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
    dev_err(&adapter->dev,
      "I2C-Adapter doesn't support "
      "I2C_FUNC_SMBUS_BYTE_DATA\n");
    return -EIO;
  }

  info = kzalloc(sizeof(struct ov2686_info), GFP_KERNEL);
  if (info == NULL)
    return -ENOMEM;

  info->pdata = pdata;

  v4l2_ctrl_handler_init(&info->ctrls,9 + ARRAY_SIZE(ov2686_ctrls));
  info->bright_ctrl =
    v4l2_ctrl_new_std(&info->ctrls,&ov2686_ctrl_ops,
          V4L2_CID_BRIGHTNESS,
          BRIGHTNESS_MIN, BRIGHTNESS_MAX, 1, 0);
  v4l2_ctrl_new_std(&info->ctrls,&ov2686_ctrl_ops,
        V4L2_CID_CONTRAST,
        CONTRAST_MIN, CONTRAST_MAX, 1, 0);
  v4l2_ctrl_new_std(&info->ctrls,&ov2686_ctrl_ops,
        V4L2_CID_VFLIP,
        0, 1, 1, 0);
  v4l2_ctrl_new_std(&info->ctrls,&ov2686_ctrl_ops,
        V4L2_CID_HFLIP,
        0, 1, 1, 0);
  v4l2_ctrl_new_std(&info->ctrls,&ov2686_ctrl_ops,
        V4L2_CID_HUE,
        HUE_MIN, HUE_MAX, HUE_STEP, 0);
  v4l2_ctrl_new_std(&info->ctrls,&ov2686_ctrl_ops,
        V4L2_CID_SHARPNESS,
        SHARPNESS_MIN, SHARPNESS_MAX, 1, -1);
  v4l2_ctrl_new_std(&info->ctrls,&ov2686_ctrl_ops,
        V4L2_CID_SATURATION,
        SATURATION_MIN, SATURATION_MAX, 1, -1);
  v4l2_ctrl_new_std(&info->ctrls,&ov2686_ctrl_ops,
        V4L2_CID_EXPOSURE,
        EXPOSURE_MIN, EXPOSURE_MAX, 1, 0);
  v4l2_ctrl_new_std_menu(&info->ctrls,&ov2686_ctrl_ops,
             V4L2_CID_COLORFX,
             V4L2_COLORFX_GRASS_GREEN,
             (0xffffffffUL ^
        ((1<< V4L2_COLORFX_NONE) |
         (1<< V4L2_COLORFX_BW) |
         (1<<V4L2_COLORFX_SEPIA) |
         (1<<V4L2_COLORFX_NEGATIVE) |
         (1<<V4L2_COLORFX_SKY_BLUE) |
         (1<<V4L2_COLORFX_GRASS_GREEN) |
         (1<<V4L2_COLORFX_EMBOSS)))
             , 0);

  for (i = 0; i < ARRAY_SIZE(ov2686_ctrls); ++i)
    v4l2_ctrl_new_custom(&info->ctrls, &ov2686_ctrls[i], NULL);


  info->sd.ctrl_handler = &info->ctrls;

  mutex_init(&info->power_lock);
  v4l2_i2c_subdev_init(&info->sd, client, &ov2686_ops);


  info->sd.internal_ops = &ov2686_subdev_internal_ops;

  info->fmt = &ov2686_formats[0];

  info->sd.flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

  info->curr_crop.width = VGA_WIDTH;
        info->curr_crop.height = VGA_HEIGHT;
        info->curr_crop.left = 0;
        info->curr_crop.top = 0;

  info->curr_fi = &ov2686_intervals[0];

  info->pad.flags = MEDIA_PAD_FL_SOURCE;
#if defined(CONFIG_MEDIA_CONTROLLER)
  ret = media_entity_init(&info->sd.entity, 1, &info->pad, 0);
#endif
  if (ret < 0) {
    v4l2_ctrl_handler_free(&info->ctrls);
    media_entity_cleanup(&info->sd.entity);
    kfree(info);
        }

  return ret;
}


static int ov2686_remove(struct i2c_client *client)
{
  struct v4l2_subdev *sd = i2c_get_clientdata(client);
  struct ov2686_info *info = to_state(sd);

  v4l2_ctrl_handler_free(&info->ctrls);
  v4l2_device_unregister_subdev(sd);
#if defined(CONFIG_MEDIA_CONTROLLER)
  media_entity_cleanup(&sd->entity);
#endif
  kfree(to_state(sd));
  return 0;
}

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

static __init int init_ov2686(void)
{
  return i2c_add_driver(&ov2686_driver);
}

static __exit void exit_ov2686(void)
{
  i2c_del_driver(&ov2686_driver);
}

module_init(init_ov2686);
module_exit(exit_ov2686);

MODULE_AUTHOR("Daniel Casner <daniel@anki.com>");
MODULE_DESCRIPTION("Minimal Driver for OmniVision ov2686 sensor");
MODULE_LICENSE("GPL");
