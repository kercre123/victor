#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveShared.h"

void intrinsicsLoop() {
  int i;

  int4 * restrict v_input1 = (int4*) &(input1[0]);
  int4 * restrict v_input2 = (int4*) &(input2[0]);
  int4 * restrict v_output = (int4*) &(output[0]);

  for(i=0; i<(DATA_LENGTH>>2); i++) {
    const int4 v1 = v_input1[i];
    const int4 v2 = v_input2[i];

    const int4 out = __builtin_shave_vau_add_i32_rr(v1, v2);

    v_output[i] = out;
  }
}