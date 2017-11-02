#ifndef ANKI_HELPER_TEXT_H_
#define ANKI_HELPER_TEXT_H_


int helper_lcdset_command_parse(const char* command, int linelen);
int helper_lcdshow_command_parse(const char* command, int linelen);

void helper_lcd_busy_spinner(void);


#endif//ANKI_HELPER_TEXT_H_
