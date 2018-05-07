// Based on Drive Testfix, updated for Cozmo EP1 Testfix
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "crypto/crypto.h"
#include "fixture.h"
#include "flash.h"
#include "meter.h"
#include "motorled.h"
#include "nvReset.h"
#include "robotcom.h"
#include "timer.h"

// Which character to escape into command code
#define ESCAPE_CODE 27

#define CONSOLE_TIMEOUT 50000   // Allow 50ms of typing before dropping out of command mode

#define BAUD_RATE   1000000

extern void SetFixtureText(bool reinit=0);
extern void SetOKText(void);
extern void SetErrorText(u16 error);

static int m_index = 0;
static char m_consoleBuffer[128];
static char m_parseBuffer[128];
static u8 m_numberOfArguments = 0;
static u8 m_isInConsoleMode = 0;
static u8 m_isReceivingSafe = 0;

static int m_readIndex = 0;

typedef struct
{
  const char* command;
  TestFunction function;
  bool doesCommunicateWithRobot;
} CommandFunction;

int ConsoleReadChar(void)
{  
  // NDTR counts down...
  if (DMA1_Stream2->NDTR == sizeof(m_consoleBuffer) - m_readIndex)
    return -1;
  
  int value = m_consoleBuffer[m_readIndex] & 0xFF;  // Don't sign-extend!
  m_readIndex = (m_readIndex + 1) % sizeof(m_consoleBuffer);
  
  return value;
}

int ConsoleGetCharNoWait(void)
{
  volatile u32 v;
  u32 status;
  
  // Check for overrun
  status = UART4->SR;
  if (status & USART_SR_ORE)
  {
    v = UART4->SR;
    v = UART4->DR;
  }
  
  return ConsoleReadChar();
}

int ConsoleGetCharWait(u32 timeout)
{
  volatile u32 v;
  u32 status;
  u32 startTime;
  int value;
  
  // Check for overrun
  status = UART4->SR;
  if (status & USART_SR_ORE)
  {
    v = UART4->SR;
    v = UART4->DR;
  }
  
  status = 0;
  value = -1;
  startTime = Timer::get();
  while (Timer::get() - startTime < timeout)
  {
    value = ConsoleReadChar();
    if (value >= 0)
      break;
  }
  
  return value;
}

int ConsolePrintf(const char* format, ...)
{
  char dest[128];
  va_list argptr;
  va_start(argptr, format);
  int len = vsnprintf(dest, sizeof(dest), format, argptr);
  va_end(argptr);
  ConsoleWrite(dest);
  return len;
}

void ConsolePutChar(char c)
{
  UART4->DR = c;
  while (!(UART4->SR & USART_FLAG_TXE))
    ;
}

int ConsoleWrite(char* s)
{
  int len = 0;
  while (*s)
  {
    UART4->DR = *s++;
    while (!(UART4->SR & USART_FLAG_TXE))
      ;
    len++;
  }
  
  return len;
}

char ConsoleGetChar()
{
  int value;
  while ((value = ConsoleReadChar()) < 0)
    ;
  
  // Clear any pending errors when reading the data
  volatile u32 v = UART4->SR;
  v = UART4->DR;
  
  return value;
}

void ConsoleWriteHex(const u8* buffer, u32 numberOfBytes, u32 offset)
{
  u32 i, j;
  u32 numberOfRows = (numberOfBytes + 15) >> 4;
  for (i = 0; i < numberOfRows; i++)
  {
    u32 index = i << 4;
    ConsolePrintf("%04x: ", (i << 4) + offset);
    for (j = 0; j < 16; j++)
    {
      ConsolePrintf("%02x ", buffer[index + j]);
    }
    ConsoleWrite((char*)"| ");
    for (j = 0; j < 16 && (index + j < numberOfBytes); j++)
    {
      char c = buffer[index + j];
      if (!(c >= 0x20 && c < 0x7E))
      {
        c = '.';
      }
      ConsolePrintf("%c", c);
    }
    ConsoleWrite((char*)"\n");
  }
}

void InitConsole(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStructure;

  // Clock configuration
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource0, GPIO_AF_UART4);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_UART4);
  
  // UART4 configuration
  USART_Cmd(UART4, DISABLE);
  USART_InitStructure.USART_BaudRate = BAUD_RATE;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
  USART_OverSampling8Cmd(UART4, ENABLE);
  USART_Init(UART4, &USART_InitStructure);
  USART_Cmd(UART4, ENABLE);
  
  // DMA configuration
  DMA_DeInit(DMA1_Stream2);
  
  DMA_InitTypeDef DMA_InitStructure;  
  DMA_InitStructure.DMA_Channel = DMA_Channel_4;
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&UART4->DR;
  DMA_InitStructure.DMA_Memory0BaseAddr = (u32)m_consoleBuffer;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
  DMA_InitStructure.DMA_BufferSize = sizeof(m_consoleBuffer);
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
  DMA_Init(DMA1_Stream2, &DMA_InitStructure);
  
  USART_DMACmd(UART4, USART_DMAReq_Rx, ENABLE);
  DMA_ITConfig(DMA1_Stream2, DMA_IT_TC, DISABLE);
  DMA_Cmd(DMA1_Stream2, ENABLE);
  
  //restore console mode from reset data
  m_isInConsoleMode = g_app_reset.valid && g_app_reset.console.isInConsoleMode;
}

// The fixture bootloader uses just the hardware driver above, not the command parser below
#ifndef FIXBOOT

static char* GetArgument(u32 index)
{
  if (index >= m_numberOfArguments)
  {
    throw ERROR_EMPTY_COMMAND;
  }
  
  u8 currentIndex = 0;
  char* buffer = m_parseBuffer;
  while (currentIndex != index)
  {
    if (*buffer == 0)
    {
      currentIndex++;
    }
    buffer++;
  }
  
  return buffer;
}

static void SetMode(void)
{
  char* arg = GetArgument(1);
  
  for (int i = 0; i < g_num_fixmodes; i++)
    if(g_fixmode_info[i].name && !strcasecmp(arg, g_fixmode_info[i].name))
    {
      g_flashParams.fixtureTypeOverride = i;
      StoreParams();
      g_fixmode = i;
      SetFixtureText(1);
      return;
    }
    
  throw ERROR_UNKNOWN_MODE;
}

static void SetSerial(void)
{
  char* arg = GetArgument(1);
  u32 serial = FIXTURE_SERIAL;
  sscanf(arg, "%i", &serial);
  
  // Check if this fixture already has a serial
  if (FIXTURE_SERIAL != 0xFFFFffff)
  {
    if( serial > 0 ) { //allow overwrite of existing serial to 0 (invalid)
      ConsolePrintf("Fixture already has serial: %i\n", serial);
      throw ERROR_SERIAL_EXISTS;
    }
  }
  
  //if ((u32)serial >= 0xf0)
  //  throw ERROR_SERIAL_INVALID;
  
  __disable_irq();
  FLASH_Unlock();
  FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL, serial & 255);
  FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+1, serial >> 8);
  FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+2, serial >> 16);
  FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+3, serial >> 24);
  FLASH_Lock();
  __enable_irq();
}

extern int g_canary;
static void GetSerialCmd(void)
{
  // Serial number, fixture type, build version
  ConsolePrintf("serial,%i,%s,%i\n", 
    FIXTURE_SERIAL, 
    fixtureName(),
    g_canary == 0xcab00d1e ? FIXTURE_VERSION : 0xbadc0de);    // This part is hard to explain
}
static void SetLotCode(void)
{
  char* arg = GetArgument(1);
  
  // Save room for the NULL byte
  if (strlen(arg) >= sizeof(g_lotCode))
  {
    ConsolePrintf("Lot code is too long\n");
    throw ERROR_LOT_CODE;
  }
  
  strcpy(g_lotCode, arg);
  SetFixtureText();
}

static void SetTime(void)
{
  char* arg = GetArgument(1);  
  sscanf(arg, "%i", &g_time);
}

static void SetDateCode(void)
{
  char* arg = GetArgument(1);  
  sscanf(arg, "%i", &g_dateCode);
}

static void TestCurrent(void)
{
  s32 v = Meter::getCurrentMa(PWR_VEXT);
  ConsolePrintf("I = %i, %X\n", v, v);
}

static void TestVoltage(void)
{
  s32 v = Meter::getVoltageMv(PWR_VEXT);
  ConsolePrintf("V = %i, %X\n", v, v);
}

static void DumpFixtureSerials(void)
{
  char* argOffset = GetArgument(1);
  
  u32 offset;
  sscanf(argOffset, "%x", &offset);
  
  if (offset >= (0x100000 >> 3))
  {
    throw ERROR_OUT_OF_RANGE;
  }
  
  u8* serials = (u8*)(0x08000000 + offset);
  ConsoleWriteHex(serials, 0x400);
}

static void BinVersionCmd(void)
{
  bool format_csv;
  try {
    char* arg = GetArgument(1);
    format_csv = false; //format for console view if user provided an arg.
  } 
  catch (int e) { 
    format_csv = true;
  }
  binPrintInfo(format_csv);
}

static void emmcdlVersionCmd(void)
{
  int display = 0, timeout_ms = 0;
  try {
    display = strtol(GetArgument(1),0,0);
    timeout_ms = strtol(GetArgument(2),0,0);
  } catch (int e) { }
  
  timeout_ms = timeout_ms < 1 ? CMD_DEFAULT_TIMEOUT : (timeout_ms > 600000 ? 600000 : timeout_ms);
  
  //read version from head and format display string
  char b[31]; int bz=sizeof(b);
  snformat(b,bz,"emmcdl: %s", helperGetEmmcdlVersion(timeout_ms));
  ConsolePrintf("%s\n", b);
  
  if( display ) {
    helperLcdSetLine(2, b);
    SetFixtureText();
  }
}

static void GetTemperatureCmd(void)
{
  int zone = DEFAULT_TEMP_ZONE;
  try { zone = strtol(GetArgument(1),0,0); } catch (int e) { }
  
  int tempC = helperGetTempC(zone);
  ConsolePrintf("zone %i: %iC\n", zone >= 0 ? zone : DEFAULT_TEMP_ZONE, tempC);
}

static void DutProgCmd_(void)
{
  int enable = 0;
  try { enable = strtol(GetArgument(1),0,0); } catch (int e) { }
  
  if( enable ) {
    Board::powerOn(PWR_DUTPROG);
    ConsolePrintf("DUT_PROG enabled\n");
  } else {
    Board::powerOff(PWR_DUTPROG);
    ConsolePrintf("DUT_PROG disabled\n");
  }
}

static void SetDetect_(void)
{
  int ms = 0;
  try { ms = strtol(GetArgument(1),0,0); } catch (int e) { }
  
  if( !ms )
    TestDebugSetDetect(); //default
  else
    TestDebugSetDetect(ms);
}

void SetMotor(void)
{
  int test = 0;
  char* arg = GetArgument(1);  
  sscanf(arg, "%i", &test);
  MotorMV(test);
}  

static void ForceStart(void) { g_forceStart = 1; }
static void Again(void) { g_isDevicePresent = false; SetDetect_(); }
static void AllowOutdated() { g_allowOutdated = true; }

static void ConsoleReset_(void)
{
  ConsoleWrite((char*)"\n");
  Timer::wait(10*1000);
  
  //save console state and issue soft reset
  g_app_reset.console.isInConsoleMode = 1;
  nvReset((u8*)&g_app_reset, sizeof(g_app_reset)); //this call will reset the mcu
  //NVIC_SystemReset();
  //while(1);
}

static void ConsolePrintModes_(void) {
  for(int n=0; n < g_num_fixmodes; n++ )
    if( g_fixmode_info[n].name )
      ConsolePrintf("  %02i %s\n", g_fixmode_info[n].mode, g_fixmode_info[n].name );
}

static CommandFunction m_functions[] =
{
  {"Again", Again, FALSE},
  {"Force", ForceStart, FALSE},
  {"AllowOutdated", AllowOutdated, FALSE},
  {"GetSerial", GetSerialCmd, FALSE},
  {"BinVersion", BinVersionCmd, FALSE},
  {"emmcdlVersion", emmcdlVersionCmd, FALSE},
  {"GetTemp", GetTemperatureCmd, FALSE},
  {"SetDateCode", SetDateCode, FALSE},
  {"SetLotCode", SetLotCode, FALSE},
  {"SetMode", SetMode, FALSE},
  {"GetModes", ConsolePrintModes_, FALSE},
  {"SetSerial", SetSerial, FALSE},
  {"SetTime", SetTime, FALSE},
  {"Current", TestCurrent, FALSE},
  {"DumpFixtureSerials", DumpFixtureSerials, FALSE},
  {"Voltage", TestVoltage, FALSE},
  {"SetMotor", SetMotor, FALSE},
  {"DUTProg", DutProgCmd_, FALSE},
  {"SetDetect", SetDetect_, FALSE},
  {"Reset", ConsoleReset_, FALSE},
  {"Exit", NULL, FALSE}, //processed directly - include here so it prints in 'help' list
};

static void ParseCommand(void)
{
  u32 i;
  char* buffer = m_parseBuffer;
  
  if (!strcasecmp(buffer, "exit"))
  {
    m_isInConsoleMode = 0;
    return;
  }
  else if (!strcasecmp(buffer, "help") || !strcasecmp(buffer, "?")) 
  {
    ConsoleWrite((char*)"Available commands:\n");
    for (i = 0; i < sizeof(m_functions) / sizeof(CommandFunction); i++)
      ConsolePrintf("  %s\n", m_functions[i].command);
    ConsoleWrite((char*)"\n");
  }
  else
  {
    // Tokenize by spaces
    m_numberOfArguments = 1;
    char* b = buffer;
    while (*b)
    {
      if (*b == ' ')
      {
        *b = 0;
        m_numberOfArguments++;
      }
      b++;
    }
    
    bool commandFound = 0;
    for (i = 0; i < sizeof(m_functions) / sizeof(CommandFunction); i++)
    {
      CommandFunction* cf = &m_functions[i];
      if (!strcasecmp(cf->command, buffer) && cf->function)
      {
        commandFound = 1;
        if (cf->doesCommunicateWithRobot && !g_isDevicePresent)
        {
          ConsolePrintf("No device present\n");
        } else {
          
          error_t error = ERROR_OK;
          try
          {
            cf->function();
          }
          catch (error_t e)
          {
            error = e;
          }
          
          ConsolePrintf("status,%i\n", error);
        }
        
        break;
      }
    }
    
    if (!commandFound && strcmp(buffer, ""))
    {
      ConsolePrintf("Unknown command: %s\n", buffer);
    }
  }
}

//keep partial line in a local buffer so we can flush it into the console
const  int  line_maxlen = 127;
static int  line_len = 0;
static char m_line[line_maxlen+1];
char* ConsoleGetLine(int timeout_us, int *out_len)
{
  //append to line - never destroy data
  int c, eol=0;
  uint32_t start = Timer::get();
  do //read 1 char (even if timeout==0)
  {
    if( (c = ConsoleReadChar()) > 0 ) { //ignore null; messes with ascii parser
      if( c == '\r' || c == '\n' )
        eol = 1;
      else if( line_len < line_maxlen )
        m_line[line_len++] = c;
      //else, whoops! drop data we don't have room for
    }
  }
  while( !eol && Timer::elapsedUs(start) < timeout_us );
  
  m_line[line_len] = '\0';
  if( out_len )
    *out_len = line_len; //always report read length (even if not EOL)
  
  if( eol ) {
    line_len = 0; //wash our hands of this data once we've passed it to caller
    return m_line;
  }
  return NULL;
}

int ConsoleFlushLine(void) {
  int n = line_len; //report how many chars we're dumping
  line_len = 0;
  return n;
}

void ConsoleProcessChar_(char c)
{
  if (m_isInConsoleMode)
  {
    //single button "again" key
    if( !m_index && c == '`' ) { //<-- 0x60, that little tick mark on the tilde key...
        ConsolePutChar('a'); m_parseBuffer[m_index++] = 'a';
        ConsolePutChar('g'); m_parseBuffer[m_index++] = 'g';
        ConsolePutChar('a'); m_parseBuffer[m_index++] = 'a';
        ConsolePutChar('i'); m_parseBuffer[m_index++] = 'i';
        ConsolePutChar('n'); m_parseBuffer[m_index++] = 'n';
        c = '\r'; //Enter
    }
    
    // Echo legal ASCII back to the console
    if (c >= 32 && c <= 126)
    {
      //Ignore chars if buffer is full
      if (m_index < sizeof(m_parseBuffer) - 1) { //leave 1 byte at end for NULL terminator
          ConsolePutChar(c);
          m_parseBuffer[m_index++] = c;
      }
    } else if (c == 27) {  // Check for escape
      ConsoleWrite((char*)"<ANKI>\n");
      m_index = 0;
    } else if (c == 0x7F || c == 8) {  // Check for delete or backspace keys
      if( m_index > 0 ) {
          ConsolePutChar(c);      //overwrite the onscreen char with a space
          ConsolePutChar(' ');    //"
          ConsolePutChar(c);      //"
          m_parseBuffer[--m_index] = 0;
      }
    } else if (c == '\r' || c == '\n') {
      ConsoleWrite((char*)"\n");
      m_parseBuffer[m_index] = 0;
      m_index = 0;
      ParseCommand();
    }
  } else {
    if (c == ESCAPE_CODE && !m_isReceivingSafe)
    {
      m_index = 0;
      m_parseBuffer[0] = 0;
      m_isInConsoleMode = 1;
      ConsoleWrite((char*)"<ANKI>\n");
    } else {
      if (!m_index &&  c != (SAFE_PLATFORM_TAG_A & 0xFF))
      {
        return;
      }
      
      u8* buffer = (u8*)&app_global_buffer[0];
      buffer[m_index++] = c;
      
      // Index is post-increment, so acknowledge the first 3 bytes - the 4th is acknowledged after the erase
      if (m_index < 4)
      {
        ConsolePutChar('1');
      }
      
      if (*(u32*)buffer == SAFE_PLATFORM_TAG_A && !m_isReceivingSafe)
      {
        m_isReceivingSafe = 1;
        FLASH_Unlock();
        FlashProgress(0);
        FLASH_EraseSector(FLASH_BLOCK_B_SECTOR_0, VoltageRange_3);
        FlashProgress(10);
        FLASH_EraseSector(FLASH_BLOCK_SECTOR_1, VoltageRange_3);
        FlashProgress(20);
        FLASH_EraseSector(FLASH_BLOCK_SECTOR_2, VoltageRange_3);
        FlashProgress(30);
        FLASH_EraseSector(FLASH_BLOCK_SECTOR_3, VoltageRange_3);
        FlashProgress(40);
        FLASH_EraseSector(FLASH_BLOCK_SECTOR_4, VoltageRange_3);
        FLASH_Lock();
        
        ConsolePutChar('1');  // Acknowledge erase
        
        while (m_isReceivingSafe)
        {
          buffer[m_index++] = ConsoleGetChar();
          //ConsolePutChar('1');
          if (m_index >= SAFE_BLOCK_SIZE)
          {
            DecodeAndFlash();
            m_index = 0;
          }
        }
      }
    }
  }
}

void ConsoleUpdate(void)
{
  if(line_len) { //some joker left data in the line buffer...
    for(int x=0; x < line_len; x++)
      ConsoleProcessChar_(m_line[x]); //shove it into the console processor
    line_len = 0;
  }
  
  int c = ConsoleReadChar();
  if (c > -1 ) {
    // Clear any pending errors when reading the data
    volatile u32 v = UART4->SR;
    v = UART4->DR;  // NDM: Why do we need this with DMA?
    
    ConsoleProcessChar_(c);
  }
}

#endif
