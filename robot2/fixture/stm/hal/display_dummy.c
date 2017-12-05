#include "hal/display.h"

//board.h: OLED_MOSI -> B.5
//static GPIO_TypeDef* MOSI_PORT = GPIOB;
//static const uint32_t MOSI_PIN = GPIO_Pin_5;
//static const uint32_t MOSI_SOURCE = GPIO_PinSource5;

//board.h: OLED_SCK  -> B.3
//static GPIO_TypeDef* SCK_PORT = GPIOB;
//static const uint32_t SCK_PIN = GPIO_Pin_3;
//static const uint32_t SCK_SOURCE = GPIO_PinSource3;

//board.h: OLED_CSN  -> C.0
//static GPIO_TypeDef* CS_PORT = GPIOC;
//static const uint32_t CS_SOURCE = GPIO_PinSource0;

//board.h: OLED_DCN  -> B.7
//static GPIO_TypeDef* CMD_PORT = GPIOB;
//static const uint32_t CMD_SOURCE = GPIO_PinSource7;

//board.h: OLED_RESN -> B.4
//static GPIO_TypeDef* RES_PORT = GPIOB;
//static const uint32_t RES_SOURCE = GPIO_PinSource4;

void InitDisplay(void) { }
void DisplaySetScroll(bool scroll) { }
void DisplayInvert(bool invert) { }
void DisplayFlip(void) { }
void DisplayUpdate(void) { }
void DisplayClear() { }
void DisplayPutChar(char character) { }
void DisplayPutString(const char* string) { }
void DisplayMoveCursor(u16 line, u16 column) { }
void DisplayTextWidthMultiplier(u16 multiplier) { }
void DisplayTextHeightMultiplier(u16 multiplier) { }
void DisplayPrintf(const char *format, ...) { }
void DisplayBigCenteredText(const char *format, ...) { }
