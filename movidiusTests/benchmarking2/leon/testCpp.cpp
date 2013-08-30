
#include <stdio.h>

#include "testCpp.h"

class Toast
{
public:
int get_Toast(int a) { return a*a*a; }

};

int helloDog(int a)
{
  Toast t;

  //printf("Here\n");
  return t.get_Toast(a);
}
