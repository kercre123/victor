// Based on Drive Testfix, updated for Cozmo Testfix
#include <assert.h>
#include "hal/board.h"
#include "hal/portable.h"
#include "hal/timers.h"

GPIO_TypeDef* GPIO_PORT[LEDn] = {LED1_GPIO_PORT, LED2_GPIO_PORT};
const uint16_t GPIO_PIN[LEDn] = {LED1_PIN, LED2_PIN};
const uint32_t GPIO_CLK[LEDn] = {LED1_GPIO_CLK, LED2_GPIO_CLK};

/**
  * @brief  Configures LED GPIO.
  * @param  Led: Specifies the Led to be configured. 
  * @retval None
  */
void STM_EVAL_LEDInit(Led_TypeDef Led)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
  /* Enable the GPIO_LED Clock */
  RCC_AHB1PeriphClockCmd(GPIO_CLK[Led], ENABLE);

  /* Configure the GPIO_LED pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_PIN[Led];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_PORT[Led], &GPIO_InitStructure);
  
  STM_EVAL_LEDOff(Led);
}

/**
  * @brief  Turns selected LED On.
  * @param  Led: Specifies the Led to be set on. 
  * @retval None
  */
void STM_EVAL_LEDOff(Led_TypeDef Led)
{
  GPIO_PORT[Led]->BSRRL = GPIO_PIN[Led];
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: Specifies the Led to be set off. 
  * @retval None
  */
void STM_EVAL_LEDOn(Led_TypeDef Led)
{
  GPIO_PORT[Led]->BSRRH = GPIO_PIN[Led];
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: Specifies the Led to be toggled. 
  * @retval None
  */
void STM_EVAL_LEDToggle(Led_TypeDef Led)
{
  GPIO_PORT[Led]->ODR ^= GPIO_PIN[Led];
}

void InitBoard(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_StructInit( &GPIO_InitStructure ); //set struct defaults
   
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  
  /* Initialize LEDs */
  STM_EVAL_LEDInit(LEDRED);
  STM_EVAL_LEDInit(LEDGREEN);
  STM_EVAL_LEDOff(LEDRED);
  STM_EVAL_LEDOff(LEDGREEN);
  
  // Always enable charger/ENCHG - this is the only way to turn off cube power
  GPIO_SetBits(GPIOA, GPIOA_ENCHG);
  GPIO_InitStructure.GPIO_Pin = GPIOA_ENCHG;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  //pins PB12-PB15 are multi-use (TBD hardware rev). Pull-down is idle state for all revs.
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  MicroWait(100);

  // PINC_CHGTX - default low (VEXT disabled)
  PIN_RESET(GPIOC, PINC_CHGTX);
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = PINC_CHGTX;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
  
  // ENBAT_LC, ENBAT
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  // NBATSINK
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  //Piezo Buzzer output
  PIN_RESET(GPIOC, PINC_BUZZER);
  PIN_OUT(GPIOC, PINC_BUZZER);
  
  DisableBAT();
}

enum pinstate_e { HIGH, LOW, Z };
static pinstate_e _test_pin_state(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
  pinstate_e state;
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit( &GPIO_InitStructure ); //set struct defaults
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin;
  
  //test value with internal pull-up
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOx, &GPIO_InitStructure);
  MicroWait(100);
  u8 pu_val = GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
  
  //test value with internal pull-down
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_Init(GPIOx, &GPIO_InitStructure);
  MicroWait(100);
  u8 pd_val = GPIO_ReadInputDataBit(GPIOx, GPIO_Pin);
  
  if( !pu_val && !pd_val )    //pulled low externally
    state = LOW;
  else if( pu_val && pd_val ) //pulled high exeternally
    state = HIGH;
  else if( pu_val && !pd_val ) //floating
    state = Z;
  else //if( !pu_val && pd_val ) //bizarro world?
    assert( false );
  
  //if we're not floating, remove pin pull to save power
  if( state != Z ) {
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
  }
  
  return state;
}

board_rev_t GetBoardRev(void)
{
  pinstate_e board_id0 = _test_pin_state(GPIOC, GPIOC_BOARD_ID0);
  pinstate_e board_id1 = _test_pin_state(GPIOC, GPIOC_BOARD_ID1);
  
  //fixture 1.0 rev1,2,3 did not have a board revision check. pins are NC/float.
  //fixture 1.5+ implemented this check
  if( board_id0 == Z )
    return BOARD_REV_1_0_REV3; //or rev1,2. Can't tell.
  else if( board_id0 == LOW )
    return BOARD_REV_1_5_0;
  else //no revision currently pulls this pin high.
    assert(0);
  
  //additional ID pins are NC/float. crash and burn if firmware is older than fixture hardware.
  assert( board_id1 == Z );
  
  return BOARD_REV_1_0_REV1; //silly compiler. we'll never make it here.
}

static const char board_rev_str_0[] = "1.0.r{1,2,3}";
static const char board_rev_str_3[] = "1.5.0";
static const char board_rev_str_x[] = "?";
char* GetBoardRevStr(void) 
{
  char* s;
  switch( GetBoardRev() ) {
    case BOARD_REV_1_0_REV1:
    case BOARD_REV_1_0_REV2:
    case BOARD_REV_1_0_REV3:  s = (char*)&board_rev_str_0[0]; break;
    case BOARD_REV_1_5_0:     s = (char*)&board_rev_str_3[0]; break;
    default:                  s = (char*)&board_rev_str_x[0]; break;
  }
  return s;
}

u8 GetBoardID(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_StructInit( &GPIO_InitStructure ); //set struct defaults
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  
  //Enable pull-ups for ID input pins PB12-PB15
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  MicroWait(100);
  
  //read id value, ~GPIOB[15:12] - bits 'set' by external pull-down resistors
  int id = (~(GPIO_READ(GPIOB) >> 12)) & 15;
  
  //Float the ID pins; they have alternate functionality on newer hardware
  //GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  //GPIO_Init(GPIOB, &GPIO_InitStructure);
  //MicroWait(100);
  
  return id;
}

void EnableVEXT(void)
{
  PIN_SET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);
}

void DisableVEXT(void)
{
  PIN_RESET(GPIOC, PINC_CHGTX);
  PIN_OUT(GPIOC, PINC_CHGTX);  
}

static u8 isEnabled = 1;
void EnableBAT(void)
{  
  GPIO_SetBits(GPIOD, GPIO_Pin_2);    // Disable sink (to prevent blowing up the fixture)
  GPIO_ResetBits(GPIOC, GPIO_Pin_2);
  GPIO_SetBits(GPIOA, GPIOA_ENCHG);
  isEnabled = 1;
}

void DisableBAT(void)
{
  if (isEnabled)
  {
    GPIO_SetBits(GPIOC, GPIO_Pin_2);
    GPIO_SetBits(GPIOA, GPIO_Pin_15);
    GPIO_ResetBits(GPIOA, GPIOA_ENCHG);
    MicroWait(1);
    GPIO_ResetBits(GPIOD, GPIO_Pin_2);  // Enable sink to quickly discharge any remaining power
    GPIO_ResetBits(GPIOC, GPIO_Pin_1);  // Sink even more current (down to 0.3V at least)
    MicroWait(50000);
    GPIO_SetBits(GPIOD, GPIO_Pin_2);    // Disable sink (to prevent blowing up the fixture)  
    GPIO_SetBits(GPIOC, GPIO_Pin_1);
  }
  isEnabled = 0;
}

void Buzzer(u8 f_kHz, u16 duration_ms)
{
  u32 half_period_us = f_kHz > 0 && f_kHz <= 20 ? (1000/2)/f_kHz : 125 /*4kHz*/; //half-period in us for delay loop
  
  PIN_RESET(GPIOC, PINC_BUZZER);
  PIN_OUT(GPIOC, PINC_BUZZER);
  
  u32 sqw_start = getMicroCounter();
  while( getMicroCounter() - sqw_start < duration_ms*1000 )
  {
    PIN_SET(GPIOC, PINC_BUZZER);
    MicroWait( half_period_us );
    PIN_RESET(GPIOC, PINC_BUZZER);
    MicroWait( half_period_us );
  }
  
  PIN_RESET(GPIOC, PINC_BUZZER); //idle low
}

#if 0
extern u8 g_fixbootbin[], g_fixbootend[];

// Check if bootloader is outdated and update it with the latest
// This is stupidly risky and could easily brick a board - but it replaces an old bootloader that bricks boards
// Fight bricking with bricking!
void UpdateBootLoader(void)
{
  // Save the serial number, in case we have to restore it
  u32 serial = FIXTURE_SERIAL;
  
  // Spend a little while checking - no point in giving up on our first try, since the board will die if we do
  for (int i = 0; i < 100; i++)
  {
    // Byte by byte comparison
    bool matches = true;
    for (int j = 0; j < (g_fixbootend - g_fixbootbin); j++)
      if (g_fixbootbin[j] != ((u8*)FLASH_BOOTLOADER)[j])
        matches = false;
      
    // Bail out if all is good
    if (matches)
      return;
    
    SlowPutString("Mismatch!\r\n");
    
    // If not so good, check a few more times, leaving time for voltage to stabilize
    if (i > 50)
    {
      SlowPutString("Flashing...");
      
      // Gulp.. it's bricking time
      FLASH_Unlock();
      
      // Erase and reflash the boot code
      FLASH_EraseSector(FLASH_BLOCK_BOOT, VoltageRange_1);
      for (int j = 0; j < (g_fixbootend - g_fixbootbin); j++)
        FLASH_ProgramByte(FLASH_BOOTLOADER + j, g_fixbootbin[j]);
      
      // Recover the serial number
      FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL, serial & 255);
      FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+1, serial >> 8);
      FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+2, serial >> 16);
      FLASH_ProgramByte(FLASH_BOOTLOADER_SERIAL+3, serial >> 24);
      FLASH_Lock();
      
      SlowPutString("Done!\r\n");
    }
  }
  
  // If we get here, we are DEAD!
}
#endif

