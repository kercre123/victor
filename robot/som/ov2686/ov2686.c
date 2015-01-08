#include <linux/module.h>
#include <linux/kernel.h>

int init_module(void) {
  printk(KERN_INFO "init_module() called\n");
  return 0;
}

void cleanup_module(void) {
  printk(KERN_INFO "cleanup_module() called\n");
}

