// Based on Drive Testfix, updated for Cozmo EP1 Testfix
#include "hal/console.h"
#include "hal/monitor.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/flash.h"
#include "hal/uart.h"
#include "hal/display.h"
#include "hal/motorled.h"
#include "hal/board.h"
#include "hal/radio.h"
#include "../../crypto/crypto.h"
#include "../app/fixture.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "../app/app.h"
#include "../app/nvReset.h"
#include "stdlib.h"

// Which character to escape into command code
#define ESCAPE_CODE 27

#define CONSOLE_TIMEOUT 50000   // Allow 50ms of typing before dropping out of command mode

#define BAUD_RATE   1000000

extern BOOL g_isDevicePresent;
extern FixtureType g_fixtureType;
extern char* FIXTYPES[];

extern void SetFixtureText(void);
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
  BOOL doesCommunicateWithRobot;
} CommandFunction;

static int ConsoleReadChar(void)
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
  startTime = getMicroCounter();
  while (getMicroCounter() - startTime < timeout)
  {
    value = ConsoleReadChar();
    if (value >= 0)
      break;
  }
  
  return value;
}

int ConsolePrintf(const char* format, ...)
{
  char dest[64];
  va_list argptr;
  va_start(argptr, format);
  int len = vsnprintf(dest, 64, format, argptr);
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
    ConsoleWrite("| ");
    for (j = 0; j < 16 && (index + j < numberOfBytes); j++)
    {
      char c = buffer[index + j];
      if (!(c >= 0x20 && c < 0x7E))
      {
        c = '.';
      }
      ConsolePrintf("%c", c);
    }
    ConsoleWrite("\r\n");
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
  
  SlowPutString("Console Initialized\r\n");
  
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
  
  for (int i = 0; i <= FIXTURE_DEBUG; i++)
    if (!strcasecmp(arg, FIXTYPES[i]))
    {
      g_flashParams.fixtureTypeOverride = i;
      StoreParams();
      g_fixtureType = i;
      SetFixtureText();
      return;
    }
    
  throw ERROR_UNKNOWN_MODE;
}

static void RedoTest(void)
{
  g_isDevicePresent = 0;
}

static void SetSerial(void)
{
  char* arg;
  u32 serial = FIXTURE_SERIAL;
  
  // Check if this fixture already has a serial
  if (serial != 0xFFFFffff)
  {
    ConsolePrintf("Fixture already has serial: %i\r\n", serial);
    throw ERROR_SERIAL_EXISTS;
  }
  
  arg = GetArgument(1);
  sscanf(arg, "%i", &serial);
  
  if ((u32)serial >= 0xf0)
    throw ERROR_SERIAL_INVALID;
  
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
  ConsolePrintf("serial,%i,%s,%i\r\n", 
    FIXTURE_SERIAL, 
    FIXTYPES[g_fixtureType],
    g_canary == 0xcab00d1e ? FIXTURE_VERSION : 0xbadc0de);    // This part is hard to explain
}
static void SetLotCode(void)
{
  char* arg = GetArgument(1);
  
  // Save room for the NULL byte
  if (strlen(arg) >= sizeof(g_lotCode))
  {
    ConsolePrintf("Lot code is too long\r\n");
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
  s32 v = MonitorGetCurrent();
  ConsolePrintf("I = %i, %X\r\n", v, v);
}

static void TestVoltage(void)
{
  s32 v = MonitorGetVoltage();
  ConsolePrintf("V = %i, %X\r\n", v, v);
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
  bool format_csv = true;
  try {
    char* arg = GetArgument(1);
    format_csv = false; //format for console view if user provided an arg.
  }
  catch (int e) {
  }
  
  binPrintInfo(format_csv);
}

#if 0
// Cozmo doesn't know how to do this, yet
static void Charge(void)
{
  const u32 sampleCount = 100;
  u32 now = getMicroCounter();
  u32 delay = 10000000 / sampleCount;
  
  TestEnableTx();
  for (u32 i = 0; i < sampleCount; i++)
  {
    MicroWait(delay);
    TestCurrent();
  }
  TestEnableRx();
}
#endif

void CubeBurn(void);

void SendTestMessage(void)
{
  int test = 0;
  char* arg = GetArgument(1);  
  sscanf(arg, "%i", &test);
  SendCommand(test, 0, 0, 0);
}

void SetRadio(void)
{  
  char* arg = GetArgument(1);
  ConsolePrintf("Remember to use uppercase for most modes\r\n");
  SetRadioMode(arg[0]);
}

void SetMotor(void)
{
  int test = 0;
  char* arg = GetArgument(1);  
  sscanf(arg, "%i", &test);
  MotorMV(test);
}  

// Set the cube test threshold in milliamps (usually 5-15)
void SetCubeCurrent(int deltaMA);
void SetCubeTest(void)
{
  int test = 0;
  char* arg = GetArgument(1);  
  sscanf(arg, "%i", &test);
  SetCubeCurrent(test);
}  

// Re-run a test
void Again(void)
{
  g_isDevicePresent = false;  // Virtually remove device
}

void HeadESP();
void CubePOST(void);
void DropSensor();
void AllowOutdated();

static CommandFunction m_functions[] =
{
  {"Again", Again, FALSE},
  {"AllowOutdated", AllowOutdated, FALSE},
  {"Drop", DropSensor, FALSE},
  {"GetSerial", GetSerialCmd, FALSE},
  {"BinVersion", BinVersionCmd, FALSE},
  {"RedoTest", RedoTest, FALSE},
  {"SetDateCode", SetDateCode, FALSE},
  {"SetLotCode", SetLotCode, FALSE},
  {"SetMode", SetMode, FALSE},
  {"SetSerial", SetSerial, FALSE},
  {"SetTime", SetTime, FALSE},
  {"Current", TestCurrent, FALSE},
  {"DumpFixtureSerials", DumpFixtureSerials, FALSE},
  {"Voltage", TestVoltage, FALSE},
  {"CubePOST", CubePOST, FALSE},
  {"Send", SendTestMessage, FALSE},
  {"HeadESP", HeadESP, FALSE},
  {"SetMotor", SetMotor, FALSE},
  {"SetRadio", SetRadio, FALSE},
  {"SetCubeTest", SetCubeTest, FALSE},
};

static void ParseCommand(void)
{
  u32 i;
  char* buffer = m_parseBuffer;
  
  if(!strcasecmp(buffer, "reset"))
  {
    ConsoleWrite((char*)"\r\n");
    MicroWait(10000);
    
    //save console state and issue soft reset
    g_app_reset.console.isInConsoleMode = 1;
    nvReset((u8*)&g_app_reset, sizeof(g_app_reset));
  }
  else if (!strcasecmp(buffer, "exit"))
  {
    m_isInConsoleMode = 0;
    return;
  }
  else if (!strcasecmp(buffer, "help") || !strcasecmp(buffer, "?")) 
  {
    ConsoleWrite((char*)"Available commands:\r\n");
    for (i = 0; i < sizeof(m_functions) / sizeof(CommandFunction); i++)
    {
      ConsoleWrite((char*)"  ");
      ConsoleWrite((char*)m_functions[i].command);
      ConsoleWrite((char*)"\r\n");
    }
    ConsoleWrite((char*)"  reset\r\n");
    ConsoleWrite((char*)"  exit\r\n");
    ConsoleWrite((char*)"\r\n");
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
    
    BOOL commandFound = 0;
    for (i = 0; i < sizeof(m_functions) / sizeof(CommandFunction); i++)
    {
      CommandFunction* cf = &m_functions[i];
      if (!strcasecmp(cf->command, buffer) && cf->function)
      {
        commandFound = 1;
        if (cf->doesCommunicateWithRobot && !g_isDevicePresent)
        {
          ConsolePrintf("No device present\r\n");
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
          
          ConsolePrintf("status,%i\r\n", error);
        }
        
        break;
      }
    }
    
    if (!commandFound && strcmp(buffer, ""))
    {
      ConsolePrintf("Unknown command: %s\r\n", buffer);
    }
  }
}

void ConsoleUpdate(void)
{
  //if (!(UART4->SR & USART_FLAG_RXNE))
  //  return;

  u32 start = getMicroCounter();
  
  // Keep allowing additional characters until timeout
  // This allows top speed communication with a PC
  //while (getMicroCounter() - start < CONSOLE_TIMEOUT)
  {
    int value = ConsoleReadChar();
    if (value < 0)
      return;
    
    // Clear any pending errors when reading the data
    volatile u32 v = UART4->SR;
    v = UART4->DR;  // NDM: Why do we need this with DMA?
    
    char c = value;

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
        ConsoleWrite("<ANKI>\r\n");
        m_index = 0;
      } else if (c == 0x7F || c == 8) {  // Check for delete or backspace keys
        if( m_index > 0 ) {
            ConsolePutChar(c);      //overwrite the onscreen char with a space
            ConsolePutChar(' ');    //"
            ConsolePutChar(c);      //"
            m_parseBuffer[--m_index] = 0;
        }
      } else if (c == '\r' || c == '\n') {
        ConsoleWrite("\r\n");
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
        ConsoleWrite("<ANKI>\r\n");
      } else {
        if (!m_index &&  c != (SAFE_PLATFORM_TAG_A & 0xFF))
        {
          return;
        }
        
        u8* buffer = GetGlobalBuffer();
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
}
#endif
