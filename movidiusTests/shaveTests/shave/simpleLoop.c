#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveShared.h"

void simpleLoop() {
  int i;

  for(i=0; i<DATA_LENGTH; i++) {
    output[i] = input1[i] + input2[i];
  }
}