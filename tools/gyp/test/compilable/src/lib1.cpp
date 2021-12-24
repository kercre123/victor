#include "lib1.hpp"

#include <stdio.h>

void lib1_function(void) {
  fprintf(stdout, "Hello from lib1.c\n");
  fflush(stdout);
}
