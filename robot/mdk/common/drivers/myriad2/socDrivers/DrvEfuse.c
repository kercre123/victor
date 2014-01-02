#include <DrvEfuse.h>
#include <mv_types.h>
#include <registersMyriad2.h>
#include <DrvRegUtilsDefines.h>
#include <swcLeonUtilsDefines.h>

#define max2(a, b) ((a)>(b)?(a):(b))
#define max3(a, b, c) (max2(max2((a), (b)), (c)))
#define max4(a, b, c, d) (max2(max2((a), (b)), max2((c), (d))))
#define max5(a, b, c, d, e) (max2(max2((a), (b)), max3((c), (d), (e))))
#define max6(a, b, c, d, e, f) (max2(max3((a), (b), (c)), max3((d), (e), (f))))
#define max7(a, b, c, d, e, f, g) (max3(max3((a), (b), (c)), max3((d), (e), (f)), (g)))
#define max8(a, b, c, d, e, f, g, h) (max2(max4((a), (b), (c), (d)), max4((e), (f), (g), (h))))
static void SetPin(u32 pins, int value) {
    u32 val = GET_REG_WORD_VAL(CPR_EFUSE_CTRL_ADR);
    if (value) {
        val |= pins;
    } else {
        val &= ~pins;
    }
    SET_REG_WORD(CPR_EFUSE_CTRL_ADR, val);
}

static void SetAddressLines(int addr) {
    u32 val = GET_REG_WORD_VAL(CPR_EFUSE_CTRL_ADR);
    val &= ~(0x3ff << 9);
    val |= addr << 9;
    SET_REG_WORD(CPR_EFUSE_CTRL_ADR, val);
}

static u32 GetData() {
    return GET_REG_WORD_VAL(CPR_EFUSE_DATA_ADR);
}

static inline void Wait(int nanoSeconds) {
    int i;
    for (i=0;i<(1 + nanoSeconds/83/4);i++) NOP;
    // TODOANDREW: this assumes no PLL, and a 12MHz crystal. implement something more exact, which will work with/without PLL, and with other crystal speeds as well.
}

#define CSB    (1 << 0)
#define STROBE (1 << 1)
#define LOAD   (1 << 2)
#define PGENB  (1 << 3)
#define PS     (1 << 4)
#define PD     (1 << 5)
#define MR     (1 << 6)
#define RSB    (1 << 7)
#define RWL    (1 << 8)
#define A(bit) (1 << (9 + bit))

// the following values are in Nano Seconds!
// They are the worst case situations described in the datasheet, with a 10 percent margin.
#define TSUR_PD_CS ( 1045 * 110 / 100 )
#define THR_PD_CS  (   20 * 110 / 100 )
#define TSUR_PS_CS (   69 * 110 / 100 )
#define THR_PS_CS  (   71 * 110 / 100 )
#define TSUR_PD    ( 1053 * 110 / 100 )
#define THR_PD     (   27 * 110 / 100 )
#define TSUR_PS    (   77 * 110 / 100 )
#define THR_PS     (   78 * 110 / 100 )
#define TSUR_RWL   (   19 * 110 / 100 )
#define THR_RWL    (   18 * 110 / 100 )
#define TSUR_RSB   (   19 * 110 / 100 )
#define THR_RSB    (   19 * 110 / 100 )
#define TSUR_CS    (    8 * 110 / 100 )
#define THR_CS     (    8 * 110 / 100 )
#define TSUR_PG    (    8 * 110 / 100 )
#define THR_PG     (    8 * 110 / 100 )
#define TRD        (  200 * 110 / 100 )
#define TSUR_A     (   18 * 110 / 100 )
#define THR_A      (   17 * 110 / 100 )
#define TSUR_LD    (    8 * 110 / 100 )
#define THR_LD     (    8 * 110 / 100 )
#define TSUR_MR    (   16 * 110 / 100 )
#define THR_MR     (   15 * 110 / 100 )
#define TSQ        (  199 * 110 / 100 )
#define TSQH       (    0 * 110 / 100 )

#define TSUP_PD_PS ( 1045 * 110 / 100 )
#define THP_PD_PS  (   20 * 110 / 100 )
#define TSUP_PS_CS (   69 * 110 / 100 )
#define THP_PS_CS  (   68 * 110 / 100 )
#define TSUP_PD    ( 1121 * 110 / 100 )
#define THP_PD     (   95 * 110 / 100 )
#define TSUP_PS    (   77 * 110 / 100 )
#define THP_PS     (   76 * 110 / 100 )
#define TSUP_RWL   (   19 * 110 / 100 )
#define THP_RWL    (   18 * 110 / 100 )
#define TSUP_RSB   (   19 * 110 / 100 )
#define THP_RSB    (   19 * 110 / 100 )
#define TSUP_CS    (    8 * 110 / 100 )
#define THP_CS     (    8 * 110 / 100 )
#define TSUP_PG    (    8 * 110 / 100 )
#define THP_PG     (    8 * 110 / 100 )
#define TPGM       (   12 * 110 / 100 )
#define TSUP_A     (   18 * 110 / 100 )
#define THP_A      (   17 * 110 / 100 )
#define TSUP_LD    (    8 * 110 / 100 )
#define THP_LD     (    8 * 110 / 100 )


// warning bitAddress is the same that gets on the eFuse address bus,
// so for example the address of word4.bit3 will be
// 4 | (3 << 5) = 0x64, and the address of FB[2]_A1
// will be 7 << 4 = 0x70. Bits of the same word are not
// at continuous addresses
void DrvEfuseProgramBit(int redundancy, int bitAddress) {
    SetAddressLines(bitAddress);
    if (redundancy) {
        SetPin(STROBE | PD | RSB | LOAD | PGENB | PS, 0);
        SetPin(RWL, 1);
    } else {
        SetPin(STROBE | PD | RWL | LOAD | PGENB | PS, 0);
    }
    Wait(TSUP_PD_PS);
    SetPin(PS, 1);
    Wait(TSUP_PS_CS);
    SetPin(CSB, 0);
    Wait(max3(max6(TSUP_PD, TSUP_PG, TSUP_A, TSUP_LD, TSUP_RWL, TSUP_RSB) - TSUP_PD_PS - TSUP_PS_CS, TSUP_PS - TSUP_PS_CS, TSUP_CS));
    SetPin(STROBE, 1);
    Wait(TPGM);
    SetPin(STROBE, 0);

    Wait(max3(THP_A, THP_CS, TPGM)); // note: the datasheet does not requrire a TPGM wait here
    SetPin(CSB, 1);
    Wait(max2(max6(THP_LD, THP_RWL, THP_RSB, THP_PG, THP_PS, THP_PD) - max3(THP_A, THP_CS, TPGM) - THP_PD_PS, THP_PS_CS));
    SetPin(PS, 0);
    Wait(THP_PD_PS);
    SetPin(PGENB, 1);
}

// warning bitAddress is the same that gets on the eFuse address bus,
// so for example the address of word4.bit3 will be
// 4 | (3 << 5) = 0x64, and the address of FB[2]_A1
// will be 7 << 4 = 0x70. Bits of the same word are not
// at continuous addresses
void DrvEfuseProgramBits(int redundancy, int n, int bitAddresses[]) {
    int i;
    SetAddressLines(bitAddresses[0]);
    if (redundancy) {
        SetPin(STROBE | PD | RSB | LOAD | PGENB | PS, 0);
        SetPin(RWL, 1);
    } else {
        SetPin(STROBE | PD | RWL | LOAD | PGENB | PS, 0);
    }
    Wait(TSUP_PD_PS);
    SetPin(PS, 1);
    Wait(TSUP_PS_CS);
    SetPin(CSB, 0);
    Wait(max3(max6(TSUP_PD, TSUP_PG, TSUP_A, TSUP_LD, TSUP_RWL, TSUP_RSB) - TSUP_PD_PS - TSUP_PS_CS, TSUP_PS - TSUP_PS_CS, TSUP_CS));
    for (i=0;i<n;i++) {
        SetPin(STROBE, 1);
        Wait(TPGM);
        SetPin(STROBE, 0);
        Wait(THP_A);
        SetAddressLines(bitAddresses[i+1]);
        Wait(max2(TSUP_A, TPGM)); // note: the datasheet does not requrire a TPGM wait here
    }
    #define __RESIDUAL_WAIT_1 (THP_A + max2(TSUP_A, TPGM))
    #define __RESIDUAL_WAIT_2 (max2(0, THP_CS - __RESIDUAL_WAIT_1))
    Wait(__RESIDUAL_WAIT_2);
    SetPin(CSB, 1);
    #define __RESIDUAL_WAIT_3 (max2(THP_PS_CS, THP_PS - (__RESIDUAL_WAIT_1 + __RESIDUAL_WAIT_2)))
    Wait(__RESIDUAL_WAIT_3);
    SetPin(PS, 0);
    Wait(max2(THP_PD_PS, max5(THP_LD, THP_RWL, THP_RSB, THP_PG, THP_PD) - (__RESIDUAL_WAIT_1 + __RESIDUAL_WAIT_2 + __RESIDUAL_WAIT_3)));
    SetPin(PGENB, 1);
    #undef __RESIDUAL_WAIT_1
    #undef __RESIDUAL_WAIT_2
    #undef __RESIDUAL_WAIT_3
}

void DrvEfuseARead(u32 *dst, int margin) {
    int i;
    SetPin(STROBE | PS | PD | RWL, 0);
    SetAddressLines(0);
    SetPin(PGENB | LOAD, 1);
    SetPin(MR, margin ? 1 : 0);
    Wait(max2(TSUR_PD_CS, TSUR_PS_CS));
    SetPin(CSB, 0);
    Wait(max2(TSUR_CS, max8(TSUR_PD, TSUR_PS, TSUR_MR, TSUR_PG, TSUR_LD, TSUR_RSB, TSUR_RWL, TSUR_A) - max2(TSUR_PD_CS, TSUR_PS_CS)));
    for (i=0;i<32;i++) {
        SetPin(STROBE, 1);
        Wait(max2(TRD, TSQ));
        SetPin(STROBE, 0);
        dst[i] = GetData();
        Wait(THR_A);
        SetAddressLines(i+1);
        Wait(max2(TSUR_A, TRD)); // note: the datasheet does not require a TRD wait here, but just to be safe
    }
    Wait(max2(0, THR_CS - max2(TSUR_A, TRD) - THR_A));
    SetPin(CSB, 1);
    Wait(max3(max8(THR_LD, THR_RWL, THR_RSB, THR_LD, THR_PG, THR_MR, THR_PS, THR_PD) - max2(0, THR_CS - max2(TSUR_A, TRD) - THR_A), THR_PD_CS, THR_PS_CS));
    SetPin(LOAD | MR | RWL, 0);
}

void DrvEfuseRRead(u32 *dst, int margin) {
    SetPin(STROBE | PS | PD | A(4) | RSB, 0);
    SetPin(PGENB | LOAD | RWL, 1);
    SetPin(MR, margin ? 1 : 0);
    Wait(max2(TSUR_PD_CS, TSUR_PS_CS));
    SetPin(CSB, 0);
    Wait(max2(TSUR_CS, max8(TSUR_PD, TSUR_PS, TSUR_MR, TSUR_PG, TSUR_LD, TSUR_RSB, TSUR_RWL, TSUR_A) - max2(TSUR_PD_CS, TSUR_PS_CS)));
    SetPin(STROBE, 1);
    Wait(max2(TRD, TSQ));
    SetPin(STROBE, 0);
    dst[0] = GetData();
    Wait(THR_A);
    SetPin(A(4), 1);
    Wait(max2(TSUR_A, TRD)); // note: the datasheet does not require a TRD wait here, but just to be safe
    SetPin(STROBE, 1);
    Wait(max2(TRD, TSQ));
    SetPin(STROBE, 0);
    dst[1] = GetData();
    Wait(max2(THR_CS, THR_A));
    SetPin(CSB, 1);
    Wait(max3(max8(THR_LD, THR_RWL, THR_RSB, THR_LD, THR_PG, THR_MR, THR_PS, THR_PD) - max2(THR_CS, THR_A), THR_PD_CS, THR_PS_CS));
    SetPin(LOAD | MR | RWL, 0);
}

void DrvEfusePowerDown() {
    SetPin(PD, 1);
    SetPin(PS | MR | STROBE | LOAD | RWL, 0);
    SetPin(CSB | PGENB, 1);
}

void DrvEfusePowerUp() {
    SetPin(PS | MR | STROBE | LOAD | RWL, 0);
    SetPin(CSB | PGENB, 1);
    SetPin(PD, 0);
}
