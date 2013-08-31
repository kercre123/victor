
#include <stdio.h>

#include "testCpp.h"

void RUN_ALL_TESTS();

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

void runTests()
{
  printf("dogcat\n");
  RUN_ALL_TESTS();
}
