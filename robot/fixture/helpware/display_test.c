/* #include <stdint.h> */
/* #include <stdio.h> */
/* #include <stdbool.h> */
/* #include <stdlib.h> */
/* #include <assert.h> */
#include <string.h>
#include <ctype.h>
/* #include <fcntl.h> */
#include <unistd.h>
/* #include <limits.h> */

#include "core/common.h"
#include "core/lcd.h"

#include "helpware/display.h"

#ifdef EXTENDED_DISPLAY_DEBUGGING
#define ddprintf printf
#else
#define ddprintf(f,...)
#endif



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

  if (invert) {
    uint16_t swap = bgcolor;
    bgcolor = fgcolor;
    fgcolor = swap;
  }

  //skip one space
  if (cp<endp && isspace(*cp)) { cp++;}


  if (layer==0 || layer >= DISPLAY_NUM_LAYERS) {layer = 1; }



  uint8_t layermask = DISPLAY_MASK_ALL;

  line--; //zero based now
  if (exclusive) {
    layermask = (1<<layer);
    display_clear_layer(layer, bgcolor, fgcolor);
  }
  display_draw_text(layer, line, fgcolor, bgcolor,
                    cp, endp-cp, (layer==3));

  display_render(layermask);
  return 0;

}

void core_common_on_exit(void) {
  lcd_shutdown();
}

#define TERMBUFSZ 80
#define LINEBUFSZ 256


int main(int argc, const char* argv[])
{
  bool exit = false;
  static char linebuf[LINEBUFSZ];
  int linelen = 0;

  ddprintf("Initializing\n");

  lcd_init();


  display_init();


  while (!exit)
  {
    printf("reading\n");
    int nread = read(0, linebuf+linelen, LINEBUFSZ-linelen);
    printf("%s",linebuf);
    if (nread<=0) { exit = true; break; }
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
