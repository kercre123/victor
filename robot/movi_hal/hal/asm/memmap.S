!==============================================================================
!= SABRE ======================================================================
!==============================================================================

! LHB 
.set	__LROM,			0x90000000
.set	__LRAM,			0x90100000
.set	__CMX_PHYS,		0xA0000000
.set	__CMX_PAGED,		0xA4000000
.set	__CMX_BCAST,		0xA8000000
.set	__CMX_CTL,		0xAE000000
.set	__DSU,			0xC0000000
.set	__MPEGTS,		0xC1400000
!.set	__CABAC,		0xC1600000
.global __LROM, __LRAM, __CMX_PHYS, __CMX_PAGED, __CMX_BCAST, __CMX_CTL, __DSU
.global __MPEGTS !, __CABAC

! SHB 
.set	__SDIO,			0xD0000000
.set	__SDH1,			0xD0100000
.set	__SDH2,			0xD0200000
.set	__USB,			0xD0300000
.set	__NFC,			0xD0400000
.global __SDIO, __SDH1, __SDH1, __USB, __NFC

! LPB1 
.set	__UART,			0x80000000
.set	__ICB,			0x80010000
.set	__TIM,			0x80020000
.set	__CPR,			0x80030000
.set	__GPIO,			0x80040000
.set	__SPI1,			0x80050000
.set	__SPI2,			0x80060000
.set	__SPI3,			0x80070000
.set	__AHBUART,		0x80080000
.set	__DDRC_CTL,		0x80090000
.set	__I2C1,			0x800A0000
.set	__I2C2,			0x800B0000
.set	__I2S1,			0x800C0000
.set	__I2S2,			0x800D0000
.set	__I2S3,			0x800E0000
.set	__L2CACHE_CTL,		0x800F0000
.global __UART, __ICB, __TIM, __CPR, __GPIO, __SPI1, __SPI2, __SPI3
.global __AHBUART, __DDRC_CTL, __I2C1, __I2C2, __I2S1, __I2S2, __I2S3
.global __L2CACHE_CTL

! LPB2 
.set	__KEYPAD,		0x80100000
.set	__TSC,			0x80110000
.set	__AXIMON,		0x80120000
.set	__SHAVE0,		0x80140000
.set	__SHAVE1,		0x80150000
.set	__SHAVE2,		0x80160000
.set	__SHAVE3,		0x80170000
.set	__SHAVE4,		0x80180000
.set	__SHAVE5,		0x80190000
.set	__SHAVE6,		0x801a0000
.set	__SHAVE7,		0x801b0000
!.set	__TVENC,		0x801c0000
.set	__GPS,			0x801d0000
.global __KEYPAD, __TSC, __AXIMON, __SHAVE0, __SHAVE1, __SHAVE2, __SHAVE3
.global __SHAVE4, __SHAVE5, __SHAVE6, __SHAVE7, __GPS !, __TVENC

! MXI 
.set	__SEBI,			0x08000000
.set	__CMX_PHYS_MXI,		0x10000000
.set	__CMX_PAGED_MXI,	0x14000000
.set	__CMX_BCAST_MXI,	0x18000000
.set	__MEBI,			0x20000000
.set	__CIF1,			0x30000000
.set	__CIF2,			0x38000000
.set	__LCD1,			0x50000000
.set	__LCD2,			0x58000000
.global __SEBI, __CMX_PHYS_MXI, __CMX_PAGED_MXI, __CMX_BCAST_MXI, __MEBI
.global	__CIF1, __CIF2,	__LCD1, __LCD2

! SXI 
.set	__L2CACHE,		0x40000000
.global	__L2CACHE
.set	__DDR,		0x48000000
.global	__DDR

  ! VCS Testbench 
  .set	__VCS, 		0x90008000
  .global __VCS

! Control stuff
!.set    __CMX_CTRL,    0xAE000020
!.global	__CMX_CTRL
!.set    __L2_MODE,     0x800F0000
!.global	__L2_MODE

!;-----------------------------------------------------------------------------

.set	LROM, __LROM
.set	LRAM, __LRAM
.set	CMX_PHYS, __CMX_PHYS
.set	CMX_PAGED, __CMX_PAGED
.set	CMX_BCAST, __CMX_BCAST
.set	CMX_CTL, __CMX_CTL
.set	DSU, __DSU
.set	MPEGTS, __MPEGTS
!.set	CABAC, __CABAC
.set	SDIO, __SDIO
.set	SDH1, __SDH1
.set	SDH2, __SDH2
.set	USB, __USB
.set	NFC, __NFC
.set	UART, __UART
.set	ICB, __ICB
.set	TIM, __TIM
.set	CPR, __CPR
.set	GPIO, __GPIO
.set	SPI1, __SPI1
.set	SPI2, __SPI2
.set	SPI3, __SPI3
.set	AHBUART, __AHBUART
.set	DDRC_CTL, __DDRC_CTL
.set	I2C1, __I2C1
.set	I2C2, __I2C2
.set	I2S1, __I2S1
.set	I2S2, __I2S2
.set	I2S3, __I2S3
.set	L2CACHE_CTL, __L2CACHE_CTL
.set	KEYPAD, __KEYPAD
.set	TSC, __TSC
.set	AXIMON, __AXIMON
.set	SHAVE0, __SHAVE0
.set	SHAVE1, __SHAVE1
.set	SHAVE2, __SHAVE2
.set	SHAVE3, __SHAVE3
.set	SHAVE4, __SHAVE4
.set	SHAVE5, __SHAVE5
.set	SHAVE6, __SHAVE6
.set	SHAVE7, __SHAVE7
! .set	TVENC, __TVENC
.set	GPS, __GPS
.set	SEBI, __SEBI
.set	CMX_PHYS_MXI, __CMX_PHYS_MXI
.set	CMX_PAGED_MXI, __CMX_PAGED_MXI
.set	CMX_BCAST_MXI, __CMX_BCAST_MXI
.set	MEBI, __MEBI
.set	CIF1, __CIF1
.set	CIF2, __CIF2
.set	LCD1, __LCD1
.set	LCD2, __LCD2
.set	L2CACHE, __L2CACHE
.set	DDR, __DDR
  .set	VCS, 		0x90008000
.weak LROM, LRAM, CMX_PHYS, CMX_PAGED, CMX_BCAST, CMX_CTL, DSU, MPEGTS !, CABAC
.weak SDIO, SDH1, SDH2, USB, NFC, UART, ICB, TIM, CPR, GPIO, SPI1, SPI2, SPI3
.weak AHBUART, DDRC_CTL, I2C1, I2C2, I2S1, I2S2, I2S3, L2CACHE_CTL, KEYPAD
.weak TSC, AXIMON, SHAVE0, SHAVE1, SHAVE2, SHAVE3, SHAVE4, SHAVE5, SHAVE6
.weak SHAVE7, GPS, L2CACHE, DDR, SEBI, CMX_PHYS_MXI, CMX_PAGED_MXI ! , TVENC
.weak CMX_BCAST_MXI, MEBI, CIF1, CIF2, LCD1, LCD2
  .weak VCS
