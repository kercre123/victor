#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#include "core/common.h"
#include "core/lcd.h"
#include "display.h"

#define LINEBUFSZ 255



#define LAYER_SMALL 1
#define LAYER_LARGE 3

#define SMALL_LINE_COUNT 8

#define HELPER_SMALL_TEXT_COLOR_FG lcd_BLUE
#define HELPER_SMALL_TEXT_COLOR_BG lcd_BLACK

static inline int min(int a, int b){  return a<b?a:b;}


void helper_text_small(int line, const char *text, int len) {
  display_draw_text(LAYER_SMALL, line-1, HELPER_SMALL_TEXT_COLOR_FG, HELPER_SMALL_TEXT_COLOR_BG,  text, len, 0);
}

void helper_text_large(uint16_t fg, uint16_t bg, const char *text, int len, bool solo) {
  if (solo || len==0) {
    display_clear_layer(LAYER_LARGE, fg, bg);
  }
  display_draw_text(LAYER_LARGE, 1, fg, bg, text, len, 1);
}

void helper_text_show(int largeSolo)
{
  uint8_t rendermask = 0xFF;
  if (largeSolo) {
    rendermask = 1<<LAYER_LARGE;
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

  if (line > SMALL_LINE_COUNT) {line = 1;}
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
/// >>>lcdshow solo color all the text
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

  helper_text_large(fgcolor, bgcolor, cp, endp-cp, solo);
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

const char* fixture_command_parse(const char*  command, int len) {
  static char responseBuffer[LINEBUFSZ];

  dprintf("parsing \"%s\"\n", command);
  if (strncmp(command, "lcdset", min(6,len))==0)
  {
    int status = helper_lcdset_command_parse(command+6, len-6);
    snprintf(responseBuffer, LINEBUFSZ, "lcdset %d\n", status);
    return responseBuffer;
  }
  if (strncmp(command, "lcdshow", min(7,len))==0)
  {
    int status = helper_lcdshow_command_parse(command+7, len-7);
    snprintf(responseBuffer, LINEBUFSZ, "lcdshow %d\n", status);
    return responseBuffer;
  }

  //not recognized, echo back invalid command with error code
  char* endcmd = memchr(command, ' ', len);
  if (endcmd) { len = endcmd - command; }
  int i;
  for (i=0;i<len;i++)
  {
    responseBuffer[i]=*command++;
  }
  snprintf(responseBuffer+i, LINEBUFSZ-i, " %d\n", -1);
  return responseBuffer;

}

void fixture_serial(int serialFd) {
  static char linebuf[LINEBUFSZ+1];
  const char* response;
  int linelen = 0;
  int nread = read(serialFd, linebuf+linelen, LINEBUFSZ-linelen);
  dprintf("%s",linebuf);
  if (nread<0) { return; }
  char* endl = memchr(linebuf+linelen, '\n', nread);
  if (!endl) {
    linelen+=nread;
    if (linelen >= LINEBUFSZ)
    {
      printf("TOO MANY CHARACTERS, truncating to %d\n", LINEBUFSZ);
      endl = linebuf+LINEBUFSZ;
      *endl = '\n';
    }
  }
  if (endl) {
    endl[1]='\0';
    response = fixture_command_parse(linebuf, endl-linebuf);
    write(serialFd, response, strlen(response));
    linelen = 0;
  }
}

#define SELF_TEST
#ifdef SELF_TEST





void on_exit(void) {
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
