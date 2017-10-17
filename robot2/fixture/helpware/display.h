#ifndef VICTOR_DISPLAY_H_
#define VICTOR_DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>

void display_init(void);

void display_draw_text(int layer, int line , uint16_t fg, uint16_t bg, const char* text, int len, bool centered);

void display_clear_layer(int layer, int fg, int bg);


void display_render(uint8_t layermask);


const char* parse_color(const char* cp, const char* endp, uint16_t* colorOut);


#endif//VICTOR_DISPLAY_H_
