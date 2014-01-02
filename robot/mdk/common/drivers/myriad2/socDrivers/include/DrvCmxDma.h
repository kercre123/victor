///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     API for accessing the CMX DMA controller.
///

#ifndef DRV_CMX_DMA_H
#define DRV_CMX_DMA_H

#include "DrvCmxDmaDefines.h"

/// @brief Initialize the persistent state used by this driver.
///
/// @warning Only one of the cores is allowed to call this function.
/// The rest of the cores must grab a pointer to the CmxDmaState structure
/// and use it after it's initialized.
///
/// Accessing the members of the state structure is protected by the hardware mutex
/// that is defined by the #DRV_CMX_DMA_MUTEX macro.
///
/// @param[out] state       Pointer to a not-yet initialized CmxDmaState structure
/// @param[in]  cmxBuffer   Pointer to a buffer in CMX that will hold linked commands
/// @param[in]  sizeInBytes Size of CmxBuffer in bytes
void DrvCmxDmaInitState(struct CmxDmaState *state, void *cmxBuffer, u32 sizeInBytes);

/// @brief Wait until a CmxDma interrupt fires, and then clear it.
///
/// @param[in] interrupt Number of the CmxDma interrupt (0..15)
void DrvCmxDmaWaitUntilInterruptAndClear(int interrupt);

/// @brief Try allocating an interrupt number.
///
/// If all the interrups are used, then return -1
///
/// @param[in] state Persistent state of the CmxDma driver
/// @return the interrupt number (0..14), or -1 if failed
int DrvCmxDmaTryAllocateInterrupt(struct CmxDmaState *state);

/// @brief Allocate an interrupt number.
///
/// If all the interrupts are used, then wait until one becomes available
///
/// @param[in] state Persistent state of the CmxDma driver
/// @return the interrupt number (0..14)
int DrvCmxDmaAllocateInterrupt(struct CmxDmaState *state);

/// @brief Try allocating an explicitly specified interrupt number.
///
/// @param[in] state Persistent state of the CmxDma driver
/// @param[in] claimedInterrupt The interrupt number that you are requesting
/// @return the interrupt number that you requested, or -1 if it was already in use.
int DrvCmxDmaTryClaimInterrupt(struct CmxDmaState *state, int claimedInterrupt);

/// @brief Free an allocated interrupt
///
/// @param[in] state Persistent state of the CmxDma driver
/// @param[in] interrupt The interrupt number that you want to free
void DrvCmxDmaFreeInterrupt(struct CmxDmaState *state, int interrupt);

/// @brief Check if the register agent is busy or not.
///
/// Note: you probably don't need to use this function, as the other
/// functions that touch the register agent will make sure that it's
/// not busy before they do anything.
/// @return non-zero if the register agent is busy.
int DrvCmxDmaRegisterAgentIsBusy();

/// @brief Post a command descriptor to the register agent.
///
/// If the register agent is busy, then this call will busy-wait for it to be freed.
/// @param[in] transaction a pointer to a CmxDmaTransaction1D or CmxDmaTransaction2D structure.
///     The structure can be anywhere in memory.
/// @param[in] keepInterrupt if this parameter is non-zero, then the interrupt field
///     of the command descriptor is not modified. Otherwise the interrupt field
///     is set to the reserved interrupt number, to make sure that it is ignored.
///     If you use keepInterrupt = non-zero, then make sure that you properly
///     allocated the interrupt that you are using.
void DrvCmxDmaRegisterAgentPostCommandNonBlocking(void *transaction, int keepInterrupt);

/// @brief post a command descriptor to the register agent, and wait until the DMA transaction
///   is complete
///
/// If the register agent is busy, then this call will busy-wait for it to be freed.
/// This call allocates an interrupt temporarily, and uses it to tell when the DMA transaction
/// is finished. After the transaction is finished, it deallocates the interrupt, and it returns.
///
/// @param[in] state Persistent state of the CmxDma driver
/// @param[in,out] transaction a pointer to a CmxDmaTransaction1D or CmxDmaTransaction2D structure.
///     The structure can be anywhere in memory. It will be modified to reflect the used interrupt.
void DrvCmxDmaRegisterAgentPostCommandBlocking(struct CmxDmaState *state, void *transaction);

/// @brief Copy memory area using CMX DMA Register Agent, without waiting for the operation to finish.
///
/// This function has a similar signature to the standard memcpy function.
/// If the register agent is busy, then this call will busy-wait for it to be freed.
/// Then it will proceed to constructing a transaction command descriptor based on the arguments.
/// @param[out] dst Destination pointer
/// @param[in] src Source pointer
/// @param[in] len Length of transaction in bytes
void DrvCmxDmaRegisterAgentMemcpyNonBlocking(void *dst, const void *src, u32 len);

/// @brief Copy memory area using CMX DMA Register Agent, and wait for the transaction to complete.
///
/// This function has a similar signature to the standard memcpy function, except it also has
/// a state argument, which is used to temporarily allocate an interrupt for this transfer.
/// If the register agent is busy, then this call will busy-wait for it to be freed.
/// Then it will proceed to constructing a transaction command descriptor based on the arguments.
/// Then it waits for the temporarily allocated interrupt to trigger, and then it clears
/// and deallocates the interrupt.
/// @param[in] state Persistent state of the CmxDma driver
/// @param[out] dst Destination pointer
/// @param[in] src Source pointer
/// @param[in] len Length of transaction in bytes
void DrvCmxDmaRegisterAgentMemcpyBlocking(struct CmxDmaState *state, void *dst, const void *src, u32 len);

/// @brief Post a command descriptor that is in CMX memory to the link agent.
///
/// The given transaction command is inserted to the end of the existing linked list.
/// @param[in,out] transaction a pointer to a CmxDmaTransaction1D or CmxDmaTransaction2D structure.
///     The structure MUST be in the CMX.
///     If keepInterrupt == 0 , then it will be modified to reflect the used (reserved) interrupt.
/// @param[in] keepInterrupt if this parameter is non-zero, then the interrupt field
///     of the command descriptor is not modified. Otherwise the interrupt field
///     is set to the reserved interrupt number, to make sure that it is ignored.
///     If you use keepInterrupt = non-zero, then make sure that you properly
///     allocated the interrupt that you are using.
void DrvCmxDmaLinkAgentPostCommandCmxPointerNonBlocking(void *transaction, int keepInterrupt);

/// @brief Post a command descriptor that is in CMX memory to the link agent,
///     and wait for the transaction to complete.
///
/// This function is similar to #DrvCmxDmaLinkAgentPostCommandCmxPointerNonBlocking,
/// except a temporary interrupt is allocated, and used to detect when the transaction is
/// finished, after which the interrupt is deallocated.
/// @param[in] state Persistent state of the CmxDma driver
/// @param[in,out] transaction a pointer to a CmxDmaTransaction1D or CmxDmaTransaction2D structure.
///     The structure MUST be in the CMX. It will be modified to reflect the used interrupt.
void DrvCmxDmaLinkAgentPostCommandCmxPointerBlocking(struct CmxDmaState *state, void *transaction);

/// @brief Post a command descriptor to the link agent.
///
/// This function is similar to #DrvCmxDmaLinkAgentPostCommandCmxPointerNonBlocking,
/// except the transaction command descriptor may live anywhere in memory, not just
/// in the CMX. It's contents will be copied to the ring buffer in the state structure.
/// If there is no space in the state structure, then this function will busy-loop.
/// @param[in] state Persistent state of the CmxDma driver
/// @param[in] transaction a pointer to a CmxDmaTransaction1D or CmxDmaTransaction2D structure.
///     The structure may be anywhere in memory.
/// @param[in] keepInterrupt if this parameter is non-zero, then the interrupt field
///     of the command descriptor is not modified. Otherwise the interrupt field
///     is set to the reserved interrupt number, to make sure that it is ignored.
///     If you use keepInterrupt = non-zero, then make sure that you properly
///     allocated the interrupt that you are using.
void DrvCmxDmaLinkAgentPostCommandNonBlocking(struct CmxDmaState *state, void *transaction, int keepInterrupt);

/// @brief Post a command descriptor to the link agent, and wait for the transaction to complete.
///
/// This function is similar to #DrvCmxDmaLinkAgentPostCommandNonBlocking,
/// except a temporary interrupt is allocated, and used to detect when the transaction is
/// finished, after which the interrupt is deallocated.
/// @param[in] state Persistent state of the CmxDma driver
/// @param[in,out] transaction a pointer to a CmxDmaTransaction1D or CmxDmaTransaction2D structure.
///     The structure MUST be in the CMX. It will be modified to reflect the used interrupt.
void DrvCmxDmaLinkAgentPostCommandBlocking(struct CmxDmaState *state, void *transaction);

/// @brief Copy memory area using CMX DMA Link Agent, without waiting for the operation to finish.
///
/// A transaction command descriptor is constructed and inserted into the ring buffer inside
/// the CmxDmaState structure.
/// @param[in] state Persistent state of the CmxDma driver
/// @param[out] dst Destination pointer
/// @param[in] src Source pointer
/// @param[in] len Length of transaction in bytes
void DrvCmxDmaLinkAgentMemcpyNonBlocking(struct CmxDmaState *state, void *dst, const void *src, u32 len);

/// @brief Copy memory area using CMX DMA Link Agent, and wait for the transaction to complete.
///
/// A transaction command descriptor is constructed and inserted into the ring buffer inside
/// the CmxDmaState structure.
/// Also, a temporary interrupt is allocated, and used to detect when the transaction is
/// finished, after which the interrupt is deallocated.
/// @param[in] state Persistent state of the CmxDma driver
/// @param[out] dst Destination pointer
/// @param[in] src Source pointer
/// @param[in] len Length of transaction in bytes
void DrvCmxDmaLinkAgentMemcpyBlocking(struct CmxDmaState *state, void *dst, const void *src, u32 len);

#endif
