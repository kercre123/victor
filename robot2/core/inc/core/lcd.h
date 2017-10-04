#ifndef CORE_LCD_H_
#define CORE_LCD_H_

#define LCD_FRAME_WIDTH     184
#define LCD_FRAME_HEIGHT    96

typedef struct LcdFrame_t {
  uint16_t data[LCD_FRAME_WIDTH*LCD_FRAME_HEIGHT];
} LcdFrame;

enum LcdColor {
  lcd_BLACK = 0x0000,
  lcd_BLUE  = 0x001F,
  lcd_GREEN = 0x07E0,
  lcd_RED   = 0xF800,
  lcd_WHITE = 0xFFFF,
};


int lcd_init(void);
void lcd_draw_frame(LcdFrame* frame);
void lcd_shutdown(void);

#endif//CORE_LCD_H_
