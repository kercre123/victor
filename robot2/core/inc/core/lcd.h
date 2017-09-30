#ifndef CORE_LCD_H_
#define CORE_LCD_H_

#define LCD_FRAME_WIDTH     184
#define LCD_FRAME_HEIGHT    96


int lcd_init(void);
void lcd_draw_frame(uint8_t* frame, int sz);
void lcd_shutdown(void);

#endif//CORE_LCD_H_
