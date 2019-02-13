/************* GPIO Interface ***************/

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "gpio.h"

typedef enum CoreAppErrorCode_t {
  app_SUCCESS = 0,
  app_USAGE = -1,
  app_FILE_OPEN_ERROR = -2,
  app_FILE_READ_ERROR = -3,
  app_SEND_DATA_ERROR = -4,
  app_INIT_ERROR = -5,
  app_FLASH_ERASE_ERROR = -6,
  app_VALIDATION_ERROR = -7,
  app_FILE_SIZE_ERROR = -8,
  app_MEMORY_ERROR = -9,
  app_IO_ERROR = -10,
  app_DEVICE_OPEN_ERROR = -11,
} CoreAppErrorCode;

#define error_return(code, msg, ...) \
  {                                  \
    va_list args;                    \
    printf("ERROR %d: ", code);      \
    printf(msg, ##__VA_ARGS__);      \
    printf("\n\n");                  \
    return code;                     \
  }

static int GPIO_BASE_OFFSET = -1;

struct GPIO_t
{
  int pin;
  int fd;
  bool isOpenDrain;
};

static int open_patiently(const char *pathname, int flags)
{
  int fd = -1;
  int retries = 10;
  while (retries > 0) {
    retries--;
    fd = open(pathname, flags);
    if (fd < 0 && errno == EACCES) {
      // Sleep 100 milliseconds and try again
      (void) usleep((useconds_t) 100000);
    } else {
      break;
    }
  }
  return fd;
}

int gpio_get_base_offset()
{
  if (GPIO_BASE_OFFSET < 0) {
    // gpio pinctrl for msm8909:
    // oe-linux/3.18: /sys/devices/soc/1000000.pinctrl/gpio/gpiochip0/base     -> 0
    // android/3.10:  /sys/devices/soc.0/1000000.pinctrl/gpio/gpiochip911/base -> 911

    // Assume we are on an OE-linux system
    int fd = open_patiently("/sys/devices/soc/1000000.pinctrl/gpio/gpiochip0/base", O_RDONLY);
    if (fd < 0) {
      // Fallback to Android
      fd = open_patiently("/sys/devices/soc.0/1000000.pinctrl/gpio/gpiochip911/base", O_RDONLY);
    }

    if (fd < 0) {
      error_return(app_DEVICE_OPEN_ERROR, "can't access gpiochip base [%d]", errno);
    }

    char base_buf[5] = {0};
    int r = read(fd, base_buf, sizeof(base_buf));
    if (r < 0) {
      error_return(app_IO_ERROR, "can't read gpiopchip base property");
    }

    if (isdigit(base_buf[0]) == 0)
    {
      error_return(app_VALIDATION_ERROR, "can't parse gpiochip base property");
    }

    GPIO_BASE_OFFSET = atoi(base_buf);
  }

  return GPIO_BASE_OFFSET;
}

int patiently_wait_for_path_to_appear(const char* path) {
  struct stat info = {};
  int retries = 10;
  while (retries > 0) {
    printf("Waiting for %s to appear...\n", path);
    int rc = stat(path, &info);
    if (!rc) {
      return 0;
    }
    // Sleep 100 milliseconds and try again
    (void) usleep((useconds_t) 100000);
    retries--;
  }

  return -1;
}

int gpio_create(int gpio_number, enum Gpio_Dir direction, enum Gpio_Level initial_value, GPIO* gpPtr) {
   char ioname[32];
   GPIO gp  = malloc(sizeof(struct GPIO_t));
   if (!gp) error_return(app_MEMORY_ERROR, "can't alloc memory for gpio %d", gpio_number);
   int fd;

   snprintf(ioname, sizeof(ioname), "/sys/class/gpio/gpio%d", gpio_number+gpio_get_base_offset());
   if (patiently_wait_for_path_to_appear(ioname)) {
     // If the pin hasn't already been exported, try to export it here.  This will fail, if
     // the process does not have permission to write to /sys/class/gpio/export
     // create io
     fd = open_patiently("/sys/class/gpio/export", O_WRONLY);
     snprintf(ioname, 32, "%d\n", gpio_number+gpio_get_base_offset());
     if (fd<0) {
       free(gp);
       *gpPtr = NULL;
       error_return(app_DEVICE_OPEN_ERROR, "Can't create exporter %d- %s\n", errno, strerror(errno));
     }
     (void)write(fd, ioname, strlen(ioname));
     close(fd);
   }


   gp->pin = gpio_number;
   gp->isOpenDrain = false;

   //set direction
   gpio_set_direction(gp, direction);

   //open value fd
   snprintf(ioname, 32, "/sys/class/gpio/gpio%d/value", gpio_number+gpio_get_base_offset());
   fd = open_patiently(ioname, O_WRONLY | O_CREAT );

   if (fd <0) {
     free(gp);
     *gpPtr = NULL;
     error_return(app_IO_ERROR, "can't create gpio %d value control %d - %s", gpio_number, errno, strerror(errno));
   }
   gp->fd = fd;
   if  (fd>0) {
     gpio_set_value(gp, initial_value);
   }
   gp->isOpenDrain = false;

   *gpPtr = gp;
   return 0;
}

static inline enum Gpio_Dir gpio_drain_direction(enum Gpio_Level value) {
  return value == gpio_LOW ? gpio_DIR_OUTPUT : gpio_DIR_INPUT;
}


int gpio_create_open_drain_output(int gpio_number, enum Gpio_Level initial_value, GPIO* gpPtr) {
  enum Gpio_Dir initial_dir = gpio_drain_direction(initial_value);
  int res = gpio_create(gpio_number, initial_dir, gpio_LOW, gpPtr);
  if(res < 0)
  {
    error_return(app_IO_ERROR, "Failed to create gpio %d for open_drain_output %d", gpio_number, res);
  }
  (*gpPtr)->isOpenDrain = true;
  return 0;
}


void gpio_set_direction(GPIO gp, enum Gpio_Dir direction)
{
  assert(gp != NULL);
   char ioname[40];
//   printf("settting direction of %d  to %s\n", gp->pin, direction  ? "out": "in");
   snprintf(ioname, 40, "/sys/class/gpio/gpio%d/direction", gp->pin+gpio_get_base_offset());
   int fd =  open_patiently(ioname, O_WRONLY );
   if (direction == gpio_DIR_OUTPUT) {
      (void)write(fd, "out", 3);
   }
   else {
      (void)write(fd, "in", 2);
   }
   close(fd);
}

int gpio_set_value(GPIO gp, enum Gpio_Level value) {
  assert(gp != NULL);
  if (gp->isOpenDrain) {
    gpio_set_direction(gp, gpio_drain_direction(value));
    return 0;
  }
  static const char* trigger[] = {"0","1"};
  const int bytes = write(gp->fd, trigger[value!=0], 1);
  return bytes;
}

void gpio_close(GPIO gp) {
  assert(gp != NULL);
   if (gp->fd > 0) {
      close(gp->fd);
   }
   free(gp);
}
