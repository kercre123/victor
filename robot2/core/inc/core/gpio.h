#ifndef CORE_GPIO_H
#define CORE_GPIO_H

#define LOW 0
#define HIGH 1
#define DIR_OUTPUT 1
#define DIR_INPUT 0

typedef struct GPIO_t {
   int pin;
   int fd;
} GPIO;



/************* GPIO Interface ***************/

void gpio_set_direction(GPIO gp, int isOutput);

void gpio_set_value(GPIO gp, int value);

GPIO gpio_create(int gpio_number, int isOutput, int initial_value);

void gpio_close(GPIO gp);


#endif//CORE_GPIO_H
