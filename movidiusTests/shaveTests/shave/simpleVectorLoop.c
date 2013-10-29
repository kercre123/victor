#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveShared.h"

void simpleVectorLoop() {
  int i;

  int4 * restrict v_input1 = (int4*) &(input1[0]);
  int4 * restrict v_input2 = (int4*) &(input2[0]);
  int4 * restrict v_output = (int4*) &(output[0]);

  for(i=0; i<(DATA_LENGTH>>2); i++) {
    v_output[i] = v_input1[i] + v_input2[i];
  }
}