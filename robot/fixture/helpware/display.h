#ifndef VICTOR_DISPLAY_H_
#define VICTOR_DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>

void display_init(void);

void display_draw_text(int layer, int line , uint16_t fg, uint16_t bg, const char* text, int len, bool centered);

void display_clear_layer(int layer, int fg, int bg);


void display_render(uint8_t layermask);


const char* parse_color(const char* cp, const char* endp, uint16_t* colorOut);


#define DISPLAY_LAYER_TINY 0
#define DISPLAY_LAYER_SMALL 1
#define DISPLAY_LAYER_MEDIUM 2
#define DISPLAY_LAYER_LARGE 3
#define DISPLAY_LAYER_LARGE_SKINNY 4

#define DISPLAY_NUM_LAYERS 5
#define DISPLAY_MASK_ALL ((1<<DISPLAY_NUM_LAYERS)-1)





#endif//VICTOR_DISPLAY_H_
