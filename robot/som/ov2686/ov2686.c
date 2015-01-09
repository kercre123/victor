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

/*** Type definitions */

struct ov2686_info {
  struct v4l2_subdev subdev;
  struct ov2686_platform_data *pdata;
  struct mutex power_lock;
  int power_count;
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
  return ret;
}

/*** Static data */

static const struct v4l2_subdev_core_ops ov2686_subdev_core_ops = {
  .s_power      = ov2686_s_power,
};

/*static const struct v4l2_subdev_video_ops ov2686_subdev_video_ops = {
  .s_stream       = ov2686_s_stream, TODO
};*/

static struct v4l2_subdev_ops ov2686_subdev_ops = {
  .core	 = &ov2686_subdev_core_ops,
  .video = NULL, // TODO &ov2686_subdev_video_ops,
};

/*** I2C module functions */

static int ov2686_probe(struct i2c_client *client, const struct i2c_device_id *device) {
  struct ov2686_info *info;
  struct ov2686_platform_data *pdata;
  struct i2c_adapter *adapter;

  printk(KERN_INFO "ov2686 i2c probe\n");

  pdata = client->dev.platform_data;
  if (pdata == NULL) {
    printk(KERN_WARNING "No platform data\n");
    return -EINVAL;
  }

  printk(KERN_INFO "Getting Adapter info\n");

  adapter = to_i2c_adapter(client->dev.parent);

  if ( !i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
    printk(KERN_WARNING "I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE_DATA\n");
    return -EIO;
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

  // Do more setup here
  printk(KERN_INFO "i2c probe complete\n");

  return 0;
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
