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

int init_module(void) {
  printk(KERN_INFO "init_module() called\n");
  return 0;
}

void cleanup_module(void) {
  printk(KERN_INFO "cleanup_module() called\n");
}
