/************* GPIO Interface ***************/

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

#include "core/gpio.h"
#include "core/common.h"

#define GPIO_BASE_OFFSET 911

struct GPIO_t
{
  int pin;
  int fd;
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
   return gp;
}


void gpio_set_direction(GPIO gp, enum Gpio_Dir direction)
{
  assert(gp != NULL);
   char ioname[40];
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
