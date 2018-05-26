#ifndef CORE_LCD_H_
#define CORE_LCD_H_

#include <stdint.h>

#define LCD_FRAME_WIDTH     184
#define LCD_FRAME_HEIGHT    96

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LcdFrame_t {
  uint16_t data[LCD_FRAME_WIDTH*LCD_FRAME_HEIGHT];
} LcdFrame;

enum LcdColor {
  lcd_BLACK = 0x0000,
  lcd_BLUE    = 0x001F,
  lcd_GREEN   = 0x07E0,
  lcd_RED     = 0xF800,
  lcd_WHITE = 0xFFFF,
};

int lcd_init(void);
void lcd_clear_screen(void);
void lcd_draw_frame(const LcdFrame* frame);
void lcd_draw_frame2(const uint16_t* frame, size_t size);
void lcd_set_brightness(int b); //0..20
void lcd_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif//CORE_LCD_H_
