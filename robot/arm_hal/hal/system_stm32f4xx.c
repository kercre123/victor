#include "stm32f4xx.h"

/*!< Uncomment the following line if you need to relocate your vector Table in
     Internal SRAM. */
/* #define VECT_TAB_SRAM */
#define VECT_TAB_OFFSET  0x0

// Uncomment the following to put Cozmo on overclocking steroids (180MHz)
#define COZMOROIDS

/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
#define PLL_M      16
#ifdef COZMOROIDS
#define PLL_N      360   // 360/2 = 180MHz
#else
#define PLL_N      336    // 336/2 = 168MHz
#endif

/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P      2

/* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
#define PLL_Q      7

uint32_t SystemCoreClock = 180000000;

static void SetSysClock(void);
static void SystemInit_ExtMemCtl(void); 

static void SetAFBitmask(GPIO_TypeDef* gpio, u32 bitmask)
{
  int i;
  for (i = 0; i < 16; i++)
  {
    if (bitmask & (1 << i))
    {
       GPIO_PinAFConfig(gpio, i, GPIO_AF_FMC);
    }
  }
}

#define BIT(a, x) ((a >> x) & 1)

// This function takes the value the RAM needs to see and swizzles it according
// to how the RAM is actually mapped to the MCU.
static u16 GetSwizzledAddress(u16 a)
{
  return (BIT(a, 1) << 0) |
    (BIT(a, 4) << 1) |
    (BIT(a, 2) << 2) |
    (BIT(a, 3) << 3) |  // Unchanged
    (BIT(a, 0) << 4) |
    (BIT(a, 6) << 5) |
    (BIT(a, 7) << 6) |
    (BIT(a, 5) << 7) |
    (BIT(a, 9) << 8) |
    (BIT(a, 8) << 9) |
    (BIT(a, 10) << 10) |  // Unchanged
    (BIT(a, 12) << 11) |
    (BIT(a, 11) << 12);
}

void SystemInit(void)
{
  /* FPU settings ------------------------------------------------------------*/
  SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
  // Should flush the pipeline here, but ST didn't wanna (and it'll be a while before an FP op)
  
  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  /* Reset PLLCFGR register */
  // 180 MHz from 16 MHz HSI == PLL @ 360 / 32
  RCC->PLLCFGR = 0x24002D20;
  
  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000;

  // Initialize external memory
  SystemInit_ExtMemCtl(); 
 
  /* Configure the System clock source, PLL Multiplier and Divider factors, 
     AHB/APBx prescalers and Flash settings ----------------------------------*/
  SetSysClock();

  /* Configure the Vector Table location add offset address ------------------*/
#ifdef VECT_TAB_SRAM
  SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal SRAM */
#else
  SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal FLASH */
#endif
}

/**
  * @brief  Configures the System clock source, PLL Multiplier and Divider factors, 
  *         AHB/APBx prescalers and Flash settings
  * @Note   This function should be called only once the RCC clock configuration  
  *         is reset to the default reset state (done in SystemInit() function).   
  */
static void SetSysClock(void)
{  
    /* Configure the main PLL */
    RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16) |
                   (RCC_PLLCFGR_PLLSRC_HSI) | (PLL_Q << 24);

    /* Select regulator voltage output Scale 1 mode, System frequency up to 168 MHz (180 in Overdrive) */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS;

    /* HCLK = SYSCLK / 1*/
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
      
    /* PCLK2 = HCLK / 2*/
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;
    
    /* PCLK1 = HCLK / 4*/
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

    /* Enable the main PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till the main PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* Enable the Over-drive to extend the clock frequency to 180 Mhz */
    // Overdrive mode is unsupported at 1V8 (as in Cozmo 2.1+)
#ifdef COZMOROIDS
    PWR->CR |= PWR_CR_ODEN;     // First enable overdrive
    while((PWR->CSR & PWR_CSR_ODRDY) == 0)
    {
    }
    PWR->CR |= PWR_CR_ODSWEN;   // Then switch to it
    while((PWR->CSR & PWR_CSR_ODSWRDY) == 0)
    {
    } 
#endif
    
    /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
    // Updated to 8 wait states for 2.1 (1V8)
    FLASH->ACR = FLASH_ACR_PRFTEN | FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_8WS;

    /* Select the main PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    /* Wait till the main PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL);
    {
    }
}

/**
  * Setup the external memory controller.
  */
void SystemInit_ExtMemCtl(void)
{
  register uint32_t tmpreg = 0, timeout = 0xFFFF;
  register uint32_t index;
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH and GPIOI interface 
      clock */
  RCC->AHB1ENR |= 0x000001FC;

  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

  // GPIOC
  GPIO_InitStructure.GPIO_Pin = (1 << 2);
  SetAFBitmask(GPIOC, GPIO_InitStructure.GPIO_Pin);
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  // GPIOD
  GPIO_InitStructure.GPIO_Pin = 
    (1 << 0) | (1 << 1) | (1 << 8) | (1 << 9) |
    (1 << 10) | (1 << 14) | (1 << 15);
  SetAFBitmask(GPIOD, GPIO_InitStructure.GPIO_Pin);
  GPIO_Init(GPIOD, &GPIO_InitStructure);
  
  // GPIOE
  GPIO_InitStructure.GPIO_Pin = 
    (1 << 0) | (1 << 1) | (1 << 7) | (1 << 8) | 
    (1 << 9) | (1 << 10) | (1 << 11) | (1 << 12) | 
    (1 << 13) | (1 << 14) | (1 << 15);
  SetAFBitmask(GPIOE, GPIO_InitStructure.GPIO_Pin);
  GPIO_Init(GPIOE, &GPIO_InitStructure);
  
  // GPIOF
  GPIO_InitStructure.GPIO_Pin = 
    (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) |
    (1 << 4) | (1 << 5) | (1 << 11) | (1 << 12) |
    (1 << 13) | (1 << 14) | (1 << 15);
  SetAFBitmask(GPIOF, GPIO_InitStructure.GPIO_Pin);
  GPIO_Init(GPIOF, &GPIO_InitStructure);
  
  // GPIOG
  GPIO_InitStructure.GPIO_Pin = 
    (1 << 0) | (1 << 1) | (1 << 2) | (1 << 4) | (1 << 5) | 
    (1 << 8) | (1 << 15);
  SetAFBitmask(GPIOG, GPIO_InitStructure.GPIO_Pin);
  GPIO_Init(GPIOG, &GPIO_InitStructure);
  
  // GPIOH
  GPIO_InitStructure.GPIO_Pin = 
    (1 << 2) | (1 << 5);
  SetAFBitmask(GPIOH, GPIO_InitStructure.GPIO_Pin);
  GPIO_Init(GPIOH, &GPIO_InitStructure);
  
/*-- FMC Configuration ------------------------------------------------------*/
  /* Enable the FMC interface clock */
  RCC->AHB3ENR |= 0x00000001;
  
  /* Configure and enable SDRAM bank1 */
  FMC_Bank5_6->SDCR[0] = 0x0000295A;  // 16-bit | 4 banks | 2 cycle CAS | SDCLK = 2 x HCLK | no burst read | read pipe = 1 cycle | 13 bits for row | 10 bits for column
  FMC_Bank5_6->SDTR[0] = 0x01115351;  // timings
  
  /* SDRAM initialization sequence */
  /* Clock enable command */
  FMC_Bank5_6->SDCMR = 0x00000011; 
  tmpreg = FMC_Bank5_6->SDSR & 0x00000020; 
  while((tmpreg != 0) && (timeout-- > 0))
  {
    tmpreg = FMC_Bank5_6->SDSR & 0x00000020; 
  }

  /* Delay */
  for (index = 0; index<1000; index++);
  
  /* PALL command */
  FMC_Bank5_6->SDCMR = 0x00000012;           
  timeout = 0xFFFF;
  while((tmpreg != 0) && (timeout-- > 0))
  {
    tmpreg = FMC_Bank5_6->SDSR & 0x00000020; 
  }
  
  /* Auto refresh command */
  FMC_Bank5_6->SDCMR = 0x00000073;
  timeout = 0xFFFF;
  while((tmpreg != 0) && (timeout-- > 0))
  {
    tmpreg = FMC_Bank5_6->SDSR & 0x00000020; 
  }
 
  /* MRD register program */
  FMC_Bank5_6->SDCMR =
    (GetSwizzledAddress(0x220) << 9) | // Burst length = 1 | Sequential | CAS# latency = 2 | No test | Write burst length = single
    0x14;  // Load Mode Register | Command issued to SDRAM Bank 1
  timeout = 0xFFFF;
  while((tmpreg != 0) && (timeout-- > 0))
  {
    tmpreg = FMC_Bank5_6->SDSR & 0x00000020; 
  }
  
  /* Set refresh count */
  tmpreg = FMC_Bank5_6->SDRTR;
  FMC_Bank5_6->SDRTR = (tmpreg | (0x0000027C<<1));
  
  /* Disable write protection */
  tmpreg = FMC_Bank5_6->SDCR[0]; 
  FMC_Bank5_6->SDCR[0] = (tmpreg & 0xFFFFFDFF);
}
