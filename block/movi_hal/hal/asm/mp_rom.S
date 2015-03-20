#ifndef NO_ROM
.set Shut_Down_Core, 0x9000a750
.global Shut_Down_Core
.set p_uh_queuehead_async2, 0x90107624
.global p_uh_queuehead_async2
.set __nalu_ref_idc, 0x90107fc0
.global __nalu_ref_idc
.set strcpy, 0x90008988
.global strcpy
.set __USB_SetQueueHeadWithEndPointInfo, 0x90002cbc
.global __USB_SetQueueHeadWithEndPointInfo
.set __AES_svu_done, 0x90003890
.global __AES_svu_done
!;.set fabs, 0x90008abc // replaced by in-mdk fabs/fabsf
!;.global fabs
!; log doesn't work correctly:
!; .set log, 0x90008b3c
!; .global log
.set __boot_main, 0x90000270
.global __boot_main
!;.set sqrt, 0x90008b28 // replaced by in-mdk sqrt/sqrtf
!;.global sqrt
.set __boot_setup_SEBI_boot_device, 0x90001080
.global __boot_setup_SEBI_boot_device
.set __bootcfg_build_no, 0x90000054
.global __bootcfg_build_no
.set __sdio_tpl_CIS0VERS_MV, 0x90007700
.global __sdio_tpl_CIS0VERS_MV
.set Shutdown_Island, 0x9000a2c0
.global Shutdown_Island
.set __CPR_extract_otp_data, 0x9000221c
.global __CPR_extract_otp_data
.set __boot_aes_decrypt_fn, 0x90107d9c
.global __boot_aes_decrypt_fn
.set __SHAVE_dataCoefCAVLCh264, 0x9000b790
.global __SHAVE_dataCoefCAVLCh264
.set p_uh_queuetransactiondescriptor_tail, 0x90107630
.global p_uh_queuetransactiondescriptor_tail
.set __pps_UseDefaultScalingMatrix8x8Flag, 0x90107fe4
.global __pps_UseDefaultScalingMatrix8x8Flag
.set __bootdrv_SEBI_setup, 0x90005ab0
.global __bootdrv_SEBI_setup
.set memmove, 0x900088e4
.global memmove
.set __boot_bringup_SXI, 0x9000189c
.global __boot_bringup_SXI
.set p_data_packet, 0x90107668
.global p_data_packet
.set __bootdrv_IICM_getchar, 0x90004ce4
.global __bootdrv_IICM_getchar
.set __AES_svu_setup, 0x9000363c
.global __AES_svu_setup
.set __boot_panic, 0x90000218
.global __boot_panic
.set __USB_device_send, 0x900028b8
.global __USB_device_send
.set _xprintf, 0x900083b4
.global _xprintf
.set __boot_bringup_DXI, 0x900018d4
.global __boot_bringup_DXI
.set __pps_ScalingList4x4, 0x90107fd8
.global __pps_ScalingList4x4
.set __boot_setup_USB_boot_device, 0x900014a0
.global __boot_setup_USB_boot_device
!;.set __GPIO_cfg_commit, 0x90002500
!;.global __GPIO_cfg_commit
.set __bootcfg_build_time, 0x90000050
.global __bootcfg_build_time
.set leon_bit_op, 0x9000a4c0
.global leon_bit_op
.set memcpy, 0x90008904
.global memcpy
.set __aes_leon_round_key, 0x90107dd8
.global __aes_leon_round_key
.set __GPIO_cfg_map, 0x9000266c
.global __GPIO_cfg_map
.set __pps_UseDefaultScalingMatrix4x4Flag, 0x90107fe0
.global __pps_UseDefaultScalingMatrix4x4Flag
.set __boot_iic_miniboot, 0x90107dbc
.global __boot_iic_miniboot
.set __boot_bringup_SHB, 0x90000d40
.global __boot_bringup_SHB
.set __boot_pad_override_or_mask, 0x90107dc4
.global __boot_pad_override_or_mask
.set __bootdrv_MEBI_setup, 0x90004f58
.global __bootdrv_MEBI_setup
.set __SHAVE_ResizeCoef, 0x9000db68
.global __SHAVE_ResizeCoef
.set __bootdrv_UART_getchar, 0x90006340
.global __bootdrv_UART_getchar
.set __bootdrv_IICS_setup, 0x900059cc
.global __bootdrv_IICS_setup
.set H264_decode_SH, 0x9000e47c
.global H264_decode_SH
.set p_uh_queuetransactiondescriptor_async1, 0x90107628
.global p_uh_queuetransactiondescriptor_async1
.set drv_syscfg_sysclk, 0x90107f98
.global drv_syscfg_sysclk
!;.set __GPIO_cfg_init_current, 0x90002598
!;.global __GPIO_cfg_init_current
.set p_control_packet, 0x90107634
.global p_control_packet
.set __bootdrv_IICM8_getchar, 0x90004c44
.global __bootdrv_IICM8_getchar
.set __aes_sbox_invert, 0x90007328
.global __aes_sbox_invert
.set __bootdrv_USBH_setup, 0x900050d0
.global __bootdrv_USBH_setup
.set DDR_ShutDown, 0x90008ef4
.global DDR_ShutDown
.set __boot_setup_SDIO_boot_device_phase1, 0x90001708
.global __boot_setup_SDIO_boot_device_phase1
.set strrchr, 0x90008a14
.global strrchr
.set Sleep_All_Clocks_Off, 0x9000a71c
.global Sleep_All_Clocks_Off
.set __AES_svu_decrypt, 0x90003670
.global __AES_svu_decrypt
.set __aes_leon_state, 0x90107dc8
.global __aes_leon_state
.set g_current_address, 0x90107664
.global g_current_address
.set __aes_sbox, 0x90007428
.global __aes_sbox
.set __libm_constants, 0x9000aeb8
.global __libm_constants
.set _gBulkOutEndpointNum, 0x90107610
.global _gBulkOutEndpointNum
.set __boot_execute, 0x90000110
.global __boot_execute
.set __boot_init, 0x90000228
.global __boot_init
.set strcat, 0x9000893c
.global strcat
.set __boot_device_skip_page_fn, 0x90107db8
.global __boot_device_skip_page_fn
.set __CPR_prepare_otp_read, 0x900024d0
.global __CPR_prepare_otp_read
.set __bootdrv_MEBI_getchar, 0x90004f30
.global __bootdrv_MEBI_getchar
.set modf, 0x90008d50
.global modf
.set H264_decode_SPS, 0x9000df04
.global H264_decode_SPS
.set _gBulkINMaximumPacketLength, 0x9010761c
.global _gBulkINMaximumPacketLength
.set __bootdrv_SPIM_getchar, 0x90004ae4
.global __bootdrv_SPIM_getchar
.set memchr, 0x90008868
.global memchr
.set __bootdrv_SDIO_getchar, 0x90006474
.global __bootdrv_SDIO_getchar
.set __sdio_recv_buffer, 0x90107790
.global __sdio_recv_buffer
.set strstr, 0x90008a7c
.global strstr
.set __nalu_pos, 0x90107fc8
.global __nalu_pos
.set __bootdrv_SDIO_setup_phase1, 0x90006508
.global __bootdrv_SDIO_setup_phase1
.set DDR_ExitStandAloneMode, 0x90008fa4
.global DDR_ExitStandAloneMode
.set __backdoor_ifcfg_modes, 0x900077b0
.global __backdoor_ifcfg_modes
.set strncmp, 0x900088a4
.global strncmp
.set __nalu_len, 0x90107fd4
.global __nalu_len
.set strncpy, 0x9000898c
.global strncpy
.set p_uh_queuehead_async1, 0x90107620
.global p_uh_queuehead_async1
.set __boot_device_putchar_fn, 0x90107db4
.global __boot_device_putchar_fn
!; log10 doesn't work correctly:
!; .set log10, 0x90008cc8
!; .global log10
.set __bootdrv_NAND_skip_page, 0x900058bc
.global __bootdrv_NAND_skip_page
.set p_uh_queuetransactiondescriptor_async2, 0x9010762c
.global p_uh_queuetransactiondescriptor_async2
.set __bootdrv_SPIS_putchar, 0x900058f8
.global __bootdrv_SPIS_putchar
.set __bootdrv_DDR_getchar, 0x900068a8
.global __bootdrv_DDR_getchar
.set memcmp, 0x900088ac
.global memcmp
.set SetLowPowerModeTimeout, 0x90008fe0
.global SetLowPowerModeTimeout
.set strncat, 0x90008940
.global strncat
.set __boot_setup_UART_boot_device, 0x900009f0
.global __boot_setup_UART_boot_device
.set __boot_bringup_MXI, 0x90000fdc
.global __boot_bringup_MXI
.set _gDeviceAddress, 0x90107608
.global _gDeviceAddress
.set p_bulkdata_out, 0x9010766c
.global p_bulkdata_out
.set Set_Wakeup_Source, 0x9000a6ec
.global Set_Wakeup_Source
.set __bootload_mvbin, 0x900018dc
.global __bootload_mvbin
.set memset, 0x90008928
.global memset
.set PowerUp_Device, 0x9000a544
.global PowerUp_Device
.set __AES_leon_setup, 0x9000308c
.global __AES_leon_setup
.set powerup, 0x90000000
.global powerup
.set __bootdrv_NULL_putchar, 0x90004adc
.global __bootdrv_NULL_putchar
.set __const_0d, 0x90007220
.global __const_0d
.set __bootdrv_IICS_putchar, 0x90005990
.global __bootdrv_IICS_putchar
.set __boot_setup_MEBI_boot_device, 0x90001330
.global __boot_setup_MEBI_boot_device
.set Set_Wakeup_GPIO, 0x9000a688
.global Set_Wakeup_GPIO
.set __SHAVE_AAC_table_4, 0x9000afe8
.global __SHAVE_AAC_table_4
.set __CPR_attempt_PLL_config, 0x90002144
.global __CPR_attempt_PLL_config
.set strcmp, 0x900088a0
.global strcmp
.set __USB_dTDSetup, 0x90002814
.global __USB_dTDSetup
.set port_EndpointInformation, 0x90107638
.global port_EndpointInformation
.set DDR_Initialize, 0x90008e34
.global DDR_Initialize
.set __memcpy_fast, 0x90003910
.global __memcpy_fast
 .set __memset_fast, 0x90003ac0
.global __memset_fast
.set __bootdrv_SEBI_putchar, 0x90005a4c
.global __bootdrv_SEBI_putchar
.set __nalu_type, 0x90107fc4
.global __nalu_type
.set __boot_setup_NAND_boot_device, 0x90000d48
.global __boot_setup_NAND_boot_device
.set strcspn, 0x900089d8
.global strcspn
.set __bootdrv_USBD_setup_descriptors, 0x9000613c
.global __bootdrv_USBD_setup_descriptors
.set __bootdrv_SEBI_getchar, 0x90005a0c
.global __bootdrv_SEBI_getchar
.set __boot_setup_debug_boot, 0x90000984
.global __boot_setup_debug_boot
.set _gBulkOutMaximumPacketLength, 0x90107614
.global _gBulkOutMaximumPacketLength
.set __boot_pad_override_and_mask, 0x90107dc0
.global __boot_pad_override_and_mask
.set __bootdrv_SPIS_setup, 0x90005924
.global __bootdrv_SPIS_setup
.set __boot_setup_SDIO_boot_device_phase2, 0x9000180c
.global __boot_setup_SDIO_boot_device_phase2
.set __boot_setup_SPI_boot_device, 0x90000bbc
.global __boot_setup_SPI_boot_device
.set __pps_ScalingList8x8, 0x90107fdc
.global __pps_ScalingList8x8
.set DDR_EnterStandAloneMode, 0x90008f64
.global DDR_EnterStandAloneMode
.set __AES_leon_decrypt, 0x900032cc
.global __AES_leon_decrypt
.set __bootdrv_USBD_setup, 0x90005c34
.global __bootdrv_USBD_setup
.set _gEP0MaximumPacketLength, 0x9010760c
.global _gEP0MaximumPacketLength
.set __CPR_extract_retention_data, 0x900023dc
.global __CPR_extract_retention_data
.set __bootdrv_UART_putchar, 0x90006370
.global __bootdrv_UART_putchar
.set __backdoor_ifsel_modes, 0x90007880
.global __backdoor_ifsel_modes
.set __boot_device_getchar_fn, 0x90107db0
.global __boot_device_getchar_fn
.set __bootdrv_SPIS_getchar, 0x900058c8
.global __bootdrv_SPIS_getchar
.set __bootdrv_NAND_setup, 0x90005708
.global __bootdrv_NAND_setup
.set __USB_device_receive, 0x900029d0
.global __USB_device_receive
.set __bootdrv_UART_setup, 0x90006398
.global __bootdrv_UART_setup
.set __bootdrv_USBH_getchar, 0x90005090
.global __bootdrv_USBH_getchar
.set __SHAVE_AAC_table_3, 0x9000b408
.global __SHAVE_AAC_table_3
.set __nalu_ptr, 0x90107fcc
.global __nalu_ptr
.set __SHAVE_MPEG2_Table, 0x9000cab0
.global __SHAVE_MPEG2_Table
.set __USB_host_receive, 0x90002fa0
.global __USB_host_receive
!; log1p doesn't work correctly:
!; .set log1p, 0x90008d08
!; .global log1p
.set __nalu_bpos, 0x90107fd0
.global __nalu_bpos
.set __BOOT_bootkey, 0x90107d30
.global __BOOT_bootkey
.set __bootdrv_USBD_getchar, 0x90005bf4
.global __bootdrv_USBD_getchar
.set strspn, 0x90008a40
.global strspn
.set __bootcfg_build_date, 0x9000004c
.global __bootcfg_build_date
.set __bootcfg_backdoor_en, 0x90000058
.global __bootcfg_backdoor_en
.set __bootdrv_IICM_setup, 0x90004df4
.global __bootdrv_IICM_setup
.set strlen, 0x900089d0
.global strlen
.set __GPIO_cfg_init_default, 0x90002630
.global __GPIO_cfg_init_default
.set __AES_leon_done, 0x90003614
.global __AES_leon_done
.set __bootdrv_IICS_getchar, 0x90005968
.global __bootdrv_IICS_getchar
.set __bootdrv_DDR_setup, 0x900068b4
.global __bootdrv_DDR_setup
.set H264_decode_PPS, 0x9000e15c
.global H264_decode_PPS
.set strchr, 0x9000885c
.global strchr
.set __USB_host_send, 0x90002eb0
.global __USB_host_send
.set __bootdrv_NAND_getchar, 0x90005650
.global __bootdrv_NAND_getchar
.set __bootdrv_SDIO_setup_phase2, 0x900065e4
.global __bootdrv_SDIO_setup_phase2
.set __SHAVE_AAC_table_1, 0x9000b600
.global __SHAVE_AAC_table_1
.set __CPR_complete_otp_read, 0x90002204
.global __CPR_complete_otp_read
.set p_bulkdata_in, 0x90107670
.global p_bulkdata_in
.set __boot_setup_IIC_boot_device, 0x90000ab8
.global __boot_setup_IIC_boot_device
.set _gBulkInEndpointNum, 0x90107618
.global _gBulkInEndpointNum
!;.set frexp, 0x90008ad4 // replaced by in-mdk frexpf
!;.global frexp
.set __bootdrv_SDIO_setup_tuples, 0x9000662c
.global __bootdrv_SDIO_setup_tuples
.set __bootdrv_SPIM_setup, 0x90004bd0
.global __bootdrv_SPIM_setup
.set __boot_SDIO_pwr_detect_pin, 0x90007890
.global __boot_SDIO_pwr_detect_pin
.set PowerDownLeon, 0x9000a27c
.global PowerDownLeon

.set GpioPadSetArrayVal, 0x900096fc
.global GpioPadSetArrayVal
.set GpioPadSetFast, 0x900092ac
.global GpioPadSetFast
.set GPIO_PAD_set_all_value, 0x900090f8
.global GPIO_PAD_set_all_value
.set GpioPadSetFastMask, 0x90009488
.global GpioPadSetFastMask
!;.set GpioPadGetAllTable, 0x90009080
!;.global GpioPadGetAllTable
.set GpioPadSet, 0x900091b8
!;.global GpioPadSet
!;.set GpioPadSetRangeVal, 0x9000976c
.global GpioPadSetRangeVal
!;.set GpioPadSetAllTable, 0x90009014
!;.global GpioPadSetAllTable
.set GpioPadSetArrayArray, 0x90009734
.global GpioPadSetArrayArray
.set GpioPadGet, 0x90009674
.global GpioPadGet

#ifdef USE_ROM_TRAPS
.set traphandler, 0x90008000
.global traphandler
.set __T_window_overflow, 0x9000801c
.global __T_window_overflow
.set __T_division_by_0, 0x9000837c
.global __T_division_by_0
.set __T_illegal_instruction, 0x90008330
.global __T_illegal_instruction
.set __T_IRQ_done, 0x900081ec
.global __T_IRQ_done
.set __T_FPU_exception, 0x90008218
.global __T_FPU_exception
.set __T_tag_overflow, 0x90008354
.global __T_tag_overflow
.set __T_IRQ, 0x9000810c
.global __T_IRQ
.set __T_window_underflow, 0x90008098
.global __T_window_underflow
.set __T_IRQ_call_handler, 0x900081c4
.global __T_IRQ_call_handler
.set __T_bad_trap, 0x90008388
.global __T_bad_trap
.set __T_FPU_disabled, 0x9000827c
.global __T_FPU_disabled
.set __T_IRQ_have_handler, 0x9000812c
.global __T_IRQ_have_handler
.set __T_privileged_instruction, 0x9000833c
.global __T_privileged_instruction
.set __T_software_trap, 0x90008390
.global __T_software_trap
.set __T_unaligned_access, 0x90008348
.global __T_unaligned_access
.set __T_watchpoint_hit, 0x90008290
.global __T_watchpoint_hit
.set __T_breakpoint_hit, 0x90008364
.global __T_breakpoint_hit
#endif


.set DrvLcdLayer2, 0x90009c58
.global DrvLcdLayer2
.set DrvLcdCsc, 0x90009fd8
.global DrvLcdCsc
.set DrvLcdVl2Scale, 0x90009f0c
.global DrvLcdVl2Scale
.set DrvLcdVl1Scale, 0x90009ec8
.global DrvLcdVl1Scale
.set DrvLcdPwm2, 0x90009e78
.global DrvLcdPwm2
.set DrvLcdCrPlanar, 0x9000a1a8
.global DrvLcdCrPlanar
.set DrvLcdLut, 0x9000a0a8
.global DrvLcdLut
.set DrvLcdDmaLayer2, 0x90009ca8
.global DrvLcdDmaLayer2
.set DrvLcdTransparency, 0x9000a22c
.global DrvLcdTransparency
.set DrvLcdPwm0, 0x90009dd8
.global DrvLcdPwm0
.set DRV_CPR_PLL_change, 0x9000a7cc
.global DRV_CPR_PLL_change
.set DrvLcdLayerPos, 0x9000a254
.global DrvLcdLayerPos
.set DrvLcdDmaLayer1, 0x90009be8
.global DrvLcdDmaLayer1
.set DrvLcdLayer3, 0x90009d18
.global DrvLcdLayer3
.set DrvLcdDmaLayer3, 0x90009d68
.global DrvLcdDmaLayer3
.set DrvLcdCtrl, 0x90009a98
.global DrvLcdCtrl
.set DrvLcdCbPlanar, 0x9000a124
.global DrvLcdCbPlanar
.set DrvLcdTiming, 0x90009990
.global DrvLcdTiming
.set DrvLcdLayer1, 0x90009b98
.global DrvLcdLayer1
.set DrvLcdGl1Scale, 0x90009f50
.global DrvLcdGl1Scale
.set DrvLcdPwm1, 0x90009e28
.global DrvLcdPwm1
.set DrvLcdGl2Scale, 0x90009f94
.global DrvLcdGl2Scale
.set DrvLcdDmaLayer0, 0x90009b20
.global DrvLcdDmaLayer0
.set DrvLcdLayer0, 0x90009ad0
.global DrvLcdLayer0

!; the following is deprecated. Do not use, because it writes to
!; a location which overlaps with the default irq_table location
!;##ifdef ROM_SYS_CFG
!;.set DRV_SYS_config, 0x9000ad10
!;.global DRV_SYS_config
!;#endif
!; NO_ROM
#endif
