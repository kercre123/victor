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

/*** Type definitions */

struct ov2686_info {
  struct v4l2_subdev subdev;

  int ident;
};

/*** Static data */

static struct v4l2_subdev_ops ov2686_subdev_ops = {
  .core	 = NULL, // TODO Make this not NULL
  .video = NULL, // TODO Make this not NULL
};

/*** Functions */


static int ov2686_probe(struct i2c_client *client, const struct i2c_device_id *device) {
  struct ov2686_info *info;

  printk(KERN_INFO "ov2686 i2c probe\n");

  // Do some initial i2c camera communication here if nessisary

  info = kzalloc(sizeof(struct ov2686_info), GFP_KERNEL);
  if (info == NULL) {
    printk(KERN_WARNING "Failed to allocate driver memory\n");
    return -ENOMEM;
  }

  v4l2_i2c_subdev_init(&info->subdev, client, &ov2686_subdev_ops);

  // Do more setup here

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
