// Based on Drive Testfix, updated for Cozmo EP1 Testfix
#include "hal/console.h"
#include "hal/monitor.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/flash.h"
#include "hal/uart.h"
#include "../../crypto/crypto.h"
#include "../app/fixture.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Which character to escape into command code
#define ESCAPE_CODE 27

#define CONSOLE_TIMEOUT 50000   // Allow 50ms of typing before dropping out of command mode

#define BAUD_RATE   1000000

extern BOOL g_isDevicePresent;
extern u8 g_modelIndex;
extern u32 g_modelIDs[8];
extern FixtureType g_fixtureType;

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

void ConsolePrintf(const char* format, ...)
{
  char dest[64];
  va_list argptr;
  va_start(argptr, format);
  vsnprintf(dest, 64, format, argptr);
  va_end(argptr);
  ConsoleWrite(dest);
}

void ConsolePutChar(char c)
{
  UART4->DR = c;
  while (!(UART4->SR & USART_FLAG_TXE))
    ;
}

void ConsoleWrite(char* s)
{
  while (*s)
  {
    UART4->DR = *s++;
    while (!(UART4->SR & USART_FLAG_TXE))
      ;
  }
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

void ConsoleWriteHex(u8* buffer, u32 numberOfBytes)
{
  u32 i, j;
  u32 numberOfRows = (numberOfBytes + 15) >> 4;
  for (i = 0; i < numberOfRows; i++)
  {
    u32 index = i << 4;
    ConsolePrintf("%04x: ", i << 4);
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
  
  if (!strcasecmp(arg, "body"))
  {
    g_fixtureType = FIXTURE_BODY_TEST;
  } else if (!strcasecmp(arg, "head")) {
    g_fixtureType = FIXTURE_HEAD_TEST;
  } else if (!strcasecmp(arg, "charge")) {
    g_fixtureType = FIXTURE_CHARGER_TEST;
  } else if (!strcasecmp(arg, "cube")) {
    g_fixtureType = FIXTURE_CUBE_TEST;
  } else if (!strcasecmp(arg, "debug")) {
    g_fixtureType = FIXTURE_DEBUG;
  } else {
    throw ERROR_UNKNOWN_MODE;
  }
  
  SetFixtureText();
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
  
  __disable_irq();
  FLASH_Unlock();
  FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL, serial & 255);
  FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+1, serial >> 8);
  FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+2, serial >> 16);
  FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+3, serial >> 24);
  FLASH_Lock();
  __enable_irq();
}

const char* FIXTYPES[] = FIXTURE_TYPES;
extern int g_canary;
static void GetSerial(void)
{
  // Serial number, fixture type, build version
  ConsolePrintf("serial,%i,%s,%i\r\n", 
    FIXTURE_SERIAL, 
    g_fixtureType & FIXTURE_DEBUG ? "DEBUG" : FIXTYPES[g_fixtureType],
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

static CommandFunction m_functions[] =
{
  {"Charge", Charge, FALSE},
  {"GetSerial", GetSerial, FALSE},
  {"RedoTest", RedoTest, FALSE},
  {"SetDateCode", SetDateCode, FALSE},
  {"SetLotCode", SetLotCode, FALSE},
  {"SetMode", SetMode, FALSE},
  {"SetSerial", SetSerial, FALSE},
  {"SetTime", SetTime, FALSE},
  {"Current", TestCurrent, FALSE},
  {"DumpFixtureSerials", DumpFixtureSerials, FALSE},
  {"Voltage", TestVoltage, FALSE},
};

static void ParseCommand(void)
{
  u32 i;
  char* buffer = m_parseBuffer;
  if (!strcasecmp(buffer, "exit"))
  {
    m_isInConsoleMode = 0;
    return;
  } else if (!strcasecmp(buffer, "help") || !strcasecmp(buffer, "?")) {
    ConsoleWrite("Available commands:\r\n");
    for (i = 0; i < sizeof(m_functions) / sizeof(CommandFunction); i++)
    {
      ConsoleWrite((char*)m_functions[i].command);
      ConsoleWrite("\r\n");
    }
    ConsoleWrite("\r\n");
  } else {
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
      // Echo legal ASCII back to the console
      if (c >= 32 && c <= 126)
      {
        ConsolePutChar(c);
        // Prevent overflows...
        if (m_index + 1 >= sizeof(m_parseBuffer))
        {
          m_index = 0;
        }
        m_parseBuffer[m_index++] = c;
      } else if (c == 27) {  // Check for escape
        ConsoleWrite("<ANKI>\r\n");
        m_index = 0;
      } else if (c == 0x7F || c == 8) {  // Check for delete
        ConsolePutChar(c);
        m_index -= 1;
        if (m_index < 0)
        {
          m_index = 0;
        }
        m_parseBuffer[m_index] = 0;
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
          //SlowPutString("RECEIVING FILE\r\n");
          m_isReceivingSafe = 1;
          FLASH_Unlock();
          // Why voltage range 2?  The handshake on this erase in old AnkiLog has an early timeout - _1 is not fast enough
          // I'm assuming the dreaded "voltage dip" issue we saw in the bootloader is related to OLED inrush and is well past
          FLASH_EraseSector(FLASH_BLOCK_B_SECTOR_0, VoltageRange_2);
          FLASH_EraseSector(FLASH_BLOCK_B_SECTOR_1, VoltageRange_2);
          FLASH_EraseSector(FLASH_BLOCK_B_SECTOR_2, VoltageRange_2);
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
