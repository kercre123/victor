#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include "core/common.h"
#include "core/lcd.h"
#include "helpware/display.h"



#define SMALL_LINE_COUNT 8

#define HELPER_SMALL_TEXT_COLOR_FG lcd_BLUE
#define HELPER_SMALL_TEXT_COLOR_BG lcd_BLACK


void helper_text_small(int line, const char *text, int len) {
  display_draw_text(DISPLAY_LAYER_SMALL, line-1, HELPER_SMALL_TEXT_COLOR_FG, HELPER_SMALL_TEXT_COLOR_BG,  text, len, 0);
}

void helper_text_large(uint16_t fg, uint16_t bg, const char *text, int len) {
  display_clear_layer(DISPLAY_LAYER_LARGE, fg, bg);
  display_draw_text(DISPLAY_LAYER_LARGE, 1, fg, bg, text, len, 1);
}

void helper_text_show(int largeSolo)
{
  uint8_t rendermask = 0xFF;
  if (largeSolo) {
    rendermask = 1<<DISPLAY_LAYER_LARGE;
  }
  display_render(rendermask);
}

  ///> lcdset <line> all the text
int helper_lcdset_command_parse(const char* command, int linelen)
{
  const char* cp = command;
  const char* endp = cp+linelen;
  uint8_t line = 0;

  //eat whitespace
  while (cp<endp && isspace(*cp)) { cp++;}

  //line N
  while (cp<endp && isdigit(*cp)){
    line = line*10 + (*cp++ - '0');
  }
  //eat one space.
  while (cp<endp && !isspace(*cp)){ cp++;}
  if (cp<endp && isspace(*cp)) { cp++;}

  if (line > SMALL_LINE_COUNT) {
    return -1;
  }
  if (line == 0) {
    for (line=0;line<SMALL_LINE_COUNT;line++)
      helper_text_small(line, " ", 1);
  }
  else {
    helper_text_small(line, cp, endp-cp);
  }
  helper_text_show(0);
  return 0;
}
/// >>lcdshow solo color all the text
int helper_lcdshow_command_parse(const char* command, int linelen)
{
  const char* cp = command;
  const char* endp = cp+linelen;
  bool solo = 0, invert = 0;
  uint16_t fgcolor, bgcolor = lcd_BLACK;

  //eat whitespace
  while (cp<endp && isspace(*cp)) { cp++;}

  //solo flag
  while (cp<endp && isdigit(*cp)){
    solo = !(*cp++ == '0');
  }

  //eat whitespace
  while (cp<endp && !isspace(*cp)){ cp++;}
  while (cp<endp && isspace(*cp)) { cp++;}

  // color code: [i](rgbw|x0000)
  //   -w = white
  //   -ir = inverse red
  //   -xF81F = purple
  if (cp<endp  && ('i' == *cp)) {
    invert=1;
    cp++;
  }
  cp = parse_color(cp, endp, &fgcolor);
  if (invert) {
    uint16_t swap = bgcolor;
    bgcolor = fgcolor;
    fgcolor = swap;
  }

  //eat one space.
  if (cp<endp && isspace(*cp)) { cp++;}

  //eat trailing space
  while (cp<endp && isspace(endp[-1])) { endp--;}

  helper_text_large(fgcolor, bgcolor, cp, endp-cp);
  helper_text_show(solo);
  return 0;
}

int helper_lcd_command_parse(const char* command, int linelen)
{
  const char* cp = command;
  const char* endp = cp+linelen;

  //must be "set" or "show"
  if (cp<endp && *cp=='s') {
    if (++cp<endp && *cp=='e') {
      if (++cp<endp && *cp=='t') {
        cp++;
        return helper_lcdset_command_parse(cp, endp-cp);
      }
    }
    else if (cp<endp && *cp=='h') {
      if (++cp<endp && *cp=='o') {
        if (++cp<endp && *cp=='w') {
          cp++;
          return helper_lcdshow_command_parse(cp, endp-cp);
        }
      }
    }
  }
  return -1; //invalid
}

void helper_lcd_busy_spinner(void) {
  const char *glyphs = "|/-\\";
  static int i=0;
  helper_text_large(lcd_BLUE|lcd_GREEN, lcd_BLACK, glyphs+i, 1);
  helper_text_show(1);
  i=(i+1)%4;
}



//#define SELF_TEST
#ifdef SELF_TEST





void core_common_on_exit(void) {
  lcd_shutdown();
}



int main(int argc, const char* argv[])
{
  bool exit = false;
  int serialFd;

  lcd_init();
  display_init();

#ifdef SERIAL_TEST
  serialFd = serial_init(FIXTURE_TTY, FIXTURE_BAUD);
#else
  serialFd = 0; //stdin
#endif

  while (!exit)
  {

    fixture_serial(serialFd);
  }
  return 0;
}


#endif
