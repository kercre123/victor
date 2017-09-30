/************* GPIO Interface ***************/

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "core/gpio.h"

GPIO gpio_create(int gpio_number, int isOutput, int initial_value) {
   char ioname[32];
   GPIO gp = {gpio_number, 0};

   //create io
   int fd = open("/sys/class/gpio/export", O_WRONLY);
   snprintf(ioname, 32, "%d", gpio_number+911);
   write(fd, ioname, strlen(ioname));
   close(fd);

   //set direction
   gpio_set_direction(gp, isOutput);

   //open value fd
   snprintf(ioname, 32, "/sys/class/gpio/gpio%d/value", gpio_number+911);
   gp.fd = open(ioname, O_WRONLY );

   if (gp.fd > 0) {
      gpio_set_value(gp, initial_value);
   }
   return gp;
}

void gpio_set_direction(GPIO gp, int isOutput)
{
   char ioname[40];
   snprintf(ioname, 40, "/sys/class/gpio/gpio%d/direction", gp.pin+911);
   int fd =  open(ioname, O_WRONLY );
   if (isOutput) {
      write(fd, "out", 3);
   }
   else {
      write(fd, "in", 2);
   }
   close(fd);
}

void gpio_set_value(GPIO gp, int value) {
   static const char* trigger[] = {"0","1"};
   write(gp.fd, trigger[value!=0], 1);
}

void gpio_close(GPIO gp) {
   if (gp.fd > 0) {
      close(gp.fd);
   }
}
