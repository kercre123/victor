#ifndef CORE_LCD_H_
#define CORE_LCD_H_

#include <stdint.h>


#define OLD_FRAME_WIDTH     184
#define OLD_FRAME_HEIGHT    96
#define LCD_FRAME_WIDTH     160
#define LCD_FRAME_HEIGHT    80

#ifdef LCD_FRAME
#if LCD_FRAME == MIDAS
#undef LCD_FRAME
#define LCD_FRAME 3022
#endif
#if LCD_FRAME == 77890
#undef LCD_FRAME_WIDTH
#undef LCD_FRAME_HEIGHT
#define LCD_FRAME_WIDTH     184
#define LCD_FRAME_HEIGHT    96
#define INIT_PROG st77890_init_scr
#endif
#if LCD_FRAME == 7789
#undef LCD_FRAME_WIDTH
#undef LCD_FRAME_HEIGHT
#define LCD_FRAME_WIDTH     240
#define LCD_FRAME_HEIGHT    135
#define INIT_PROG st7789_init_scr
#endif
#if LCD_FRAME == 7735
#undef LCD_FRAME_WIDTH
#undef LCD_FRAME_HEIGHT
#define LCD_FRAME_WIDTH     160
#define LCD_FRAME_HEIGHT    80
#define INIT_PROG st7735_init_scr
#endif
#if LCD_FRAME == 3022
#undef LCD_FRAME_WIDTH
#undef LCD_FRAME_HEIGHT
#define LCD_FRAME_WIDTH     160
#define LCD_FRAME_HEIGHT    80
#define INIT_PROG nv3022_init_scr
#endif
#endif

#define PXL_CNT  (LCD_FRAME_WIDTH * LCD_FRAME_HEIGHT)
#define OLP_CNT  (OLD_FRAME_WIDTH * OLD_FRAME_HEIGHT)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LcdFrame_t {
  uint16_t data[LCD_FRAME_WIDTH*LCD_FRAME_HEIGHT];
} LcdFrame;

enum LcdColor {
  lcd_BLACK   = 0x0000,
  lcd_BLUE    = 0x001F,
  lcd_GREEN   = 0x07E0,
  lcd_CYAN    = 0x7FFF,
  lcd_GRAY    = 0x8430,
  lcd_RED     = 0xF800,
  lcd_MAGENTA = 0xF81F,
  lcd_YELLOW  = 0xFFE0,
  lcd_WHITE   = 0xFFFF,
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
