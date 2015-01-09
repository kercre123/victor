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

/*** Register definitions */

static const u16 OV2686_REG_CHIP_ID_L = 0x300A; static const u8 OV2686_CHIP_ID_L_VALUE = 0x26;
static const u16 OV2686_REG_CHIP_ID_M = 0x300B; static const u8 OV2686_CHIP_ID_M_VALUE = 0x85;
static const u16 OV2686_REG_CHIP_ID_H = 0x300C; static const u8 OV2686_CHIP_ID_H_VALUE = 0x00;


/*** Type definitions */

struct ov2686_info {
  struct v4l2_subdev subdev;
  struct media_pad pad;
  struct ov2686_platform_data *pdata;
  struct mutex power_lock;
  int power_count;
  int streaming;
  int ident;
};

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
  }
  else {
    if (info->pdata->s_xclk) info->pdata->s_xclk(&info->subdev, 1);
    msleep(1);
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
  if (ret < 0) {
    printk(KERN_WARNING "ov2686 failed reading register 0x%04x!\n", reg);
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
  struct ov2686_info * info = to_state(subdev);
  int ret = 0;

  printk(KERN_INFO "ov2686_s_stream %d\n", enable);

  if (info->power_count == 0) {
    printk(KERN_INFO "\tpower count (%d) invalid\n", info->power_count);
    return -EINVAL;
  }
  if (enable == 0) {
    // TODO Do something to disable streaming
    info->streaming = 0;
  }
  else {
    // TODO Do something to enable streaming
    info->streaming = 1;
  }
  return ret;
}

static int ov2686_enum_mbus_code(struct v4l2_subdev *subdev,
                                 struct v4l2_subdev_fh *fh,
                                 struct v4l2_subdev_mbus_code_enum *code) {
  printk(KERN_INFO "ov2686_enum_mbus_code\n");

  // TODO populate code structure
  return 0;
}

static int ov2686_enum_frame_size(struct v4l2_subdev *subdev,
                                  struct v4l2_subdev_fh *fh,
                                  struct v4l2_subdev_frame_size_enum *fse) {
  printk(KERN_INFO "ov2686_enum_frame_size\n");

  // TODO populate fse structure
  return 0;
}

static int ov2686_get_format(struct v4l2_subdev *subdev,
                             struct v4l2_subdev_fh *fh,
                             struct v4l2_subdev_format *fmt) {
  printk(KERN_INFO "ov2686_get_format\n");

  // TODO populate fmt structure;
  return 0;
}

static int ov2686_set_format(struct v4l2_subdev *subdev,
                             struct v4l2_subdev_fh *fh,
                             struct v4l2_subdev_format *fmt) {
  printk(KERN_INFO "ov2686_set_format\n");

  // TODO execute format set
  // TODO populate fmt structure
  return 0;
}

static int ov2686_get_crop(struct v4l2_subdev *subdev,
                           struct v4l2_subdev_fh *fh,
                           struct v4l2_subdev_crop *crop) {
  printk(KERN_INFO "ov2686_get_crop\n");

  // TODO populate crop strucuture.
  return 0;
}

static int ov2686_set_crop(struct v4l2_subdev *subdev,
                           struct v4l2_subdev_fh *fh,
                           struct v4l2_subdev_crop *crop) {
  printk(KERN_INFO "ov2686_set_crop\n");

  // TODO change crop
  // TODO populate crop strucutre

  return 0;
}

static int ov2686_registered(struct v4l2_subdev *subdev) {
  struct i2c_client *client;
  struct ov2686_info *info;
  u8 data;
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
    printk(KERN_INFO "ov2686 not detected at address 0x02x\n", client->addr);
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
  .s_stream       = ov2686_s_stream,
};

static const struct v4l2_subdev_pad_ops ov2686_subdev_pad_ops = {
  .enum_mbus_code  = ov2686_enum_mbus_code,
  .enum_frame_size = ov2686_enum_frame_size,
  .get_fmt         = ov2686_get_format,
  .set_fmt         = ov2686_set_format,
  .get_crop        = ov2686_get_crop,
  .set_crop        = ov2686_set_crop,
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

  v4l2_i2c_subdev_init(&info->subdev, client, &ov2686_subdev_ops);
  info->subdev.internal_ops = &ov2686_subdev_internal_ops;

  printk(KERN_INFO "Initalizing media controller\n");

  info->pad.flags = MEDIA_PAD_FL_SOURCE;
  ret = media_entity_init(&info->subdev.entity, 1, &info->pad, 0);
  if (ret < 0) {
    printk(KERN_WARNING "ov2686 media_entity_init failed: %d\n", ret);
    return ret;
  }

  // Do more setup here
  printk(KERN_INFO "i2c probe complete\n");

  return ret;
}

static int ov2686_remove(struct i2c_client *client)
{
  struct ov2686_info *info = i2c_get_clientdata(client);

  printk(KERN_INFO "ov2686 i2c remove\n");

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
  printk(KERN_INFO "init_ov2686() called\n");
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
