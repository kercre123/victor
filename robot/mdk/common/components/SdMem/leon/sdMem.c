#include "sdMem.h"
#include "DrvCpr.h"
#include "DrvTimer.h"
#include "isaac_registers.h"
#include "DrvHsdio.h"
#include "isaac_registers.h"
#include "assert.h"
#include "DrvIcb.h"
#include "DrvGpio.h"
#include "swcLeonUtils.h"

#ifndef DRV_SDMEM_DEBUG
#define DRV_SDMEM_DEBUG 0
#endif
/* Level 0: No Debug
 * Level 1: Critical Errors (programming errors only)
 * Level 2: Warnings (programming errors only)
 * Level 3: Notice
 * Level 4: Verbose (affects program flow significantly)
 * Level 5: Even more verbose
 */
#define SDMEM_DEBUG_PREFIX ""

#define DPRINTF1(...) do {} while (0)
#define DPRINTF2(...) do {} while (0)
#define DPRINTF3(...) do {} while (0)
#define DPRINTF4(...) do {} while (0)
#define DPRINTF5(...) do {} while (0)
#if DRV_SDMEM_DEBUG >=1
#include <stdio.h>
#undef DPRINTF1
#define DPRINTF1(...) printf(SDMEM_DEBUG_PREFIX __VA_ARGS__)
#endif
#if DRV_SDMEM_DEBUG >= 2
#undef DPRINTF2
#define DPRINTF2(...) printf(SDMEM_DEBUG_PREFIX __VA_ARGS__)
#endif
#if DRV_SDMEM_DEBUG >= 3
#undef DPRINTF3
#define DPRINTF3(...) printf(SDMEM_DEBUG_PREFIX __VA_ARGS__)
#endif
#if DRV_SDMEM_DEBUG >= 4
#undef DPRINTF4
#define DPRINTF4(...) printf(SDMEM_DEBUG_PREFIX __VA_ARGS__)
#if DRV_SDMEM_DEBUG >= 5
#undef DPRINTF5
#define DPRINTF5(...) printf(SDMEM_DEBUG_PREFIX __VA_ARGS__)
#endif
#endif

const tySdMemSlotCfg SdMemMv0153 = {
    .hostPeripheralIndex = 1,
    .slotIndex = 1,
    .availableBusBits = 4,
    .cardDetectPinAvailabe = 1,
    .writeProtectPinAvailable = 0,
    .maximumSdClockKhz = 25000, //50000,
    .maximumCurrentConsumption = 1000,
    .allowHighSpeedTiming = 0,
    .requiredVoltageWindow = D_SDIO_HOST_VOLT_RANGE,
    .pins = {
        {85, 90, 2},
        {92, 92, 2},
        {-1, -1, -1},
    },
};

const tySdMemSlotCfg SdMemMv0117 = { // TODO not yet tested
    .hostPeripheralIndex = 1,
    .slotIndex = 2,
    .availableBusBits = 4,
    .cardDetectPinAvailabe = 1,
    .writeProtectPinAvailable = 1,
    .maximumSdClockKhz = 25000, //50000,
    .maximumCurrentConsumption = 1000,
    .allowHighSpeedTiming = 0,
    .requiredVoltageWindow = D_SDIO_HOST_VOLT_RANGE,
    .pins = {
        {129, 136, 2},
        {-1, -1, -1},
    },
};

static tySdMemSlotState *slots[2][2] = {{NULL, NULL}, {NULL, NULL}};

static u32 getSlotBaseAddress(int hostPeripheralIndex, int slotIndex) {
    u32 base = 0;
    switch (hostPeripheralIndex) {
        case 1: base = SDIOH1_BASE_ADR; break;
        case 2: base = SDIOH2_BASE_ADR; break;
    }
    if (slotIndex==1 && base!=0) return base;
    if (slotIndex==2 && hostPeripheralIndex==1) return base + 0x100;
    DPRINTF1("Error: SD MEM pheripheral/slot not valid (%d/%d)\n", hostPeripheralIndex, slotIndex);
    return 0;
}

static int calcDivider(int hostClkKhz, int maxClkKhz, int max2ClkKhz) {
    int desiredClkKhz = (maxClkKhz < max2ClkKhz) ? maxClkKhz: max2ClkKhz;
    int preliminaryDivider = (hostClkKhz + desiredClkKhz - 1) / desiredClkKhz;
    int i;
    for (i=1; i < preliminaryDivider; i<<=1);
    assert(i>=1);
    assert(i<=256);
    return i;
}

static void sdMemBusClkSetDivider(tySdMemSlotState *slotState, int divider) {
	DPRINTF3("Setting SD bus Clk divider to %d, frequency: %d kHz\n",
						divider, slotState->hostPeripheralClockKhz / divider);
	if (slotState->slotCfg->allowHighSpeedTiming &&
	        slotState->hostPeripheralClockKhz / divider >= 25000) {
	    SET_REG_WORD(slotState->sbase + SDIOH_HOST_CTRL ,
	            GET_REG_WORD_VAL(slotState->sbase + SDIOH_HOST_CTRL) | D_HSDIO_CTRL_HIGH_SPEED);
	    DPRINTF3("Setting High-speed timing\n");
	} else
	    SET_REG_WORD(slotState->sbase + SDIOH_HOST_CTRL ,
	            GET_REG_WORD_VAL(slotState->sbase + SDIOH_HOST_CTRL) & ~D_HSDIO_CTRL_HIGH_SPEED);
	u32 clkreg = GET_REG_WORD_VAL(slotState->sbase + SDIOH_CLK_CTRL);
	clkreg &= ~0xFF00;
	clkreg |= ((1 << 6) << divider) & 0xFF00;
	SET_REG_WORD(slotState->sbase + SDIOH_CLK_CTRL,  clkreg);
	slotState->currentDivider = divider;
}

static void sdMemIrqHandlerCommon(tySdMemSlotState *slotState, u32 int_status) {
	if (int_status & 2) { // transfer complete interrupt
		SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 2);
		slotState->dataTransferCmdFinished = 1;
	}
	if (int_status & 8) { // dma interrupt
		SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 8);
		SET_REG_WORD(slotState->sbase + SDIOH_SDMA, GET_REG_WORD_VAL(slotState->sbase + SDIOH_SDMA));
	}
}

static void sdMemIrqHandlerHost1(u32 source) {
	int base = SDIOH1_BASE_ADR; // this variable is used as optimization to force
	// the compiler to use the 'ld [reg + imm], reg' form of the load instruction
	int s1 = GET_REG_WORD_VAL(base + SDIOH_INT_STATUS);
	int s2 = GET_REG_WORD_VAL(base + 0x100 + SDIOH_INT_STATUS);
	SET_REG_WORD(base + SDIOH_INT_STATUS, 1);
	SET_REG_WORD(base + 0x100 + SDIOH_INT_STATUS, 1);
    // if everything went ok, then this first part should be no more then 6 instructions

	if (s1) sdMemIrqHandlerCommon(slots[0][0], s1);
	if (s2) sdMemIrqHandlerCommon(slots[0][1], s2);
	if (!s1 && !s2) DPRINTF2("Warning: received SD host 1 interrupt, but no interrupt bit was set!\n");

	DrvIcbIrqClear(IRQ_SDIO_HOST_1);
}

static void sdMemIrqHandlerHost2(u32 source) {
	int base = SDIOH2_BASE_ADR; // this variable is used as optimization
	int s = GET_REG_WORD_VAL(base + SDIOH_INT_STATUS);
	SET_REG_WORD(base + SDIOH_INT_STATUS, 1);

	if (!s) DPRINTF2("Warning: received SD host 2 interrupt, but no interrupt bit was set!\n");

	sdMemIrqHandlerCommon(slots[1][0], s);
	DrvIcbIrqClear(IRQ_SDIO_HOST_2);
}

static void sdMemSetupInterrupt(tySdMemSlotState *slotState) {
	slots[slotState->slotCfg->hostPeripheralIndex - 1][slotState->slotCfg->slotIndex - 1] = slotState;
	if (slotState->slotCfg->hostPeripheralIndex == 1) {
		slotState->irq_number = IRQ_SDIO_HOST_1;
		slotState->irq_handler = sdMemIrqHandlerHost1;
	} else {
		slotState->irq_number = IRQ_SDIO_HOST_2;
		slotState->irq_handler = sdMemIrqHandlerHost2;
	}
	DrvIcbDisableIrq(slotState->irq_number);
	DrvIcbIrqClear(slotState->irq_number);
	DrvIcbSetIrqHandler(slotState->irq_number, slotState->irq_handler);
	DrvIcbConfigureIrq(slotState->irq_number, SDMEM_INTERRUPT_LEVEL, POS_LEVEL_INT);

	SET_REG_WORD(slotState->sbase + SDIOH_INT_SIG_ENABLE, 1 | 2 | 8);
}

void sdMemSetupSlot(tySdMemSlotState *slotState, const tySdMemSlotCfg *slotCfg) {
	slotState->slotCfg = slotCfg;
    slotState->sbase = getSlotBaseAddress(slotCfg->hostPeripheralIndex, slotCfg->slotIndex);
    slotState->slot = slotCfg->slotIndex - 1;
    slotState->base = getSlotBaseAddress(slotCfg->hostPeripheralIndex, 1);
    
    DPRINTF4("sdMemSetupSlot(host: %d, slot: %d)\n", slotCfg->hostPeripheralIndex, slotCfg->slotIndex);

    // Check if needed clocks are enabled, and if the SD Host peripheral clock is not too high.
    // Also save the SD Host peripheral clock frequency into the slotState
    // It will later be used to determine the identification and data transfer mode clock dividers.
    u32 clkmask = (slotCfg->hostPeripheralIndex == 1) ? (DEV_SDIO_H1>>32) : (DEV_SDIO_H1>>32);
    if (!(GET_REG_WORD_VAL(CPR_CLK_EN1_ADR) & clkmask)) DPRINTF2("WARNING: SD Host clock is not enabled!\n");
    if (!(GET_REG_WORD_VAL(CPR_CLK_EN1_ADR) & (DEV_SAHB_BUS>>32))) DPRINTF2("WARNING: Slow AHB clock is not enabled!\n");
    clkmask = (slotCfg->hostPeripheralIndex == 1) ? AUX_CLK_MASK_SDHOST_1 : AUX_CLK_MASK_SDHOST_2;
    if (!(GET_REG_WORD_VAL(CPR_AUX_CLK_EN_ADR) & clkmask)) DPRINTF2("WARNING: SD Host AUX clock is not enabled!\n");
    if (!(GET_REG_WORD_VAL(CPR_AUX_CLK_EN_ADR) & AUX_CLK_MASK_SAHB)) DPRINTF2("WARNING: Slow AHB AUX clock is not enabled!\n");
    tyClockType clocktype = (slotCfg->hostPeripheralIndex == 1) ? AUX_CLK_SDHOST_1 : AUX_CLK_SDHOST_2;
    slotState->hostPeripheralClockKhz = DrvCprGetClockFreqKhz(clocktype, NULL);
    slotState->systemToPeripheralClockRatio = ((float)DrvCprGetSysClockKhz()) / slotState->hostPeripheralClockKhz;
    DPRINTF3("host peripheral %d is running at %d kHz\n", slotCfg->hostPeripheralIndex, slotState->hostPeripheralClockKhz);
    if (slotState->hostPeripheralClockKhz > 52000) DPRINTF2("WARNING: SD Host peripheral clock is too high!\n");
    if (slotState->hostPeripheralClockKhz < 100) DPRINTF2("WARNING: SD Host peripheral clock should be more then 100kHz\n");
    
    DPRINTF4("Resetting the SD host peripheral nr. %d\n", slotCfg->hostPeripheralIndex);
    DrvCprSysDeviceAction(RESET_DEVICES, (slotCfg->hostPeripheralIndex == 1) ? DEV_SDIO_H1 : DEV_SDIO_H2);
    
    // Calculate SD bus clock dividers:
    slotState->identificationDivider = calcDivider(slotState->hostPeripheralClockKhz,
                                                   slotCfg->maximumSdClockKhz,
                                                   400);
    DPRINTF3("Identification frequency: %d kHz, divider: %d\n",
    			slotState->hostPeripheralClockKhz / slotState->identificationDivider,
    			slotState->identificationDivider);
    slotState->dataTransferDivider = calcDivider(slotState->hostPeripheralClockKhz,
                                                   slotCfg->maximumSdClockKhz,
                                                   25000);
    DPRINTF3("Default speed data transfer frequency: %d kHz, divider: %d\n",
        			slotState->hostPeripheralClockKhz / slotState->dataTransferDivider,
        			slotState->dataTransferDivider);
    slotState->highSpeedDivider = calcDivider(slotState->hostPeripheralClockKhz,
                                                   slotCfg->maximumSdClockKhz,
                                                   50000);
    if (slotState->hostPeripheralClockKhz / slotState->highSpeedDivider >= 25000)
    	DPRINTF3("High speed data transfer frequency: %d kHz, divier: %d\n",
    				slotState->hostPeripheralClockKhz / slotState->highSpeedDivider,
    				slotState->highSpeedDivider);
    else DPRINTF3("High speed data transfer disabled\n");
    slotState->MMC20MHzDivider = calcDivider(slotState->hostPeripheralClockKhz,
            slotCfg->maximumSdClockKhz,
            20000);
    DPRINTF3("Default speed data transfer frequency: %d kHz, divider: %d\n",
            slotState->hostPeripheralClockKhz / slotState->MMC20MHzDivider,
            slotState->MMC20MHzDivider);
    if (slotState->hostPeripheralClockKhz / slotState->identificationDivider < 100)
        DPRINTF2("WARNING: SD Host identification frequency too low, must be more then 100kHz\n");
    
    DrvHsdioInit(slotState->base, slotState->slot,
                   D_HSDIO_CTRL_1BIT_MODE);
    sdMemBusClkSetDivider(slotState, slotState->identificationDivider);

    const tySdMemPinRange *pr = slotCfg->pins;
    while (pr[0].start>=0) {
        DrvGpioModeRange(pr[0].start, pr[0].end, pr[0].mode);
        pr++;
    }
    slotState->cardInitialized = 0;
    slotState->writeProtected = 0;
    sdMemSetupInterrupt(slotState);
}

/// Sleeps for the maximum of 'clocks' SD clock cycles, or for 'us' microseconds
static void sleepSdClocksOrMicro(tySdMemSlotState *slotState, u32 clocks, u32 us) {
	u32 sd_ticks = (u32) (clocks * slotState->systemToPeripheralClockRatio * slotState->currentDivider);
	u32 us_ticks = DrvCprGetSysClocksPerUs() * us;
	SleepTicks(sd_ticks > us_ticks ? sd_ticks : us_ticks);
}

int sdMemNoCard(tySdMemSlotState *slotState) {
	/* Note: in current Myriad the D_HSDIO_STAT_CARD_STATE_STABLE and
	 * D_HSDIO_STAT_CARD_INSERT bits don't work. Debouncing is only
	 * important at card init, so it will not be implemented in this function
	 * to avoid unnecessary looping
	 */
	if (slotState->slotCfg->cardDetectPinAvailabe)
		return (GET_REG_WORD_VAL(slotState->sbase + SDIOH_PRESENT_STATE)
				& (D_HSDIO_STAT_CARD_DETECT)) !=
					(D_HSDIO_STAT_CARD_DETECT);
	return 0;
}

#define CARD_DETECT_DEBOUNCE_MS 1.0
static int sdMemNoCardDebounced0(tySdMemSlotState *slotState) {
	if (!slotState->slotCfg->cardDetectPinAvailabe) return 0;
	if (sdMemNoCard(slotState)) return 1;
	tyTimeStamp timeStamp;
	DrvTimerStart(&timeStamp);
	while (DrvTimerTicksToMs(DrvTimerElapsedTicks(&timeStamp))<CARD_DETECT_DEBOUNCE_MS)
		if (sdMemNoCard(slotState)) return 1;
	return 0;
}

static void sdMemResetCMDLine(tySdMemSlotState *slotState) {
	u32 reg = GET_REG_WORD_VAL(slotState->sbase + SDIOH_CLK_CTRL);
	reg &= ~( (1<<(16+8+2)) | (1<<(16+8+1)) | (1<<(16+8+0)) );
	reg |= (1<<(16+8+1));
	SET_REG_WORD(slotState->sbase + SDIOH_CLK_CTRL, reg);
}

#define RETURN_IF_ERROR_CMD(errvar, cmd) \
	if (errvar) { \
		DPRINTF3("init failed because of CMD%d, error: 0x%x\n", cmd, errvar); \
		return 0x100 + cmd; \
	}

#define RETURN_IF_ERROR_ACMD(errvar, cmd) \
	if (errvar) { \
		DPRINTF3("init failed because of ACMD%d, error: 0x%x\n", cmd, errvar); \
		return 0x200 + cmd; \
	}

static int sdMemCmd6(tySdMemSlotState *slotState, u32 mode, u32 functionGroup, u32 function, u32 switchResponse[]) {
    if (slotState->SD_SPEC < 1) { // SD SPEC 1.10 is not supported
        DPRINTF4("Card doesn't support SD spec 1.10\n");
        return -1;
    }
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, (1<<5)); // make sure buf read ready is cleared
    while ( GET_REG_WORD_VAL(slotState->sbase + SDIOH_PRESENT_STATE) & 0x03 ) NOP;  // wait while command inhibit
    SET_REG_WORD(slotState->sbase + SDIOH_BLOCK_SIZE, D_HSDIO_BLOCK_S_BOUND_512K | (512/8) | (1 << 16));
    u32 functionGroupMask = (0xf)<<((functionGroup-1) * 4);
    u32 functionShifted = (function)<<((functionGroup-1) * 4);
    u32 query = (~functionGroupMask) & 0x00FFFFFF;
    if (mode) query |= 0x80000000;
    query |= functionShifted;
    DPRINTF5("CMD6 query: 0x%08x\n", query);
    SET_REG_WORD(slotState->sbase + SDIOH_ARGUEMENT , query); // MODE 0 , query
    SET_REG_WORD(slotState->sbase + SDIOH_TRANS_MODE,
            (6<<24) | // CMD6
            D_HSDIO_COMM_R_TYPE_LEN_48 |
            D_HSDIO_TRMODE_DIR_RD |
            D_HSDIO_COMM_DATA_PRESENT);
    u32 SDIO_err;
    u32 resp = DrvHsdioGetR1(slotState->base, slotState->slot, &SDIO_err);
    if (SDIO_err) {
        DPRINTF4("Card returned error to CMD6\n");
        SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 0xffffffff);
        return -2;
    }
    while (!(GET_REG_WORD_VAL(slotState->sbase + SDIOH_INT_STATUS) & (1<<5))) {
        int err = (GET_REG_WORD_VAL(slotState->sbase + SDIOH_INT_STATUS) >> 16) & 0xFFFF;
        if (err) {
            DPRINTF4("Error during CMD6\n", err);
            SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 0xffffffff);
            return -3;
        }
    }
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<5); // clear buffer read ready int
    int i;
    for (i=15;i>=0;i--) {
        u32 t = GET_REG_WORD_VAL(slotState->sbase + SDIOH_BUF_DATA_PORT);
        switchResponse[i] = ((t & 0xff) << 24) | ((t & 0xff00) << 8) | ((t & 0xff0000) >> 8) | ((t >> 24) & 0xff);
    }
    int required_current = switchResponse[15] >> 16;
    DPRINTF5("Required current: %u mA\n", required_current);
    if (required_current > slotState->slotCfg->maximumCurrentConsumption) {
        DPRINTF4("Current consumption is too high for functionGroup/function: %d/%d\n", functionGroup, function);
        return -4;
    }

    for (i=6;i>=1;i--) {
        slotState->supportedFunctions[i-1] = (switchResponse[(400 + (i-1) * 16) / 32] >> ((400 + (i-1) * 16) % 32)) & 0xffff;
        if (slotState->supportedFunctions[i-1] & (0x7ffe))
            DPRINTF5("CMD6 Supported Functions in group %d: %x\n", i, slotState->supportedFunctions[i-1]);
    }

    u32 ds_version = (switchResponse[368 / 32] >> (368 % 32)) & 0xff;
    if (ds_version == 1) {
        u32 this_fg_busy_bitaddr = 272 + (functionGroup-1) * 16;
        u32 this_fg_busy = (switchResponse[this_fg_busy_bitaddr / 32] >> (this_fg_busy_bitaddr % 32)) & 0xffff;
        if (this_fg_busy & (1 << function)) {
            DPRINTF5("The selected function is currently busy!\n");
            return -6;
        }
    }

    u32 responseFunction = (switchResponse[(376 + 4 * (functionGroup - 1)) / 32] >> ((376 + 4 * (functionGroup - 1)) % 32)) & 0xf;
    if (function != 0xf && responseFunction != function) {
       DPRINTF5("The card can't switch to functiongroup/function: %d/%d\n", functionGroup, function);
       return -5;
    }

    return 0;
}

static void sdMemTrySwitchToHighSpeedMode(tySdMemSlotState *slotState) {
    if (slotState->hostPeripheralClockKhz / slotState->highSpeedDivider < 25000) {
        DPRINTF4("High speed is not supported by slot configuration\n");
        return;
    }
    u32 switchResponse[16];
    if (0==sdMemCmd6(slotState, /*mode*/0, /*fg*/1, /*f*/1, switchResponse)) {
        if (0==sdMemCmd6(slotState, /*mode*/1, /*fg*/1, /*f*/1, switchResponse)) {
            DPRINTF4("Switching to high speed was successful!\n");
            // now wait 8 SDCLK cycles
            sleepSdClocksOrMicro(slotState, 8, 0);
            // and set up the host for high-speed mode:
            sdMemBusClkSetDivider(slotState, slotState->highSpeedDivider);
        } else {
            DPRINTF4("Card first claimed it supported high speed, but then refused to switch.\n");
        }
    } else {
        DPRINTF4("Card doesn't support high-speed mode\n");
    }
}

static int sdMemMMCInit(tySdMemSlotState *slotState);
static u32 sdMemGetSectorCountFromCSD(tySdMemSlotState *slotState);

int sdMemCardInit(tySdMemSlotState * slotState) {
	slotState->cardInitialized = 0;
    if (slotState->slotCfg==NULL) {
 	    DPRINTF1("ERROR: sdMemCardInit: You must first call sdMemSetupSlot\n");
	    return -1;
    }
    DPRINTF4("sdMemCardInit(host: %d, slot: %d)\n", slotState->slotCfg->hostPeripheralIndex, slotState->slotCfg->slotIndex);

    if (sdMemNoCardDebounced0(slotState)) {
    	DPRINTF3("no card detected using the cardDetect pin\n");
    	return -2;
    }

    slotState->writeProtected = 0;
    if (slotState->slotCfg->writeProtectPinAvailable &&
    		(GET_REG_WORD_VAL(slotState->sbase + SDIOH_PRESENT_STATE) & D_HSDIO_STAT_WP == 0))
    	slotState->writeProtected = 1;

    sdMemBusClkSetDivider(slotState, slotState->identificationDivider);

    sleepSdClocksOrMicro(slotState, 74, 1000); // see SD Physical Layer Spec, 6.4.1 - Power Up

    SET_REG_WORD(slotState->base + 0x100*slotState->slot + SDIOH_INT_STATUS, 0xFFFF); // clear all interrupts

    u32 SDIO_err;

    DrvHsdioCmd0(slotState->base, slotState->slot, &SDIO_err);
    RETURN_IF_ERROR_CMD(SDIO_err, 0);

    sleepSdClocksOrMicro(slotState, 8, 0); // see SD Physical Layer Spec, 4.3.10 - Switch Function Command

    u32 SD_r = DrvHsdioCmd8(slotState->base, slotState->slot, &SDIO_err, (0x1 << 8) | 0xAA);
    // 0xAA is the recommended check pattern. 0x1 << 8 is the VHS field set to 2.7-3.6V
    slotState->noResponseToCMD8 = SDIO_err & 1;
    SDIO_err &= ~1; // mask no response error
    // no need to clear the command timeout interrupt, as it's already cleared by the _get_R1 function.
    // but it looks like we have to reset the CMD line:
    if (slotState->noResponseToCMD8) sdMemResetCMDLine(slotState);
    RETURN_IF_ERROR_CMD(SDIO_err, 8);
    if (!slotState->noResponseToCMD8 && SD_r != ((0x1 << 8) | 0xAA)) {
    	DPRINTF3("init failed because CMD8 response was not as expected (%x instead of %x)\n", SD_r, ((0x1 << 8) | 0xAA));
    	return -3;
    }

    // CMD55: next command is Application Specific (ACMD*)
    SD_r = DrvHsdioCmd55(slotState->base, slotState->slot, &SDIO_err, 0x0000); // query mode
    slotState->noResponseToACMD41 = SDIO_err & 1;
    SDIO_err &= ~1; // mask no response error
    RETURN_IF_ERROR_CMD(SDIO_err, 55);

    if (slotState->noResponseToACMD41) {
    	sdMemResetCMDLine(slotState);
    	return sdMemMMCInit(slotState);
    }

    if ((SD_r & 0x20) != 0x20) {
    	DPRINTF3("invalid response to CMD55, trying MMC\n");
    	return sdMemMMCInit(slotState);
    }

    SD_r = DrvHsdioAcmd41(slotState->base, slotState->slot, &SDIO_err, /*SD_OCR*/0x0000);
    slotState->noResponseToACMD41 = SDIO_err & 1;
    SDIO_err &= ~1; // mask no response error
    RETURN_IF_ERROR_ACMD(SDIO_err, 41);
    if (slotState->noResponseToACMD41) {
        sdMemResetCMDLine(slotState);
        return sdMemMMCInit(slotState);
    }
    slotState->OCR_reg = SD_r; // bit 30 (CCS) is not yet valid

    if ((slotState->slotCfg->requiredVoltageWindow & slotState->OCR_reg) !=
    		slotState->slotCfg->requiredVoltageWindow) {
    	DPRINTF3("SDcard doesn't support required Vdd voltage window.\n");
    	DPRINTF3("Voltage window - required: %x, supported: %x\n",
    			slotState->slotCfg->requiredVoltageWindow,
    			slotState->OCR_reg);
    	return -4;
    }

    u32 ACMD41_arg = slotState->slotCfg->requiredVoltageWindow;
    if (!slotState->noResponseToCMD8) ACMD41_arg |= 1<<30; // set the HCS bit (we support high capacity)

    tyTimeStamp beforeACMD41TimeStamp;
    DrvTimerStart(&beforeACMD41TimeStamp);
    do {
    	// CMD55: next command is Application Specific (ACMD*)
    	SD_r = DrvHsdioCmd55(slotState->base, slotState->slot, &SDIO_err, 0x0000);
    	RETURN_IF_ERROR_CMD(SDIO_err, 55);
    	if ((SD_r & 0x20) != 0x20) {
    		DPRINTF3("invalid response to second (or later) CMD55\n");
    		return 0x100 + 55;
    	}

    	SD_r = DrvHsdioAcmd41(slotState->base, slotState->slot, &SDIO_err, ACMD41_arg);
    	RETURN_IF_ERROR_ACMD(SDIO_err, 41);
    	if (DrvTimerTicksToMs(DrvTimerElapsedTicks(&beforeACMD41TimeStamp)) > 1000) {
    		// the timeout is 1 second according to SD Physical Layer Spec - 6.4.1 Power Up
    		DPRINTF3("initialization process timeout.");
    		return -5;
    	}
    } while ((SD_r & D_SDIO_CARD_BUSY) != D_SDIO_CARD_BUSY);
    slotState->OCR_reg = SD_r;

    if (!slotState->noResponseToCMD8) slotState->sectorAccess = SD_r & (1<<30);
    else slotState->sectorAccess = 0;

    DrvHsdioCmd2(slotState->base, slotState->slot, &SDIO_err, slotState->CID_reg);
    RETURN_IF_ERROR_CMD(SDIO_err, 2);
    SD_r = DrvHsdioCmd3(slotState->base, slotState->slot, &SDIO_err);
    slotState->stuffed_RCA = SD_r & 0xFFFF0000;
    RETURN_IF_ERROR_CMD(SDIO_err, 3);

    DrvHsdioCmd9(slotState->base, slotState->slot, &SDIO_err, slotState->CSD_reg, slotState->stuffed_RCA);
    RETURN_IF_ERROR_CMD(SDIO_err, 9);

    // maybe to do: if we want to call SET_DSR (CMD4) then do it here, before raising the clock frequency
    // now we're in data-transfer mode, it's safe to raise the SD clock frequency:
    sleepSdClocksOrMicro(slotState, 8, 0); // not explicitly required by spec, just to be safe
    sdMemBusClkSetDivider(slotState, slotState->dataTransferDivider);

    DPRINTF5("  CID = 0x%x 0x%x 0x%x 0x%x \n",
    		slotState->CID_reg[3], slotState->CID_reg[2],
    		slotState->CID_reg[1], slotState->CID_reg[0]);
    DPRINTF5("  CSD = 0x%x 0x%x 0x%x 0x%x \n",
    		slotState->CSD_reg[3], slotState->CSD_reg[2],
    		slotState->CSD_reg[1], slotState->CSD_reg[0]);
    DPRINTF5("  RCA = 0x%x   \n", slotState->stuffed_RCA);
    DPRINTF5("  OCR = 0x%x   \n", slotState->OCR_reg);

    // select card
    SD_r = DrvHsdioCmd7(slotState->base, slotState->slot, &SDIO_err, slotState->stuffed_RCA);
    RETURN_IF_ERROR_CMD(SDIO_err, 7);

    // make sure we're in 1-bit mode, because the following AMCD51 uses the data lines.
    SET_REG_WORD(slotState->sbase + SDIOH_HOST_CTRL, (GET_REG_WORD_VAL(slotState->sbase + SDIOH_HOST_CTRL) & ~(1<<1)));

    // CMD55: next command is Application Specific (ACMD*)
    SD_r = DrvHsdioCmd55(slotState->base, slotState->slot, &SDIO_err, slotState->stuffed_RCA);
    RETURN_IF_ERROR_CMD(SDIO_err, 55);
    if ((SD_r & 0x20) != 0x20) {
    	DPRINTF3("invalid response to CMD55 (befoe ACMD51)\n");
    	return 0x100 + 55;
    }

    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, (1<<5)); // make sure buf read ready is cleared
    while ( GET_REG_WORD_VAL(slotState->sbase + SDIOH_PRESENT_STATE) & 0x03 ) NOP;  // wait while command inhibit
    SET_REG_WORD(slotState->sbase + SDIOH_BLOCK_SIZE, D_HSDIO_BLOCK_S_BOUND_512K | 8 | (1 << 16));
    SET_REG_WORD(slotState->sbase + SDIOH_TRANS_MODE,
    		SD_ACMD_51 |
            D_HSDIO_COMM_R_TYPE_LEN_48 |
            D_HSDIO_TRMODE_DIR_RD |
            D_HSDIO_COMM_DATA_PRESENT);
    u32 resp = DrvHsdioGetR1(slotState->base, slotState->slot, &SDIO_err);
    RETURN_IF_ERROR_ACMD(SDIO_err, 51);

    while (!(GET_REG_WORD_VAL(slotState->sbase + SDIOH_INT_STATUS) & (1<<5))) {
    	int err = (GET_REG_WORD_VAL(slotState->sbase + SDIOH_INT_STATUS) >> 16) & 0xFFFF;
    	if (err) {
    		DPRINTF3("Error during ACMD51 (0x%08x)\n", err);
    		return 0x200 + 51;
    	}
    }
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<5);

    u32 t = GET_REG_WORD_VAL(slotState->sbase + SDIOH_BUF_DATA_PORT);
    slotState->SD_SCR[1] = ((t & 0xff) << 24) | ((t & 0xff00) << 8) | ((t & 0xff0000) >> 8) | ((t >> 24) & 0xff);
    t = GET_REG_WORD_VAL(slotState->sbase + SDIOH_BUF_DATA_PORT);
    slotState->SD_SCR[0] = ((t & 0xff) << 24) | ((t & 0xff00) << 8) | ((t & 0xff0000) >> 8) | ((t >> 24) & 0xff);

    DPRINTF5("  SCR = 0x%x 0x%x\n", slotState->SD_SCR[1], slotState->SD_SCR[0]);
    slotState->cardSupports4bit = (slotState->SD_SCR[1] >> (50-32)) & 1;
    slotState->SD_SPEC = (slotState->SD_SCR[1] >> (56-32)) & 0xf;

    if (slotState->cardSupports4bit && slotState->slotCfg->availableBusBits>=4) {
    	DPRINTF4("switching to 4-bit bus\n");
    	// CMD55: next command is Application Specific (ACMD*)
    	SD_r = DrvHsdioCmd55(slotState->base, slotState->slot, &SDIO_err, slotState->stuffed_RCA);
    	RETURN_IF_ERROR_CMD(SDIO_err, 55);
    	if ((SD_r & 0x20) != 0x20) {
    		DPRINTF3("invalid response to CMD55 (befoe ACMD6)\n");
    		return 0x100 + 55;
    	}

    	// ACMD6 - set the bus to 4 bits
    	SD_r = DrvHsdioAcmd6(slotState->base, slotState->slot, &SDIO_err, 1 );
    	RETURN_IF_ERROR_ACMD(SDIO_err, 6);

    	SET_REG_WORD(slotState->sbase + SDIOH_HOST_CTRL, (GET_REG_WORD_VAL(slotState->sbase + SDIOH_HOST_CTRL) | (1<<1)));
    }
    int i;
    for (i=0;i<5;i++) slotState->supportedFunctions[i] = 0;
    sdMemTrySwitchToHighSpeedMode(slotState);
    slotState->sectorCount = sdMemGetSectorCountFromCSD(slotState);
    slotState->cardInitialized = 1;
    DPRINTF4("sdMemCardInit() successful!\n");
    return 0;
}

/// MMC specific initialization
/// This is not yet tested properly!!!
static int sdMemMMCInit(tySdMemSlotState *slotState) {
    u32 SDIO_err, SD_r;
    DPRINTF4("sdMemMMCInit(host: %d, slot: %d)\n", slotState->slotCfg->hostPeripheralIndex, slotState->slotCfg->slotIndex);
    // SEND_OP_COND: (querying supported voltage range)
    SD_r = DrvHsdioCmd1(slotState->base, slotState->slot, &SDIO_err, /*SD_OCR*/0x0000);
    RETURN_IF_ERROR_CMD(SDIO_err, 1);
    slotState->OCR_reg = SD_r; // Access Mode is not yet valid
    if ((slotState->slotCfg->requiredVoltageWindow & slotState->OCR_reg) !=
            slotState->slotCfg->requiredVoltageWindow) {
        DPRINTF3("MMC doesn't support required Vdd voltage window.\n");
        DPRINTF3("Voltage window - required: %x, supported: %x\n",
                slotState->slotCfg->requiredVoltageWindow,
                slotState->OCR_reg);
        return -4;
    }

    u32 CMD1_arg = slotState->slotCfg->requiredVoltageWindow |
            (1<<30); // sector type addressing is supported

    tyTimeStamp beforeCMD1TimeStamp;
    DrvTimerStart(&beforeCMD1TimeStamp);
    do {
        SD_r = DrvHsdioCmd1(slotState->base, slotState->slot, &SDIO_err, CMD1_arg);
        RETURN_IF_ERROR_CMD(SDIO_err, 1);
        if (DrvTimerTicksToMs(DrvTimerElapsedTicks(&beforeCMD1TimeStamp)) > 1000) {
            // the timeout is 1 second according to MMCA 4.2 spec - Chapter 12.3, Power-up
            DPRINTF3("MMC CMD1 initialization process timeout.");
            return -5;
        }
    } while ((SD_r & D_SDIO_CARD_BUSY) != D_SDIO_CARD_BUSY);
    slotState->OCR_reg = SD_r;

    u32 access_mode = (SD_r >> 29) & 3;
    if (access_mode > 2) {
        DPRINTF3("reported MMC access mode is not supported");
        return -300;
    }
    slotState->sectorAccess = (access_mode == 2);

    // ALL_SEND_CID:
    DrvHsdioCmd2(slotState->base, slotState->slot, &SDIO_err, slotState->CID_reg);
    RETURN_IF_ERROR_CMD(SDIO_err, 2);
    // SET_RELATIVE_ADDR:
    DrvHsdioCmd(slotState->base, slotState->slot, 0x00000000, 0x0000/*bl count*/, 0x00010000/*arg*/,
                   SD_CMD_3 |
                   D_HSDIO_COMM_CMD_IDX_CHK_EN |
                   D_HSDIO_COMM_CMD_CRC_CHK_EN |
                   D_HSDIO_COMM_R_TYPE_LEN_48 |
                   D_HSDIO_TRMODE_DIR_WR);

    SDIO_err = 0;
    DrvHsdioGetR1(slotState->base, slotState->slot, &SDIO_err);
    slotState->stuffed_RCA = 0x00010000;
    RETURN_IF_ERROR_CMD(SDIO_err, 3);

    DrvHsdioCmd9(slotState->base, slotState->slot, &SDIO_err, slotState->CSD_reg, slotState->stuffed_RCA);
    RETURN_IF_ERROR_CMD(SDIO_err, 9);

    // maybe to do: if we want to call SET_DSR (CMD4) then do it here, before raising the clock frequency
    // now we're in data-transfer mode, it's safe to raise the SD clock frequency:
    sleepSdClocksOrMicro(slotState, 8, 0); // not explicitly required by spec, just to be safe
    sdMemBusClkSetDivider(slotState, slotState->MMC20MHzDivider);

    // select card
    SD_r = DrvHsdioCmd7(slotState->base, slotState->slot, &SDIO_err, slotState->stuffed_RCA);
    RETURN_IF_ERROR_CMD(SDIO_err, 7);

    // make sure we're in 1-bit mode
    SET_REG_WORD(slotState->sbase + SDIOH_HOST_CTRL, (GET_REG_WORD_VAL(slotState->sbase + SDIOH_HOST_CTRL) & ~(1<<1)));

    slotState->sectorCount = 0; // TODO: sector count not yet implemented for MMC.
    return 0;
}

u32 sdMemGetSectorSize(tySdMemSlotState *slotState) {
    return 512; // TODO (for bigger sd cards)
}

u32 sdMemGetSectorCount(tySdMemSlotState *slotState) {
    return slotState->sectorCount;
}

static u32 sdMemGetSectorCountFromCSD(tySdMemSlotState *slotState) {
    int bitsshift;

    switch (slotState->CSD_reg[3] >> (30 - 8) & 3) { // CSD is shifted 8 bytes
    case 0: {
        DPRINTF5("Reading card size (CSD 1.0)\n");
        u32 C_SIZE = (slotState->CSD_reg[2] & 0x3) << 10;
        C_SIZE |= slotState->CSD_reg[1] >> 22;
        u32 C_SIZE_MULT = (slotState->CSD_reg[1] >> (47 - 8 - 32)) & 0x07;
        u32 READ_BLK_LEN = (slotState->CSD_reg[2] >> (80 - 8 - 64)) & 0x0f;
        bitsshift = READ_BLK_LEN + C_SIZE_MULT + 2 - 9; // -9 = 512 byte sectors
        DPRINTF5("C_SIZE: %u\n", C_SIZE);
        DPRINTF5("C_SIZE_MULT: %u\n", C_SIZE_MULT);
        DPRINTF5("READ_BLK_LEN: %u\n", READ_BLK_LEN);
        u32 sectorCount = (C_SIZE+1) << bitsshift;
        DPRINTF4("SectorCount: %u (about %u MiB)\n", sectorCount, sectorCount >> 11);
        return sectorCount;
        break;
    }
    case 1: {
        DPRINTF5("Reading card size (CSD 2.0)\n");
        u32 C_SIZE = (slotState->CSD_reg[1] >> (48 - 8 - 32)) & ((1<<22)-1);
        DPRINTF5("C_SIZE: %u\n", C_SIZE);
        u32 sectorCount = (C_SIZE+1) << 10;
        DPRINTF4("SectorCount: %u (about %u MiB)\n", sectorCount, sectorCount >> 11);
        return sectorCount;
        break;
    }
    default:
        DPRINTF3("ERROR: unsupported CSD structure version! returning 0\n");
        break;
    }
    return 0;
}

u32 sdMemGetEraseBlockSize(tySdMemSlotState *slotState) {
    return sdMemGetSectorSize(slotState);
}

int sdMemEraseSector(tySdMemSlotState *slotState, u32 start_sector, u32 end_sector) {
    // TODO
    return 0;
}

int sdMemSync(tySdMemSlotState *slotState) {
    // TODO
    return 0;
}

int sdMemWriteFlipped(tySdMemSlotState *slotState, const unsigned char *Buffer, u32 SectorNumber, u32 SectorCount) {
    // since the cpu already needs to read in and write out the data, to endianness-flip it, there is no point in using DMA
    if (!slotState->sectorAccess) SectorNumber <<= 9;
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<4); // make sure it's cleared.
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<1);
    DrvHsdioCmd(slotState->base, slotState->slot, (u32) Buffer, SectorCount, SectorNumber,
            SD_CMD_25 |
            D_HSDIO_COMM_CMD_IDX_CHK_EN |
            D_HSDIO_COMM_CMD_CRC_CHK_EN |
            D_HSDIO_COMM_DATA_PRESENT |
            D_HSDIO_COMM_R_TYPE_LEN_48 |
            D_HSDIO_TRMODE_CMD12_EN |
            D_HSDIO_TRMODE_BLOCK_COUNT_EN |
            D_HSDIO_TRMODE_MULT_BLOC |
            D_HSDIO_TRMODE_DIR_WR);
    u32 err = 0x00;
      // response should be 120h ready and acmd enabled
    u32 resp = DrvHsdioGetR1(slotState->base, slotState->slot, &err);

    u32 *wbuff = (u32*) Buffer;

    u32 i;
    for (i=0;i<SectorCount;i++) {
        while (!(GET_REG_WORD_VAL(slotState->sbase + SDIOH_INT_STATUS) & (1<<4))) ;  // wait for buffer write ready
        SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<4);
        int j;
        if (((u32)wbuff) & 3) {
            for (j=0;j<512/4;j++) {
                u32 t = (swcLeonGetByte(wbuff, le_pointer) << 24) |
                        (swcLeonGetByte(((char *)wbuff)+1, le_pointer) << 16) |
                        (swcLeonGetByte(((char *)wbuff)+2, le_pointer) << 8) |
                        (swcLeonGetByte(((char *)wbuff)+3, le_pointer) << 0);
                SET_REG_WORD(slotState->sbase + SDIOH_BUF_DATA_PORT, t);
                wbuff ++;
            }
        } else { /* if aligned, then this is faster: */
            for (j=0;j<512/4;j++) {
                u32 t = wbuff[0];
                t = ((t & 0xff) << 24) | ((t & 0xff00) << 8) | ((t & 0xff0000) >> 8) | ((t >> 24) & 0xff);
                SET_REG_WORD(slotState->sbase + SDIOH_BUF_DATA_PORT, t);
                wbuff ++;
            }
        }
    }
    while (!(GET_REG_WORD_VAL(slotState->sbase + SDIOH_INT_STATUS) & (1<<1))) ; // wait for transfer complete
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<1);
    return 0;
}

int sdMemWrite(tySdMemSlotState *slotState, const unsigned char *Buffer, u32 SectorNumber, u32 SectorCount, pointer_type endianness) {
    DPRINTF4("sdMemWrite(sdaddr=%d, count=%d endian=%s)\n", SectorNumber, SectorCount, endianness==be_pointer?"be":"le");
    if (slotState->sectorCount > 0 && SectorNumber + SectorCount > slotState->sectorCount) {
        DPRINTF3("Trying to write beyond SD card size.");
        return -1;
    }
    if (endianness==be_pointer) return sdMemWriteFlipped(slotState, Buffer, SectorNumber, SectorCount);
    if (!slotState->sectorAccess) SectorNumber <<= 9;
    while (GET_REG_WORD_VAL(slotState->sbase + SDIOH_PRESENT_STATE) & 0x03 ) NOP;
    slotState->dataTransferCmdFinished = 0;
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 2 | 8); // clear transfer complete, and DMA interrupt
    // in case something else forgot to clear it before we got here
    DrvIcbIrqClear(slotState->irq_number);
    DrvIcbEnableIrq(slotState->irq_number);
    DrvHsdioCmd(slotState->base, slotState->slot, (u32)Buffer, SectorCount, SectorNumber,
                        SD_CMD_25 |
                        D_HSDIO_COMM_CMD_IDX_CHK_EN |
                        D_HSDIO_COMM_CMD_CRC_CHK_EN |
                        D_HSDIO_COMM_DATA_PRESENT |
                        D_HSDIO_COMM_R_TYPE_LEN_48 |
                        D_HSDIO_TRMODE_DMA_EN |
                        D_HSDIO_TRMODE_CMD12_EN |
                        D_HSDIO_TRMODE_BLOCK_COUNT_EN |
                        D_HSDIO_TRMODE_MULT_BLOC |
                        D_HSDIO_TRMODE_DIR_WR);
    while (!slotState->dataTransferCmdFinished);
    DrvIcbDisableIrq(slotState->irq_number);
    return 0;
}

int sdMemReadFlipped(tySdMemSlotState *slotState, const unsigned char *Buffer, u32 SectorNumber, u32 SectorCount) {
    // since the cpu already needs to read in and write out the data, to endianness-flip it, there is no point in using DMA
    if (!slotState->sectorAccess) SectorNumber <<= 9;
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<5); // make sure it's cleared.
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<1);
    DrvHsdioCmd(slotState->base, slotState->slot, (u32) Buffer, SectorCount, SectorNumber,
            SD_CMD_18 |
            D_HSDIO_COMM_CMD_IDX_CHK_EN |
            D_HSDIO_COMM_CMD_CRC_CHK_EN |
            D_HSDIO_COMM_DATA_PRESENT |
            D_HSDIO_COMM_R_TYPE_LEN_48 |
            D_HSDIO_TRMODE_CMD12_EN |
            D_HSDIO_TRMODE_BLOCK_COUNT_EN |
            D_HSDIO_TRMODE_MULT_BLOC |
            D_HSDIO_TRMODE_DIR_RD);

    u32 err = 0x00;
      // response should be 120h ready and acmd enabled
    u32 resp = DrvHsdioGetR1(slotState->base, slotState->slot, &err);

    u32 *wbuff = (u32*) Buffer;

    u32 i;
    for (i=0;i<SectorCount;i++) {
        while (!(GET_REG_WORD_VAL(slotState->sbase + SDIOH_INT_STATUS) & (1<<5))) ;  // wait for buffer read ready
        SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<5);
        int j;
        if (((u32)wbuff) & 3) {
            for (j=0;j<512/4;j++) {
                u32 t = GET_REG_WORD_VAL(slotState->sbase + SDIOH_BUF_DATA_PORT);
                swcLeonSetByte(wbuff, be_pointer, t & 0xff);
                swcLeonSetByte(((char *)wbuff)+1, be_pointer, (t >> 8) & 0xff);
                swcLeonSetByte(((char *)wbuff)+2, be_pointer, (t >> 16) & 0xff);
                swcLeonSetByte(((char *)wbuff)+3, be_pointer, (t >> 24) & 0xff);
                wbuff ++;
            }
        } else {
            for (j=0;j<512/4;j++) {
                u32 t = GET_REG_WORD_VAL(slotState->sbase + SDIOH_BUF_DATA_PORT);
                t = ((t & 0xff) << 24) | ((t & 0xff00) << 8) | ((t & 0xff0000) >> 8) | ((t >> 24) & 0xff);
                wbuff[0] = t;
                wbuff ++;
            }
        }
    }
    while (!(GET_REG_WORD_VAL(slotState->sbase + SDIOH_INT_STATUS) & (1<<1))) ; // wait for transfer complete
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 1<<1);
    return 0;
}

int sdMemRead(tySdMemSlotState *slotState, unsigned char *Buffer, u32 SectorNumber, u32 SectorCount, pointer_type endianness) {
    DPRINTF4("sdMemRead(sdaddr=%d, count=%d endian=%s)\n", SectorNumber, SectorCount, endianness==be_pointer?"be":"le");
    if (slotState->sectorCount > 0 && SectorNumber + SectorCount > slotState->sectorCount) {
        DPRINTF3("Trying to read beyond SD card size.");
        return -1;
    }
    if (endianness==be_pointer) return sdMemReadFlipped(slotState, Buffer, SectorNumber, SectorCount);
    if (!slotState->sectorAccess) SectorNumber <<= 9;
    swcLeonDataCacheFlush();
    while (GET_REG_WORD_VAL(slotState->sbase + SDIOH_PRESENT_STATE) & 0x03 ) NOP;
    slotState->dataTransferCmdFinished = 0;
    SET_REG_WORD(slotState->sbase + SDIOH_INT_STATUS, 2 | 8); // clear transfer complete, and DMA interrupt
    // in case something else forgot to clear it before we got here
    DrvIcbIrqClear(slotState->irq_number);
    DrvIcbEnableIrq(slotState->irq_number);
    DrvHsdioCmd(slotState->base, slotState->slot, (u32)Buffer, SectorCount, SectorNumber,
                        SD_CMD_18 |
                        D_HSDIO_COMM_CMD_IDX_CHK_EN |
                        D_HSDIO_COMM_CMD_CRC_CHK_EN |
                        D_HSDIO_COMM_DATA_PRESENT |
                        D_HSDIO_COMM_R_TYPE_LEN_48 |
                        D_HSDIO_TRMODE_DMA_EN |
                        D_HSDIO_TRMODE_CMD12_EN |
                        D_HSDIO_TRMODE_BLOCK_COUNT_EN |
                        D_HSDIO_TRMODE_MULT_BLOC |
                        D_HSDIO_TRMODE_DIR_RD);
    while (!slotState->dataTransferCmdFinished);
    DrvIcbDisableIrq(slotState->irq_number);
    return 0;
}


