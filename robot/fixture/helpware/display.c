#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "core/common.h"
#include "core/lcd.h"

#include "helpware/display.h"


#ifdef EXTENDED_DISPLAY_DEBUGGING
#define ddprintf printf
#else
#define ddprintf(f,...)
#endif


#define DISPLAY_SCREEN_WIDTH LCD_FRAME_WIDTH
#define DISPLAY_SCREEN_HEIGHT LCD_FRAME_HEIGHT

typedef struct Font_t
{
  int LineCount;
  int CharsPerLine;
  int BitHeight;
  int BitWidth;


  unsigned char CharStart;
  unsigned char CharEnd;

  bool CenteredByDefault;
  bool HasVSpace;

  const void* glyphs;


} Font;

static const uint8_t gSmallFontGlyph[] = {
     0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14,
     0x3E,0x7F,0x63,0x7F,0x3E, 0x23,0x13,0x08,0x64,0x62, 0x0C,0x3C,0x3C,0x3C,0x0C, 0x00,0x08,0x07,0x03,0x00,

     0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00, 0x2A,0x1C,0x7F,0x1C,0x2A, 0x08,0x08,0x3E,0x08,0x08,
     0x00,0x80,0x70,0x30,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x00,0x60,0x60,0x00, 0x20,0x10,0x08,0x04,0x02,
     0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x72,0x49,0x49,0x49,0x46, 0x21,0x41,0x49,0x4D,0x33,
     0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x31, 0x41,0x21,0x11,0x09,0x07,
     0x36,0x49,0x49,0x49,0x36, 0x46,0x49,0x49,0x29,0x1E, 0x00,0x00,0x14,0x00,0x00, 0x00,0x40,0x34,0x00,0x00,
     0x00,0x08,0x14,0x22,0x41, 0x14,0x14,0x14,0x14,0x14, 0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x59,0x09,0x06,
     0x3E,0x41,0x5D,0x59,0x4E, 0x7C,0x12,0x11,0x12,0x7C, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
     0x7F,0x41,0x41,0x41,0x3E, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x41,0x51,0x73,
     0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41,
     0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x1C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
     0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x26,0x49,0x49,0x49,0x32,
     0x03,0x01,0x7F,0x01,0x03, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F,
     0x63,0x14,0x08,0x14,0x63, 0x03,0x04,0x78,0x04,0x03, 0x61,0x59,0x49,0x4D,0x43, 0x00,0x7F,0x41,0x41,0x41,
     0x02,0x04,0x08,0x10,0x20, 0x00,0x41,0x41,0x41,0x7F, 0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
     0x00,0x03,0x07,0x08,0x00, 0x20,0x54,0x54,0x78,0x40, 0x7F,0x28,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x28,
     0x38,0x44,0x44,0x28,0x7F, 0x38,0x54,0x54,0x54,0x18, 0x00,0x08,0x7E,0x09,0x02, 0x18,0xA4,0xA4,0x9C,0x78,
     0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x40,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00,
     0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x78,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38,
     0xFC,0x18,0x24,0x24,0x18, 0x18,0x24,0x24,0x18,0xFC, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x24,
     0x04,0x04,0x3F,0x44,0x24, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C,
     0x44,0x28,0x10,0x28,0x44, 0x4C,0x90,0x90,0x90,0x7C, 0x44,0x64,0x54,0x4C,0x44, 0x00,0x08,0x36,0x41,0x00,
     0x00,0x00,0x77,0x00,0x00, 0x00,0x41,0x36,0x08,0x00, 0x02,0x01,0x02,0x04,0x02, 0x3C,0x26,0x23,0x26,0x3C
  };


#define SMALL_FONT_WIDTH 5
#define SMALL_FONT_HEIGHT 8
#define SMALL_FONT_CHARS_PER_LINE (LCD_FRAME_WIDTH/(SMALL_FONT_WIDTH+1))
#define SMALL_FONT_LINE_COUNT (LCD_FRAME_HEIGHT/SMALL_FONT_HEIGHT)

#define SMALL_FONT_CHAR_START 32
#define SMALL_FONT_CHAR_END  127
#define SMALL_FONT_UNPRINTABLE (127-SMALL_FONT_CHAR_START)

static const Font gSmallFont = {
  /* int LineCount; */       DISPLAY_SCREEN_HEIGHT/SMALL_FONT_HEIGHT,
  /* int CharsPerLine; */    DISPLAY_SCREEN_WIDTH/SMALL_FONT_WIDTH,
  /* int BitHeight; */       SMALL_FONT_HEIGHT,
  /* int BitWidth; */        SMALL_FONT_WIDTH,
  SMALL_FONT_CHAR_START,
  SMALL_FONT_CHAR_END,
  1, /* CenteredByDefault& */
  0,
  gSmallFontGlyph,
};


#include "medium_font.inc"
#include "big_font.inc"
#include "large_font.inc"
#include "huge_font.inc"


static const Font* gFont[DISPLAY_NUM_LAYERS] = {
  &gSmallFont,
  &gMediumFont,
  &gBigFont,
  &gHugeFont,  //DISPLAY_LAYER_LARGE uses original "Huge" font
  &gLargeFont, //DISPLAY_LAYER_LARGE_SKINNY uses "Large"
};

//each screen_line is 8 bits high.
//All fonts must be in units of half lines
#define SCAN_LINE_COUNT (LCD_FRAME_HEIGHT/(CHAR_BIT/2))
/****************/

//each layer has N lines of text.
// each line of text has content, fg and bg colors. Content can be zero.
//each layer can be active inactive or exclusive. Exclusive inactivates all the others.
//higher layers hide lower ones.
///. so .  Draw the bitmap from high to low.
// fill in the colors as you go.



typedef struct TextProperties_t {
  uint16_t fg;
  uint16_t bg;
  bool active;
} TextProperties;


struct {
  char*text[DISPLAY_NUM_LAYERS];
  LcdFrame frame;
  uint8_t active_layers;
  TextProperties layer_prop[DISPLAY_NUM_LAYERS];
  TextProperties map_prop[SCAN_LINE_COUNT];
} gDisplay;

char BLANK_LINE[]={" "};

void display_init(void) {
  int i;
  memset(&gDisplay, 0, sizeof(gDisplay));
  for (i=0;i<DISPLAY_NUM_LAYERS;i++)
  {
    gDisplay.text[i]=malloc(gFont[i]->LineCount * gFont[i]->CharsPerLine * sizeof(char));
  }
}


static inline int min(int a, int b){  return a<b?a:b;}

void display_set_colors(int layer, uint16_t fg, uint16_t bg) {
  gDisplay.layer_prop[layer].fg = fg;
  gDisplay.layer_prop[layer].bg = bg;
}




static inline unsigned char restrict_to_printable(unsigned char c, unsigned char font_start, unsigned char font_end) {
  c -= font_start;
  return min(c, (font_end-font_start)+1);
}

static inline void set_map_properties(int scanline, int ct, TextProperties* tp) {
  int i;
  ddprintf("setting scanlines %d..%d to %x/%x\n",scanline, scanline+ct, tp->fg, tp->bg);

  for (i=scanline;i<scanline+ct;i++) {
    gDisplay.map_prop[i]=*tp;
  }
}


void display_set_colors2(int layer, int line, uint16_t fg, uint16_t bg) {
  const Font* font = gFont[layer];
  int scanline = line*font->BitHeight/4;
  gDisplay.layer_prop[layer].fg = fg;
  gDisplay.layer_prop[layer].bg = bg;

  set_map_properties(scanline, font->BitHeight/4, &gDisplay.layer_prop[layer]);
}



void display_render_8bit_text(uint8_t* bitmap, int layer, const Font* font)
{
  //bitmap is 12 rows of uint8's, FRAME_WIDTH pixels wide.
  // small text puts 1 lines in each row
  int line,col;

  int mapline = 0;
  for (line=0;line<font->LineCount;line++)
  {
    uint8_t* map = &bitmap[mapline*LCD_FRAME_WIDTH];
    const char* text = gDisplay.text[layer]+(line*font->CharsPerLine);
    if (*text)  {
      ddprintf(": %s\n", text);
      for (col=0;col<font->CharsPerLine; col++)
      {
        unsigned char c = restrict_to_printable(*text++, font->CharStart, font->CharEnd);

        const uint8_t* glyph = ((uint8_t*)font->glyphs) + (c * font->BitWidth);
        int i;
        for (i=0; i<font->BitWidth; i++)
        {
          *map++ |= (*glyph++);
        }
        if (!font->HasVSpace) {
          *map++ = 0; //char separator
        }
      }
    }
    mapline++;
  }
}


void display_render_16bit_text(uint8_t* bitmap, int layer, const Font* font)
{
  //bitmap is 12 rows of uint8's, FRAME_WIDTH pixels wide.
  // 12 bit text puts 1  lines in each 1.5 rows
  // 16 bit text puts 1  lines in each 2 rows
  int line, col;
  int mapline = 0;
  int shift = 0;

  for (line=0;line<font->LineCount;line++)
  {
    uint8_t* m1 = &bitmap[mapline*LCD_FRAME_WIDTH];
    uint8_t* m2 = &bitmap[(mapline+1)*LCD_FRAME_WIDTH];
    const char* text = gDisplay.text[layer]+(line*font->CharsPerLine);
    if (*text)  {
      ddprintf("< %s\n", text);
      for (col=0;col<font->CharsPerLine; col++)
      {
        unsigned char c = restrict_to_printable(*text++, font->CharStart, font->CharEnd);

        const uint16_t* glyph = ((uint16_t*)font->glyphs) + (c * font->BitWidth);
//        const uint16_t* glyph = &Font->glyphs[c * font->BitWidth];
        int i;
        for (i=0; i<font->BitWidth; i++)

        {
          uint16_t gg =  (*glyph++)<<shift;
          *m1++ |= (gg&0xFF);
          *m2++ |= (gg>>8);
        }
        if (!font->HasVSpace) {
          *m1++ = 0; //char separator
          *m2++ = 0; //char separator
        }
      }
    }
    shift ^= ( 16 - font->BitHeight); //only works for 16 or 12
    mapline+= (shift)?1:2;
  }
}



void display_render_32bit_text(uint8_t* bitmap, int layer, const Font* font)
{
  //bitmap is 12 rows of uint8's, FRAME_WIDTH pixels wide.
  // 32 bit text puts 1 line over 4 rows.
  // theoretically we could do 24 bit text...
  int line, col;
  int mapline = 0;
  for (line=0;line<font->LineCount;line++)
  {
    uint8_t* m1 = &bitmap[mapline*LCD_FRAME_WIDTH];
    uint8_t* m2 = &bitmap[(mapline+1)*LCD_FRAME_WIDTH];
    uint8_t* m3 = &bitmap[(mapline+2)*LCD_FRAME_WIDTH];
    uint8_t* m4 = &bitmap[(mapline+3)*LCD_FRAME_WIDTH];

    const char* text = gDisplay.text[layer]+(line*font->CharsPerLine);
    if (*text)  {
      ddprintf("^ %s\n", text);
      for (col=0;col<font->CharsPerLine; col++)
      {
        unsigned char c = restrict_to_printable(*text++, font->CharStart, font->CharEnd);

        const uint32_t* glyph = ((uint32_t*)font->glyphs) + (c * font->BitWidth);
        int i;
        for (i=0; i<font->BitWidth; i++)

        {
          uint32_t gg =  (*glyph++);
          *m1++ |= (gg&0xFF);
          *m2++ |= ((gg>>8)&0xFF);
          *m3++ |= ((gg>>16)&0xFF);
          *m4++ |= (uint8_t)(gg>>24);

        }
        if (!font->HasVSpace) {
          *m1++ = 0; //char separator
          *m2++ = 0; //char separator
          *m3++ = 0; //char separator
          *m4++ = 0; //char separator
        }
      }
    }
    mapline+= 4;
  }
}

void bitmap_render_text(uint8_t* bitmap, int layer, const Font* font)
{
  switch(font->BitHeight) {
    case 8:
      display_render_8bit_text(bitmap, layer, font);
      break;
    case 12:
    case 16:
      display_render_16bit_text(bitmap, layer, font);
      break;
    case 32:
      display_render_32bit_text(bitmap, layer, font);
      break;
    default:
      printf("FONT HEIGHT %d NOT SUPPORTED\n", font->BitHeight);
      break;
  }
}


#define EXTRACT_BIT(map,x,y) \
  (((map)[(x)+((y)/8)*LCD_FRAME_WIDTH]>>((y)%8))&1)


void display_render(uint8_t layermask) {
  //two stage process
  //first render the fonts into a bitmap.
  uint8_t bitmap[LCD_FRAME_WIDTH * LCD_FRAME_HEIGHT/8]={0};
  int i;

  for (i=0;i<DISPLAY_NUM_LAYERS;i++) {
    if (layermask & (1<<i) ) {
    ddprintf("rendering layer %d\n", i);
      bitmap_render_text(bitmap, i, gFont[i]);
    }
  }
  //second, convert the bitmap to color using the textprop values.
  int x,y;
  for (y=0;y<LCD_FRAME_HEIGHT;y++) {
    const TextProperties* tp = &gDisplay.map_prop[y/4];
    ddprintf("%02d %04x/%04x: ",y, tp->fg, tp->bg);
    for (x=0;x<LCD_FRAME_WIDTH;x++) {
      int isSet = EXTRACT_BIT(bitmap,x,y);
      uint16_t color = (isSet) ? tp->fg : tp->bg;
      gDisplay.frame.data[y*LCD_FRAME_WIDTH+x] = color;
      ddprintf("%c", isSet?'X':'.');
    }
    ddprintf("\n");
  }
  lcd_draw_frame(&gDisplay.frame);
}


void display_draw_text(int layer, int line , uint16_t fg, uint16_t bg, const char* text, int len, bool centered)
{
  if( layer == DISPLAY_LAYER_LARGE && len > gFont[layer]->CharsPerLine ) {
    layer = DISPLAY_LAYER_LARGE_SKINNY;
  }
  const Font* font =  gFont[layer];
  assert(line < font->LineCount);
  int nchars =  min(len, font->CharsPerLine);
  char* textline = gDisplay.text[layer]+(line*font->CharsPerLine);
  memset(textline, ' ', font->CharsPerLine);
  if (nchars == 0) {
    *textline = 0;
  }
  else  {
    int leftpad = centered? (font->CharsPerLine + 1 - nchars)/2 : 0;
    strncpy(textline+leftpad, text, nchars);
  }
  ddprintf("Drawing text %8s\n", textline);
  display_set_colors(layer, fg, bg);
  display_set_colors2(layer, line, fg, bg);

}

void display_clear_layer(int layer, int fg, int bg) {
  int i;
  for (i=0; i<gFont[layer]->LineCount; i++)
  {
    display_draw_text(layer, i, fg, bg,
                      BLANK_LINE, 1, 0);
  }
}


static inline int hextoint(char digit)
{
  assert(isxdigit(digit));
  if (digit <= '9') return digit-'0';
  if (digit <= 'F') return digit-'A'+10;
  return digit-'a'+10;
}


const char* parse_color(const char* cp, const char* endp, uint16_t* colorOut)
{
  if (cp<endp) {
    char colorchar=*cp;
    switch (colorchar) {
      case 'r':
        *colorOut = lcd_RED;
        break;
      case 'g':
        *colorOut = lcd_GREEN;
        break;
      case 'b':
        *colorOut = lcd_BLUE;
        break;
      case 'w':
        *colorOut = lcd_WHITE;
        break;
      case 'x':
        cp++;
        *colorOut = 0;
        while (cp<endp && isxdigit(*cp)) {
          *colorOut = (*colorOut<<4)+hextoint(*cp++);
        }
        break;
      default:
        *colorOut = lcd_WHITE;
        return cp; //don't advance
        break;
    }
  }
  //ignore any other characters
  while(cp<endp && !isspace(*cp)) {cp++;}
  return cp;
}
