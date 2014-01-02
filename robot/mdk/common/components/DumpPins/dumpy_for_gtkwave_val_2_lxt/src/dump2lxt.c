#include "lxt_write.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

uint32_t g(FILE *f) {
    uint8_t arr[4];
    if ( 1!= fread(arr, 4, 1, f)) return 0;;
    return arr[0] | (arr[1] << 8) | (arr[2] << 16) | (arr[3] << 24);
}

uint64_t g64(FILE *f) {
    uint64_t tmp = g(f);
    tmp |= ((uint64_t)(g(f))) << 32;
    return tmp;
}

int main(int argc, char ** argv) {
    if (argc != 3) {
        printf("Usage: %s input.val output.lxt\n", argv[0]);
        return -1;
    }

    struct lt_trace *lt = lt_init(argv[2]);

    lt_set_timescale(lt, 6);

    FILE *f = fopen(argv[1], "rb");

    int N = g(f);
    int gpios[200];
    struct lt_symbol *lt_gpios[200];

    int i;
    for (i=0;i<N;i++) {
        gpios[i] = g(f);

        char s[100] = "";

        sprintf(s, "gpio%d", gpios[i]);

        lt_gpios[i] = lt_symbol_add(lt, s, 0, 0, 0, 0);
    }
    
    lt_set_time(lt, 0);
    
    uint64_t prevTime = 0;
    while (!feof(f)) {
        uint64_t time = g64(f);
        uint32_t val = g(f);
        
        if (time == 0)
            continue;
        
        printf("%lld, %d\n", time, val);
        lt_set_time64(lt, time);
        for (i=N-1;i>=0;i--) {
            if (val & 1) {
                lt_emit_value_bit_string(lt, lt_gpios[i], 0,  "1");
            } else {
                lt_emit_value_bit_string(lt, lt_gpios[i], 0,  "0");
            }
            val >>= 1;
        }
    }
    
    fclose(f);

    lt_close(lt);
    return 0;
}