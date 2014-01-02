; ///
; /// @file
; /// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
; ///            For License Warranty see: common/license.txt
; ///
; /// @brief
; ///

.version 00.50.00

.code .text.MvCV_Conv3x3fp32

.lalign
	conv3x3fp32:
		bru.swp i30
		nop 5

.end