#include "DrvCmxDma.h"
#include <DrvRegUtils.h>
#include <DrvMutex.h>
#include <registersMyriad2.h>
#include <assert.h>

// TODO: IMPORTANT: L2 cache, and L1 cache coherency is not addressed at this point

void DrvCmxDmaInitState(struct CmxDmaState *state, void *cmxBuffer, u32 sizeInBytes)
{
    state -> CmxBuffer = cmxBuffer;
    state -> SizeInBytes = sizeInBytes;
    state -> UsedInterrupts = (1 << DRV_CMX_DMA_MAX_INTERRUPT_NUMBER);
    state -> FirstUsedByte = 0;
    state -> FirstFreeByte = 0;
    SET_REG_DWORD(CDMA_CTRL_ADR, DRV_CMX_DMA_CDMA_CTRL_SOFTWARE_RESET_MASK);
    SET_REG_DWORD(CDMA_CTRL_ADR, 0x0000);
    SET_REG_DWORD(CDMA_INT_EN_ADR, 0x0000);
    SET_REG_DWORD(CDMA_INT_RESET_ADR, (1 << (DRV_CMX_DMA_MAX_INTERRUPT_NUMBER + 1)) - 1);
    SET_REG_DWORD(CDMA_CTRL_ADR, DRV_CMX_DMA_CDMA_CTRL_ENABLE_DMA_MASK);
}

void DrvCmxDmaWaitUntilInterruptAndClear(int interrupt)
{
    assert(interrupt <= DRV_CMX_DMA_MAX_INTERRUPT_NUMBER);
    assert(interrupt >= 0);
    while ((GET_REG_DWORD_VAL(CDMA_INT_STATUS_ADR) & (1 << interrupt)) == 0)
        ; // wait until interrupt fires.
    SET_REG_DWORD(CDMA_INT_RESET_ADR, (1 << interrupt));
}

int DrvCmxDmaTryAllocateInterrupt(struct CmxDmaState *state)
{
    int i;
    DrvMutexLock(DRV_CMX_DMA_MUTEX);
    for (i=0; i<=DRV_CMX_DMA_MAX_INTERRUPT_NUMBER; i++)
    {
        if ((state -> UsedInterrupts & (1 << i)) == 0)
        {
            state -> UsedInterrupts |= 1 << i;
            break;
        }
    }
    DrvMutexUnlock(DRV_CMX_DMA_MUTEX);
    if (i > DRV_CMX_DMA_MAX_INTERRUPT_NUMBER) i = -1;
    return i;
}

int DrvCmxDmaAllocateInterrupt(struct CmxDmaState *state) {
    int interrupt;
    do {
        while (((volatile struct CmxDmaState *)state)->UsedInterrupts == 0xffffffffu)
            ;
        interrupt = DrvCmxDmaTryAllocateInterrupt(state);
    } while (interrupt == -1);
    return interrupt;
}

int DrvCmxDmaTryClaimInterrupt(struct CmxDmaState *state, int claimedInterrupt)
{
    assert(claimedInterrupt <= DRV_CMX_DMA_MAX_INTERRUPT_NUMBER);
    assert(claimedInterrupt >= 0);
    DrvMutexLock(DRV_CMX_DMA_MUTEX);
    int alreadyUsed = (state -> UsedInterrupts) & (1 << claimedInterrupt);
    state -> UsedInterrupts |= 1 << claimedInterrupt;
    DrvMutexUnlock(DRV_CMX_DMA_MUTEX);
    if (alreadyUsed)
        return -1;
    else
        return claimedInterrupt;
}

void DrvCmxDmaFreeInterrupt(struct CmxDmaState *state, int interrupt)
{
    assert(interrupt <= DRV_CMX_DMA_MAX_INTERRUPT_NUMBER);
    assert(interrupt >= 0);
    DrvMutexLock(DRV_CMX_DMA_MUTEX);
    state -> UsedInterrupts &= ~ (1 << interrupt);
    DrvMutexUnlock(DRV_CMX_DMA_MUTEX);
}

int DrvCmxDmaRegisterAgentIsBusy()
{
    return GET_REG_DWORD_VAL(CDMA_STATUS_ADR) & DRV_CMX_DMA_CDMA_STATUS_REGISTER_COMMAND_AGENT_BUSY;
}

static void DrvCmxDmaLockMutexAndMakeSureRegisterAgentIsIdle()
{
    while (1)
    {
        while (DrvCmxDmaRegisterAgentIsBusy())
            ; // Wait for register agent to become free
        DrvMutexLock(DRV_CMX_DMA_MUTEX);
        if (DrvCmxDmaRegisterAgentIsBusy())
        {
            // In case of a race condition and it became used right after obtaining the lock, then relese and try again
            DrvMutexUnlock(DRV_CMX_DMA_MUTEX);
        }
        else
        {
            // The register agent is still free. We can proceed to using it.
            break;
        }
    }
}

void DrvCmxDmaRegisterAgentPostCommandNonBlocking(void *transaction, int keepInterrupt)
{
    DrvCmxDmaLockMutexAndMakeSureRegisterAgentIsIdle();
    u64 cdmaCfg = GET_REG_DWORD_VAL(CDMA_CFG_LINK_ADR);
    cdmaCfg &= 0x00000000ffffffffULL;
    cdmaCfg |= ((u64 *)transaction)[0] & 0xffffffff00000000ULL;
    if (!keepInterrupt)
    {
        cdmaCfg &= ~DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_MASK;
        cdmaCfg |= (unsigned long long)(DRV_CMX_DMA_IGNORED_INTERRUPT_NUMBER) << DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_SHIFT;
    }
    SET_REG_DWORD(CDMA_CFG_LINK_ADR, cdmaCfg);
    SET_REG_DWORD(CDMA_DST_SRC_ADR, ((u64 *)transaction)[1]);
    SET_REG_DWORD(CDMA_LEN_ADR, ((u64 *)transaction)[2]);
    if (cdmaCfg & DRV_CMX_DMA_CDMA_CFG_LINK_TRANSACTION_TYPE_MASK)
    {
        SET_REG_DWORD(CDMA_SRC_STRIDE_WIDTH_ADR, ((u64 *)transaction)[3]);
        SET_REG_DWORD(CDMA_DST_STRIDE_WIDTH_ADR, ((u64 *)transaction)[4]);
    }
    // TODO: it's possible that this is the only needed critical section:
    SET_REG_DWORD(CDMA_CTRL_ADR, GET_REG_DWORD_VAL(CDMA_CTRL_ADR) | DRV_CMX_DMA_CDMA_CTRL_START_CONTROL_REGISTER_AGENT_MASK);

    DrvMutexUnlock(DRV_CMX_DMA_MUTEX);
}

void DrvCmxDmaRegisterAgentPostCommandBlocking(struct CmxDmaState *state, void *transaction)
{
    u32 interrupt = DrvCmxDmaAllocateInterrupt(state);
    u64 cdmaCfg = ((u64 *) transaction)[0];
    cdmaCfg &= ~DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_MASK;
    cdmaCfg |= ((unsigned long long) interrupt) << DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_SHIFT;
    ((u64 *) transaction)[0] = cdmaCfg;
    DrvCmxDmaRegisterAgentPostCommandNonBlocking(transaction, DRV_CMX_DMA_KEEP_INTERRUPT);
    DrvCmxDmaWaitUntilInterruptAndClear(interrupt);
    DrvCmxDmaFreeInterrupt(state, interrupt);
}

static inline void DrvCmxDmaConstructSimpleTransaction(struct CmxDmaTransaction1D *transaction, void *dst, const void *src, u32 len)
{
    transaction -> Src = (void *)src;
    transaction -> Dst = dst;
    transaction -> Len = len;
    transaction -> unused = 0;
    transaction -> CfgLink.FullRegister =
        ((u64)DRV_CMX_DMA_DEFAULT_TRANSACTION_PRIORITY << DRV_CMX_DMA_CDMA_CFG_LINK_TRANSACTION_PRIORITY_SHIFT) |
        ((u64)DRV_CMX_DMA_DEFAULT_BURST_LENGTH << DRV_CMX_DMA_CDMA_CFG_LINK_BURST_LENGTH_SHIFT);
}

void  DrvCmxDmaRegisterAgentMemcpyNonBlocking(void *dst, const void *src, u32 len)
{
    struct CmxDmaTransaction1D transaction;
    DrvCmxDmaConstructSimpleTransaction(&transaction, dst, src, len);
    DrvCmxDmaRegisterAgentPostCommandNonBlocking(&transaction, DRV_CMX_DMA_IGNORE_INTERRUPT);
}

void  DrvCmxDmaRegisterAgentMemcpyBlocking(struct CmxDmaState *state, void *dst, const void *src, u32 len)
{
    struct CmxDmaTransaction1D transaction;
    DrvCmxDmaConstructSimpleTransaction(&transaction, dst, src, len);
    DrvCmxDmaRegisterAgentPostCommandBlocking(state, &transaction);
}

void DrvCmxDmaLinkAgentPostCommandCmxPointerNonBlocking(void *transaction, int keepInterrupt)
{
    u64 cdmaCfg = 0;
    cdmaCfg |= ((u64 *)transaction)[0] & 0xffffffff00000000ULL;
    if (!keepInterrupt)
    {
        cdmaCfg &= ~DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_MASK;
        cdmaCfg |= (u64) DRV_CMX_DMA_IGNORED_INTERRUPT_NUMBER << DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_SHIFT;
    }
    ((u64 *)transaction)[0] = cdmaCfg;
    DrvMutexLock(DRV_CMX_DMA_MUTEX);
    SET_REG_DWORD(CDMA_CTRL_ADR, GET_REG_DWORD_VAL(CDMA_CTRL_ADR) & ~DRV_CMX_DMA_CDMA_CTRL_ENABLE_DMA_MASK);
    u32 transactionAtTop = GET_REG_DWORD_VAL(CDMA_TOP0_ADR);
    if (transactionAtTop == 0)
    {
        // This transaction will be the next one at top
        SET_REG_DWORD(CDMA_CFG_LINK_ADR, (u32) transaction);
    } else {
        // Walk the linked list to the last transaction
        SET_REG_DWORD(CDMA_CFG_LINK_ADR, transactionAtTop);
        u32 father = transactionAtTop;
        while ((*(u32 *)father)!=0)
            father = *(u32 *)father;
        *(u32 *)father = (u32) transaction;
    }
    SET_REG_DWORD(CDMA_CTRL_ADR, GET_REG_DWORD_VAL(CDMA_CTRL_ADR) | DRV_CMX_DMA_CDMA_CTRL_ENABLE_DMA_MASK);
    SET_REG_DWORD(CDMA_CTRL_ADR, GET_REG_DWORD_VAL(CDMA_CTRL_ADR) | DRV_CMX_DMA_CDMA_CTRL_START_LINK_COMMAND_AGENT_MASK);
    DrvMutexUnlock(DRV_CMX_DMA_MUTEX);
}

void DrvCmxDmaLinkAgentPostCommandCmxPointerBlocking(struct CmxDmaState *state, void *transaction)
{
    u32 interrupt = DrvCmxDmaAllocateInterrupt(state);
    u64 cdmaCfg = ((u64 *) transaction)[0];
    cdmaCfg &= ~DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_MASK;
    cdmaCfg |= (u64) (interrupt) << DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_SHIFT;
    ((u64 *) transaction)[0] = cdmaCfg;
    DrvCmxDmaLinkAgentPostCommandCmxPointerNonBlocking(transaction, DRV_CMX_DMA_KEEP_INTERRUPT);
    DrvCmxDmaWaitUntilInterruptAndClear(interrupt);
    DrvCmxDmaFreeInterrupt(state, interrupt);
}

static u32 DrvCmxDmaLockMutexAndMakeSureFreeWordsThereAreAtLeast(struct CmxDmaState *state, u32 words)
{
    while (1) {
        DrvMutexLock(DRV_CMX_DMA_MUTEX);
        SET_REG_DWORD(CDMA_CTRL_ADR, GET_REG_DWORD_VAL(CDMA_CTRL_ADR) & ~DRV_CMX_DMA_CDMA_CTRL_ENABLE_DMA_MASK);
        u32 transactionAtTop = GET_REG_DWORD_VAL(CDMA_TOP0_ADR);
        if (transactionAtTop == 0)
        {
            state -> FirstUsedByte = 0;
            state -> FirstFreeByte = 0;
        } else {
            // Walk the linked list to the last transaction
            u32 trans = transactionAtTop;
            while (trans)
            {
                if ((trans >= (u32) state -> CmxBuffer) &&
                    (trans < (((u32) state -> CmxBuffer) + state -> SizeInBytes)))
                    {
                        break;
                    }
                trans = *(u32 *)trans;
            }
            if (trans == 0)
            {
                state -> FirstUsedByte = 0;
                state -> FirstFreeByte = 0;
            } else
                state -> FirstUsedByte = trans - (u32) state -> CmxBuffer;
        }
        if (state -> FirstFreeByte + words * 4 < state -> FirstUsedByte)
            return state -> FirstFreeByte;
        if (state -> FirstFreeByte >= state -> FirstUsedByte)
        {
            if ((state -> FirstFreeByte + words * 4 <= state -> SizeInBytes) &&
                (state -> FirstFreeByte + words * 4 < state -> SizeInBytes + state -> FirstUsedByte))
                return state -> FirstFreeByte;
            if (words * 4 < state -> FirstUsedByte)
                return 0;
        }
        DrvMutexUnlock(DRV_CMX_DMA_MUTEX);
        while (transactionAtTop == GET_REG_DWORD_VAL(CDMA_TOP0_ADR))
            ; // this is to make sure that we hold the mutex as little as possible
        DrvMutexLock(DRV_CMX_DMA_MUTEX);
    }
}

void DrvCmxDmaLinkAgentPostCommandNonBlocking(struct CmxDmaState *state, void *transaction, int keepInterrupt)
{
    u32 offset = 0;
    u32 words = 6;
    if (((u64 *)transaction)[0] & DRV_CMX_DMA_CDMA_CFG_LINK_TRANSACTION_TYPE_MASK)
    {
        words = 10;
    }
    offset = DrvCmxDmaLockMutexAndMakeSureFreeWordsThereAreAtLeast(state, words);
    u32 newFirstFreeByte = offset + 4 * words;
    u32 i;
    for (i=0; i<words; i++) {
        *(u32 *)(state -> CmxBuffer + offset + i*4) = *(((u32 *)transaction) + i);
    }
    state -> FirstFreeByte = newFirstFreeByte;
    DrvCmxDmaLinkAgentPostCommandCmxPointerNonBlocking((void *)(state -> CmxBuffer + offset + i*4), keepInterrupt);
    DrvMutexUnlock(DRV_CMX_DMA_MUTEX);
}

void DrvCmxDmaLinkAgentPostCommandBlocking(struct CmxDmaState *state, void *transaction)
{
    u32 interrupt = DrvCmxDmaAllocateInterrupt(state);
    u64 cdmaCfg = ((u64 *) transaction)[0];
    cdmaCfg &= ~DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_MASK;
    cdmaCfg |= (u64)interrupt << DRV_CMX_DMA_CDMA_CFG_LINK_INTERRUPT_SHIFT;
    ((u64 *) transaction)[0] = cdmaCfg;
    DrvCmxDmaLinkAgentPostCommandNonBlocking(state, transaction, DRV_CMX_DMA_KEEP_INTERRUPT);
    DrvCmxDmaWaitUntilInterruptAndClear(interrupt);
    DrvCmxDmaFreeInterrupt(state, interrupt);
}

void DrvCmxDmaLinkAgentMemcpyNonBlocking(struct CmxDmaState *state, void *dst, const void *src, u32 len)
{
    struct CmxDmaTransaction1D transaction;
    DrvCmxDmaConstructSimpleTransaction(&transaction, dst, src, len);
    DrvCmxDmaLinkAgentPostCommandNonBlocking(state, &transaction, DRV_CMX_DMA_IGNORE_INTERRUPT);
}

void DrvCmxDmaLinkAgentMemcpyBlocking(struct CmxDmaState *state, void *dst, const void *src, u32 len)
{
    struct CmxDmaTransaction1D transaction;
    DrvCmxDmaConstructSimpleTransaction(&transaction, dst, src, len);
    DrvCmxDmaLinkAgentPostCommandBlocking(state, &transaction);
}
