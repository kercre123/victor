/************* GPIO Interface ***************/

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>

#include "core/gpio.h"
#include "core/common.h"


#define GPIO_BASE_OFFSET 911

struct GPIO_t
{
  int pin;
  int fd;
  bool isOpenDrain;
};

GPIO gpio_create(int gpio_number, enum Gpio_Dir direction, enum Gpio_Level initial_value) {
   char ioname[32];
   GPIO gp  = malloc(sizeof(struct GPIO_t));
   if (!gp) error_exit(app_MEMORY_ERROR, "can't alloc memory for gpio %d", gpio_number);


   //create io
   int fd = open("/sys/class/gpio/export", O_WRONLY);
   snprintf(ioname, 32, "%d\n", gpio_number+GPIO_BASE_OFFSET);
   if (fd<0) {
     free(gp);
     error_exit(app_DEVICE_OPEN_ERROR, "Can't create exporter %d- %s\n", errno, strerror(errno));
   }
   write(fd, ioname, strlen(ioname));
   close(fd);


   gp->pin = gpio_number;
   gp->isOpenDrain = false;

   //set direction
   gpio_set_direction(gp, direction);

   //open value fd
   snprintf(ioname, 32, "/sys/class/gpio/gpio%d/value", gpio_number+GPIO_BASE_OFFSET);
   fd = open(ioname, O_WRONLY | O_CREAT );

   if (fd <0) {
     free(gp);
     error_exit(app_IO_ERROR, "can't create gpio %d value control %d - %s", gpio_number, errno, strerror(errno));
   }
   gp->fd = fd;
   if  (fd>0) {
     gpio_set_value(gp, initial_value);
   }
   gp->isOpenDrain = false;
   return gp;
}

static inline enum Gpio_Dir gpio_drain_direction(enum Gpio_Level value) {
  return value == gpio_LOW ? gpio_DIR_OUTPUT : gpio_DIR_INPUT;
}


GPIO gpio_create_open_drain_output(int gpio_number, enum Gpio_Level initial_value) {
  enum Gpio_Dir initial_dir = gpio_drain_direction(initial_value);
  GPIO gp = gpio_create(gpio_number, initial_dir, gpio_LOW);
  gp->isOpenDrain = true;
  return gp;

}


void gpio_set_direction(GPIO gp, enum Gpio_Dir direction)
{
  assert(gp != NULL);
   char ioname[40];
//   printf("settting direction of %d  to %s\n", gp->pin, direction  ? "out": "in");
   snprintf(ioname, 40, "/sys/class/gpio/gpio%d/direction", gp->pin+GPIO_BASE_OFFSET);
   int fd =  open(ioname, O_WRONLY );
   if (direction == gpio_DIR_OUTPUT) {
      write(fd, "out", 3);
   }
   else {
      write(fd, "in", 2);
   }
   close(fd);
}

void gpio_set_value(GPIO gp, enum Gpio_Level value) {
  assert(gp != NULL);
  if (gp->isOpenDrain) {
    gpio_set_direction(gp, gpio_drain_direction(value));
    return;
  }
  static const char* trigger[] = {"0","1"};
  write(gp->fd, trigger[value!=0], 1);
}

void gpio_close(GPIO gp) {
  assert(gp != NULL);
   if (gp->fd > 0) {
      close(gp->fd);
   }
   free(gp);
}
