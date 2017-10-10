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
  1 /* CenteredByDefault& */
};


#include "medium_font.inc"

#include "huge_font.inc"



#define HUGE_FONT_START_LINE 1

typedef enum {
  layer_NONE = 0,
  layer_SMALL_TEXT = 1<<0,
  layer_MEDIUM_TEXT = 1<<1,
  layer_BIG_TEXT = 1<<2,
  layer_HUGE_TEXT = 1<<3,
  layer_ALL_TEXT = (1<<4)-1
} DisplayLayer;

#define DISPLAY_NUM_LAYERS 4


static const Font* gFont[DISPLAY_NUM_LAYERS] = {
  &gSmallFont,
  &gMediumFont,
  &gMediumFont, //todo gBigFont
  &gHugeFont,
};

//each screen_line is 8 bits high.
//All fonts must be in units of half lines
#define SCAN_LINE_COUNT (LCD_FRAME_HEIGHT/CHAR_BIT)
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
  TextProperties prop[DISPLAY_NUM_LAYERS];
} gDisplay;

char BLANK_LINE[]={" "};

void display_init(void) {
  int i;
  memset(&gDisplay, 0, sizeof(gDisplay));
  for (i=0;i<DISPLAY_NUM_LAYERS;i++)
  {
    gDisplay.text[i]=malloc(gFont[i]->LineCount * gFont[i]->CharsPerLine * sizeof(char));
    printf("allocated %d bytes for layer %d at %x\n", gFont[i]->LineCount * gFont[i]->CharsPerLine * sizeof(char), i, gDisplay.text[i] );

  }
}


static inline int min(int a, int b){  return a<b?a:b;}

void display_set_colors(int layer, uint16_t fg, uint16_t bg) {
  gDisplay.prop[layer].fg = fg;
  gDisplay.prop[layer].bg = bg;
}


void display_draw_text(int layer, int line , uint16_t fg, uint16_t bg, const char* text, int len, bool centered)
{
  const Font* font =  gFont[layer];
  assert(line < font->LineCount);
  printf("drawing %d %d into %x w/ %x\n", layer, line, gDisplay.text[layer], font );

  int nchars =  min(len, font->CharsPerLine);
  char* textline = gDisplay.text[layer]+(line*font->CharsPerLine);
  printf("text starts with %8s\n", textline);
  memset(textline, ' ', font->CharsPerLine);
  if (nchars == 0) {
    *textline = 0;
  }
  else  {
    int leftpad = centered? (font->CharsPerLine - nchars)/2 : 0;
    strncpy(textline+leftpad, text, nchars);
  }
  display_set_colors(layer, fg, bg);
}


static inline unsigned char restrict_to_printable(unsigned char c, unsigned char font_start, unsigned char font_end) {
  c -= font_start;
  return min(c, (font_end-font_start)+1);
}

static inline void set_map_properties(int scanline, int ct, TextProperties* tp) {
  if (tp->active) {
  int i;
  printf("setting scanlines %d..%d to %x/%x\n",scanline, scanline+ct, tp->fg, tp->bg);

  for (i=scanline;i<scanline+ct;i++) {
    gDisplay.prop[i]=*tp;
  }
  }
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
      TextProperties* tp = &gDisplay.prop[line];
      int scanline = mapline*2;
      set_map_properties( scanline, 2, tp);
      printf(": %s\n", text);
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
  printf("rendeering 16bit\n");

  for (line=0;line<font->LineCount;line++)
  {
    uint8_t* m1 = &bitmap[mapline*LCD_FRAME_WIDTH];
    uint8_t* m2 = &bitmap[(mapline+1)*LCD_FRAME_WIDTH];
    const char* text = gDisplay.text[layer]+(line*font->CharsPerLine);
    printf("text at %x is %8s\n", text, text);
    if (*text)  {
      TextProperties* tp = &gDisplay.prop[line];
      int scanline = mapline*2;
      if (shift) scanline++;
      set_map_properties( scanline, 2, tp);
      printf("< %s\n", text);
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
    printf("writing first bitmap entry at  %d\n",m1-bitmap);

    const char* text = gDisplay.text[layer]+(line*font->CharsPerLine);
    if (*text)  {
      TextProperties* tp = &gDisplay.prop[line];
      int scanline = mapline*2;
      set_map_properties( scanline, 2, tp);
      printf("^ %s\n", text);
      for (col=0;col<font->CharsPerLine; col++)
      {
        unsigned char c = restrict_to_printable(*text++, font->CharStart, font->CharEnd);

        const uint32_t* glyph = ((uint32_t*)font->glyphs) + (c * font->BitWidth);
//        const uint32_t* glyph = &Font->glyphs[c * font->BitWidth];
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
    printf("wrote  last bitmap entry at  %d\n",m4-bitmap-1);
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


void display_clear_layer(int layer, int fg, int bg) {
  int i;
  for (i=0; i<gFont[layer]->LineCount; i++)
  {
    display_draw_text(layer, i, fg, bg,
                      BLANK_LINE, 1, 0);
  }
}



#define EXTRACT_BIT(map,x,y) \
  (((map)[(x)+((y)/8)*LCD_FRAME_WIDTH]>>((y)%8))&1)


static void display_render_text(uint8_t layermask) {
  //two stage process
  //first render the fonts into a bitmap.
  uint8_t bitmap[LCD_FRAME_WIDTH * LCD_FRAME_HEIGHT/8]={0};
  printf("bitmap size is %d\n", sizeof(bitmap));
  int i, toplayer=0;

  for (i=0;i<DISPLAY_NUM_LAYERS;i++) {
    if (layermask & (1<<i) ) {
    printf("rendering %d\n", i);
      bitmap_render_text(bitmap, i, gFont[i]);
      toplayer = i;
    }
  }
  //second, convert the bitmap to color using the textprop values.
  int x,y;
  for (y=0;y<LCD_FRAME_HEIGHT;y++) {
    const TextProperties* tp = &gDisplay.prop[toplayer];
    printf("%02d %04x/%04x: ",y, tp->fg, tp->bg);
    for (x=0;x<LCD_FRAME_WIDTH;x++) {
      int isSet = EXTRACT_BIT(bitmap,x,y);
      uint16_t color = (isSet) ? tp->fg : tp->bg;
      gDisplay.frame.data[y*LCD_FRAME_WIDTH+x] = color;
      printf("%c", isSet?'X':'.');
    }
    printf("\n");
  }
  lcd_draw_frame(&gDisplay.frame);
  printf("ok\n");
}


static inline int hextoint(char digit)
{
  assert(isxdigit(digit));
  if (digit <= '9') return digit-'0';
  if (digit <= 'F') return digit-'A'+10;
  return digit-'a'+10;
}


static const char* parse_color(const char* cp, const char* endp, uint16_t* colorOut)
{
  if (cp<endp) {
    char colorchar=*cp++;
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
        *colorOut = 0;
        while (cp<endp && isxdigit(*cp)) {
          *colorOut = (*colorOut<<4)+hextoint(*cp++);
        }
        break;
      default:
        *colorOut = lcd_WHITE;
        break;
    }
  }
  return cp;
}

static void activate_layer(int layer, bool active) {
  if (active) {
    gDisplay.active_layers |= (1<<layer);
  }
  else {
    gDisplay.active_layers &= ~(1<<layer);
  }
}


void show_props(void) {
  int i;
  for (i=0;i<DISPLAY_NUM_LAYERS;i++){
    printf("textbuffer for layer %d is %x\n", i, gDisplay.text[i] );
  }

  printf("Active_layers = %02x\n",gDisplay.active_layers);
}


#define isdot(ch) ('.'==(ch))
#define isdash(ch) ('-'==(ch))

//display layer line [rgbit] all the following words
int display_parse(const char* command, int linelen)
{
  int layer=0, line=0;
  bool invert = 0, exclusive = 0;
  uint16_t  fgcolor=lcd_WHITE, bgcolor = lcd_BLACK;
  const char* cp = command;
  const char* endp = cp+linelen;

  //skip spaces
  while (cp<endp && isspace(*cp)) { cp++;}

  //layer  N[x]
  while (cp<endp && isdigit(*cp)){
    layer = layer*10 + (*cp++ - '0');
  }
  if (cp<endp  && ('x' == *cp)) {
    exclusive=1;
    cp++;
  }

  //to next char after space
  while (cp<endp && !isspace(*cp)){ cp++;}
  while (cp<endp && isspace(*cp)) { cp++;}

  //line
  while (cp<endp && isdigit(*cp)) {
    line = (line*10) + *cp++ - '0';
  }

  //to next char after space
  while (cp<endp && !isspace(*cp)){ cp++;}
  while (cp<endp && isspace(*cp)) { cp++;}



  // color code: -[i](rgbw|x0000)
  //   -w = white
  //   -ir = inverse red
  //   -xF81F = purple
  if (cp<endp  && ('i' == *cp)) {
    invert=1;
    cp++;
  }
  cp = parse_color(cp, endp, &fgcolor);

  printf("l.l = %d.%d.  >%s\n", layer,line, cp);
  printf("color = %c%04x >%s\n", invert?'i':' ', fgcolor, cp);
  if (invert) {
    uint16_t swap = bgcolor;
    bgcolor = fgcolor;
    fgcolor = swap;
  }

  //skip one space
  if (cp<endp && isspace(*cp)) { cp++;}
  printf("advanced to >%s\n", cp);


  if (layer==0 || layer >= DISPLAY_NUM_LAYERS) {layer = 1; }

  //line 0 deactivates layer.
  bool isActive = (line!=0);
  activate_layer(layer, isActive);

  show_props();

  uint8_t layermask = layer_ALL_TEXT;

  line--; //zero based now
  if (exclusive) {
    layermask = (1<<layer);
    display_clear_layer(layer, bgcolor, fgcolor);
  }
  display_draw_text(layer, line, fgcolor, bgcolor,
                    cp, endp-cp, gFont[layer]->CenteredByDefault);

  show_props();
  display_render_text(gDisplay.active_layers & layermask);
      printf("ok\n");

  return 0;

}

void on_exit(void) {
  lcd_shutdown();
}

#define TERMBUFSZ 80
#define LINEBUFSZ 256


int main(int argc, const char* argv[])
{
  bool exit = false;
  static char linebuf[LINEBUFSZ];
  int linelen = 0;

  dprintf("Initializing\n");

  lcd_init();


  display_init();


//  gDisplay.serialFd = serial_init(FIXTURE_TTY, FIXTURE_BAUD);

  while (!exit)
  {
    printf("reading\n");
    int nread = read(0, linebuf+linelen, LINEBUFSZ-linelen);
    printf("%s",linebuf);
    if (nread<0) { exit = true; break; }
    printf("searching\n");
    char* endl = memchr(linebuf+linelen, '\n', nread);
    if (!endl) {
      linelen+=nread;
      if (linelen >= LINEBUFSZ)
      {
        printf("TOO MANY CHARACTERS, truncating to %d\n", LINEBUFSZ);
        endl = linebuf+LINEBUFSZ-1;
        *endl = '\n';
      }
    }
    if (endl) {
      printf("parsing \"%s\"\n", linebuf);
      display_parse(linebuf, endl-linebuf);
      linelen = 0;
      printf("ok\n");
    }
  }
  return 0;
}
