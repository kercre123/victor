#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#include "core/common.h"
#include "core/lcd.h"



static const uint8_t gSmallFont[] = {
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

#include "medium_font.inc"

#define MEDIUM_FONT_CHARS_PER_LINE (LCD_FRAME_WIDTH/(MEDIUM_FONT_WIDTH+1))
#define MEDIUM_FONT_LINE_COUNT (LCD_FRAME_HEIGHT/MEDIUM_FONT_HEIGHT)

#include "huge_font.inc"

#define HUGE_FONT_CHARS_PER_LINE (LCD_FRAME_WIDTH/(HUGE_FONT_WIDTH+1))
#define HUGE_FONT_LINE_COUNT (LCD_FRAME_HEIGHT/HUGE_FONT_HEIGHT)


#define HUGE_FONT_START_LINE 1

//each screen_line is 8 bits high.
//All fonts must be in units of half lines
#define SCREEN_LINE_COUNT (LCD_FRAME_HEIGHT/CHAR_BIT)
/****************/

//each layer has N lines of text.
// each line of text has content, fg and bg colors. Content can be zero.
//each layer can be active inactive or exclusive. Exclusive inactivates all the others.
//higher layers hide lower ones.
///. so .  Draw the bitmap from high to low.
// fill in the colors as you go.


#define TERMBUFSZ 80
#define LINEBUFSZ 256


typedef enum {
  layer_NONE = 0,
  layer_SMALL_TEXT = 1<<0,
  layer_MEDIUM_TEXT = 1<<1,
  layer_HUGE_TEXT = 1<<2,
  layer_ALL_TEXT = (1<<3)-1
} DisplayLayer;
#define DISPLAY_NUM_LAYERS 3

#define isdot(ch) ('.'==(ch))
#define isdash(ch) ('-'==(ch))


// W  x H
//184 x 96 pixels
//5 x 8 font.
// 30 chars x 12 lines
//--or
//15 x 24 font
// 11 chars x 12 lines.

LcdFrame textbuffer;

typedef struct TextProperties_t {
  uint16_t fg;
  uint16_t bg;
  bool active;
} TextProperties;


struct {
  TextProperties small_prop[SMALL_FONT_LINE_COUNT];
  TextProperties med_prop[MEDIUM_FONT_LINE_COUNT];
  TextProperties huge_prop[HUGE_FONT_LINE_COUNT];
  char smalltext[SMALL_FONT_LINE_COUNT][SMALL_FONT_CHARS_PER_LINE];
  char medtext[MEDIUM_FONT_LINE_COUNT][MEDIUM_FONT_CHARS_PER_LINE];
  char hugetext[HUGE_FONT_LINE_COUNT][HUGE_FONT_CHARS_PER_LINE];
  LcdFrame frame;
  uint8_t active_layers;
  TextProperties map_prop[SCREEN_LINE_COUNT*2];
} gDisplay;// = {{{lcd_BLUE, lcd_BLACK, 0}},0};


char BLANK_LINE[]={" "};

/* void display_init(void) { */
/*   gDisplay.smalltext.text = malloc(SMALL_FONT_ */
/* } */


static inline int min(int a, int b){  return a<b?a:b;}



void draw_text_small(int line, uint16_t fg, uint16_t bg, const char* text, int len)
{
  assert(line<SMALL_FONT_LINE_COUNT);
  int nchars =  min(len, SMALL_FONT_CHARS_PER_LINE);
  memset(gDisplay.smalltext[line], ' ', SMALL_FONT_CHARS_PER_LINE);
  if (nchars == 0) {
    *gDisplay.smalltext[line]='\0';
  }
  else  {
    strncpy(gDisplay.smalltext[line], text, nchars);
  }
  gDisplay.small_prop[line].fg = fg;
  gDisplay.small_prop[line].bg = bg;
  gDisplay.small_prop[line].active = (nchars>0);
}
void draw_text_medium(int line, uint16_t fg, uint16_t bg, const char* text, int len)
{
  assert(line<MEDIUM_FONT_LINE_COUNT);
  int nchars =  min(len, MEDIUM_FONT_CHARS_PER_LINE);
  memset(gDisplay.medtext[line], ' ', MEDIUM_FONT_CHARS_PER_LINE);
  if (nchars == 0) {
    *gDisplay.medtext[line]='\0';
  }
  else  {
    strncpy(gDisplay.medtext[line], text, nchars);
  }
  gDisplay.med_prop[line].fg = fg;
  gDisplay.med_prop[line].bg = bg;
  gDisplay.med_prop[line].active = (nchars>0);
}

void draw_text_huge(int line, uint16_t fg, uint16_t bg, const char* text, int len)
{
  int nchars = min(len, HUGE_FONT_CHARS_PER_LINE);
  int npad = (HUGE_FONT_CHARS_PER_LINE - nchars )/ 2;
  printf("NPAD = %d\n", npad);

  line = HUGE_FONT_START_LINE;
  memset(gDisplay.hugetext[line], ' ', HUGE_FONT_CHARS_PER_LINE);
  if (nchars == 0) {
    *gDisplay.hugetext[line]='\0';
  }
  else  {
      strncpy(gDisplay.hugetext[line]+npad, text, nchars);
  }
  gDisplay.huge_prop[line].fg = fg;
  gDisplay.huge_prop[line].bg = bg;
  gDisplay.huge_prop[line].active = (nchars > 0);

}

static inline unsigned char restrict_to_printable(unsigned char c, unsigned char font_start, unsigned char font_end) {
  c -= font_start;
  return min(c, (font_end-font_start)+1);
}


/* static inline const TextProperties* set_textprops(int y, uint8_t layermask) */
/* { */
/*   const TextProperties* retval = &gDisplay.small_prop[0]; */
/*   int line = y/(CHAR_BIT/2); */
/*   if (line/2 > SMALL_FONT_LINE_COUNT) return retval; */
/*   printf("checking text properties for halfrow %d (%x)\n",line, layermask); */
/*   if (layermask & layer_SMALL_TEXT) { */
/*     if (gDisplay.small_prop[line/2].active){ */
/*       retval = &gDisplay.small_prop[line/2]; */
/*       printf("active at small %d -  %x/%x\n", line/2, retval->fg, retval->bg); */
/*     } */
/*   } */
/*   if (layermask & layer_MEDIUM_TEXT) { */
/*     if (gDisplay.med_prop[line/3].active){ */
/*       retval = &gDisplay.med_prop[line/3]; */
/*       printf("active at med %d -  %x/%x\n", line/3, retval->fg, retval->bg); */
/*     } */
/*   } */
/*   if (layermask & layer_HUGE_TEXT) { */
/*     if (gDisplay.huge_prop[line/8].active){ */
/*       retval = &gDisplay.huge_prop[line/8]; */
/*       printf("active at huge %d -  %x/%x\n", line/8, retval->fg, retval->bg); */
/*     } */
/*   } */

/*   return retval; */
/* } */

static inline void set_map_properties(int scanline, int ct, TextProperties* tp) {
  if (tp->active) {
  int i;
  printf("setting scanlines %d..%d to %x/%x\n",scanline, scanline+ct, tp->fg, tp->bg);

  for (i=scanline;i<scanline+ct;i++) {
    gDisplay.map_prop[i]=*tp;
  }
  }
}

void display_render_small_text(uint8_t* bitmap)
{
  //bitmap is 12 rows of uint8's, FRAME_WIDTH pixels wide.
  // small text puts 1 lines in each row
  int line,col;

  for (line=0;line<SMALL_FONT_LINE_COUNT;line++)
  {
    uint8_t* map = &bitmap[line*LCD_FRAME_WIDTH];
    const char* text = gDisplay.smalltext[line];
    if (*text)  {
      TextProperties* tp = &gDisplay.small_prop[line];
      set_map_properties( line*2, 2, tp);
      printf(": %s\n", text);
      for (col=0;col<SMALL_FONT_CHARS_PER_LINE; col++)
      {
        unsigned char c = restrict_to_printable(*text++, SMALL_FONT_CHAR_START,SMALL_FONT_CHAR_END);

        const uint8_t* glyph = &gSmallFont[c * SMALL_FONT_WIDTH];
        int i;
        for (i=0;i<SMALL_FONT_WIDTH;i++)
        {
          *map++ |= (*glyph++);
        }
        *map++ = 0; //char separator
      }
    }
  }
}


void display_render_medium_text(uint8_t* bitmap)
{
  //bitmap is 12 rows of uint8's, FRAME_WIDTH pixels wide.
  // medium text puts 2  lines in each 3 rows
  int line, col;
  int shift = 0;
  int mapline = 0;
  for (line=0;line<MEDIUM_FONT_LINE_COUNT;line++)
  {
    uint8_t* m1 = &bitmap[mapline*LCD_FRAME_WIDTH];
    uint8_t* m2 = &bitmap[(mapline+1)*LCD_FRAME_WIDTH];
    const char* text = gDisplay.medtext[line];
    if (text)  {
      TextProperties* tp = &gDisplay.med_prop[line];
      set_map_properties(mapline*2+(shift!=0), 3, tp);
      /* if (!shift) { */
      /*   gDisplay.map_prop[mapline*2]=*tp; */
      /* } */
      /* gDisplay.map_prop[mapline*2+1]=*tp; */
      /* gDisplay.map_prop[(mapline+1)*2]=*tp; */
      /* if (shift) { */
      /*   gDisplay.map_prop[(mapline+1)*2+1]=*tp; */
      /* } */

      printf("<<%d %s \n", shift, text);
      for (col=0;col<MEDIUM_FONT_CHARS_PER_LINE; col++)
      {
        unsigned char c = restrict_to_printable(*text++, MEDIUM_FONT_CHAR_START,MEDIUM_FONT_CHAR_END);
        const uint16_t* glyph = &gMediumFont[c*MEDIUM_FONT_WIDTH];
        int i;
        for (i=0;i<MEDIUM_FONT_WIDTH;i++)
        {
          uint16_t gg =  (*glyph++)<<shift;
//        printf("Rendering %c (%04x) into rows %d & %d\n", c+MEDIUM_FONT_CHAR_START, gg, mapline, mapline+1);
          *m1++ |= (gg&0xFF);
          *m2++ |= (gg>>8);

        }
        *m1++ = 0; //char separator
        *m2++ = 0; //char separator
      }
    }
    shift ^=4;
    mapline+= (shift)?1:2;
  }
}


void display_render_huge_text(uint8_t* bitmap)
{
  //bitmap is 12 rows of uint8's, FRAME_WIDTH pixels wide.
  // huge text puts 1 line over 4 rows.
  int line, col;
  int mapline = 0;
  for (line=0;line<HUGE_FONT_LINE_COUNT;line++)
  {
    uint8_t* m1 = &bitmap[mapline*LCD_FRAME_WIDTH];
    uint8_t* m2 = &bitmap[(mapline+1)*LCD_FRAME_WIDTH];
    uint8_t* m3 = &bitmap[(mapline+2)*LCD_FRAME_WIDTH];
    uint8_t* m4 = &bitmap[(mapline+3)*LCD_FRAME_WIDTH];
    printf("writing first bitmap entry at  %d\n",m1-bitmap);
    const char* text = gDisplay.hugetext[line];
    if (*text)  {
      TextProperties* tp = &gDisplay.huge_prop[line];
      set_map_properties(mapline*2,8,tp);

      printf("^ %s\n", text);
      for (col=0;col<HUGE_FONT_CHARS_PER_LINE; col++)
      {
        unsigned char c = restrict_to_printable(*text++, HUGE_FONT_CHAR_START,HUGE_FONT_CHAR_END);
        const uint32_t* glyph = &gHugeFont[c*HUGE_FONT_WIDTH];
        int i;
        for (i=0;i<HUGE_FONT_WIDTH;i++)
        {
          uint32_t gg =  (*glyph++);
          *m1++ |= (gg&0xFF);
          *m2++ |= ((gg>>8)&0xFF);
          *m3++ |= ((gg>>16)&0xFF);
          *m4++ |= (uint8_t)(gg>>24);
        }
        /* *m1++ = 0; //char separator */
        /* *m2++ = 0; //char separator */
        /* *m3++ = 0; //char separator */
        /* *m4++ = 0; //char separator */
      }
    }
    mapline+= 4;
  printf("wrote  last bitmap entry at  %d\n",m4-bitmap-1);
  }


}


static inline const TextProperties* get_textprops(int y, uint8_t layermask)
{
  return &gDisplay.map_prop[y/(CHAR_BIT/2)];
  /* change this to read map_prop[]; */
  /* const TextProperties* retval = &gDisplay.small_prop[0]; */
  /* int line = y/(CHAR_BIT/2); */
  /* if (line/2 > SMALL_FONT_LINE_COUNT) return retval; */
  /* printf("checking text properties for halfrow %d (%x)\n",line, layermask); */
  /* if (layermask & layer_SMALL_TEXT) { */
  /*   if (gDisplay.small_prop[line/2].active){ */
  /*     retval = &gDisplay.small_prop[line/2]; */
  /*     printf("active at small %d -  %x/%x\n", line/2, retval->fg, retval->bg); */
  /*   } */
  /* } */
  /* if (layermask & layer_MEDIUM_TEXT) { */
  /*   if (gDisplay.med_prop[line/3].active){ */
  /*     retval = &gDisplay.med_prop[line/3]; */
  /*     printf("active at med %d -  %x/%x\n", line/3, retval->fg, retval->bg); */
  /*   } */
  /* } */
  /* if (layermask & layer_HUGE_TEXT) { */
  /*   if (gDisplay.huge_prop[line/8].active){ */
  /*     retval = &gDisplay.huge_prop[line/8]; */
  /*     printf("active at huge %d -  %x/%x\n", line/8, retval->fg, retval->bg); */
  /*   } */
  /* } */

  /* return retval; */
}



#define EXTRACT_BIT(map,x,y) \
  (((map)[(x)+((y)/8)*LCD_FRAME_WIDTH]>>((y)%8))&1)


static void display_render_text(uint8_t layermask) {
  //two stage process
  //first render the fonts into a bitmap.
  uint8_t bitmap[LCD_FRAME_WIDTH * LCD_FRAME_HEIGHT/8]={0};
  printf("bitmap size is %d\n", sizeof(bitmap));
  if (layermask & layer_SMALL_TEXT) {
    display_render_small_text(bitmap);
  }
  if (layermask & layer_MEDIUM_TEXT) {
    display_render_medium_text(bitmap);
  }
  if (layermask & layer_HUGE_TEXT) {
    display_render_huge_text(bitmap);
  }
  //second, convert the bitmap to color using the textprop values.
  int x,y;
  for (y=0;y<LCD_FRAME_HEIGHT;y++) {
//    printf("y=%d\n",y);
    const TextProperties* tp = get_textprops(y, layermask);
    printf("%02d %04x/%04x: ",y, tp->fg, tp->bg);
    for (x=0;x<LCD_FRAME_WIDTH;x++) {
//      printf("extract?");
      int isSet = EXTRACT_BIT(bitmap,x,y);
//      printf("ok\n");
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
  /* printf("small_prop "); */
  /* for (i=0;i<SMALL_FONT_LINE_COUNT;i++){ */
  /*   printf("%d ", gDisplay.small_prop[i].active); */
  /* } */
  /* printf("\n"); */
  /* printf("med_prop "); */
  /* for (i=0;i<MEDIUM_FONT_LINE_COUNT;i++){ */
  /*   printf("%d ", gDisplay.med_prop[i].active); */
  /* } */
  /* printf("\n"); */
  /* printf("huge_prop "); */
  /* for (i=0;i<HUGE_FONT_LINE_COUNT;i++){ */
  /*   printf("%d ", gDisplay.huge_prop[i].active); */
  /* } */
  printf("Active_layers = %02x\n",gDisplay.active_layers);
}

//display layer line [rgbit] all the following words
int display_parse(const char* command, int linelen)
{
  int i;
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
  switch (layer) {
    case 1:
    {
      if (exclusive) {
        layermask = layer_MEDIUM_TEXT;
        for (i=0; i<MEDIUM_FONT_LINE_COUNT; i++)
        {
          draw_text_medium(i, fgcolor, bgcolor,
                           BLANK_LINE, 1);
        }
      }
      draw_text_medium(line, fgcolor, bgcolor, cp, endp-cp);  //5x8
      break;
    }
    case 2:
      printf("HUGE %s\n", cp);
      if (exclusive) {
        layermask = layer_HUGE_TEXT;
        for (i=0; i<MEDIUM_FONT_LINE_COUNT; i++)
        {
          draw_text_huge(i, fgcolor, bgcolor,
                           BLANK_LINE, 1);
        }
      }
      printf("huge line %d\n", line);
      draw_text_huge(line, fgcolor, bgcolor, cp, endp-cp); //22x32?
      break;
    default:
      assert(!layer); //invalid layer
      break;
  }
  show_props();
  display_render_text(gDisplay.active_layers & layermask);
      printf("ok\n");

  return 0;

}

void on_exit(void) {
  lcd_shutdown();
}

int main(int argc, const char* argv[])
{
  bool exit = false;
  static char linebuf[LINEBUFSZ];
  int linelen = 0;

  dprintf("Initializing\n");

  lcd_init();

  memset(&gDisplay, 0, sizeof(gDisplay));

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
