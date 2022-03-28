#ifndef CORE_LCD_H_
#define CORE_LCD_H_

#include <stddef.h>
#include <stdint.h>


#define LCD_FRAME_WIDTH_SANTEK    184
#define LCD_FRAME_HEIGHT_SANTEK   96
#define LCD_FRAME_WIDTH_MIDAS     160
#define LCD_FRAME_HEIGHT_MIDAS    80

#define PXL_CNT_SANTEK  (LCD_FRAME_WIDTH_SANTEK * LCD_FRAME_HEIGHT_SANTEK)
#define PXL_CNT_MIDAS   (LCD_FRAME_WIDTH_SANTEK * LCD_FRAME_HEIGHT_MIDAS)
#define OLP_CNT  (OLD_FRAME_WIDTH * OLD_FRAME_HEIGHT)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LcdFrame_t {
  uint16_t data[LCD_FRAME_WIDTH_SANTEK*LCD_FRAME_HEIGHT_SANTEK];
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

typedef enum {
  SANTEK,
  MIDAS
} lcd_display_t;

lcd_display_t lcd_display_version();
bool lcd_use_midas_crop();
int lcd_init(void);
void lcd_clear_screen(void);
void lcd_draw_frame(const LcdFrame* frame);
void lcd_draw_frame2(const uint16_t* frame, size_t size);
int lcd_set_brightness(int b); //0..20
void lcd_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif//CORE_LCD_H_
