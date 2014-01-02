///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     Driver for Toshiba mipi bridge chip TC358746
/// 
/// 
/// 

#ifndef IC_TC358746_H
#define IC_TC358746_H 

// 1: Includes
// ----------------------------------------------------------------------------
#include "icMipiTC358746Defines.h"
#include "DrvI2cMaster.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

extern const u64 icTC358746CommonDefaultRegisterFieldValues[];
extern const int icTC358746CommonDefaultRegisterFieldValuesCount;

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------
/// Reset Toshiba mipi bridge chip
/// Also it sets the ConfCtl__TriEn bit to Disabled, as it's a saner default for most cases.
/// @param[in] bridge Pointer to a TMipiBridge structure which has already been initialised
void icTC358746Reset(TMipiBridge *bridge);

/// Assigns an I2C device pointer to a specific bridge 
/// @param[in] bridge Pointer to a TMipiBridge structure which has already been initialised
/// @param[in] i2cDev pointer to an i2c device 
int icTC358746AssignI2c(TMipiBridge *bridge, I2CM_Device * i2cDev);

/// set the CS pin for a specific bridge (if it can set)
/// this function is useful only if ConfCtl__TriEn was left enabled in the MIPI configuration of an TX bridge.
/// @param[in] bridge Pointer to a TMipiBridge structure which has already been initialised
void icTC358746SetCs(TMipiBridge *bridge, int value);

/// Set registers on a specific bridge
/// @deprecated Please use icTC358746UpdateMultipleRegFields instead
/// @param[in] bridge Pointer to a TMipiBridge structure which has already been initialised
/// @param[in] data pointer to a table with a config of the MIPI bridge
/// @param[in] len number of registers to be written 
int icTC358746SetRegs(TMipiBridge *bridge, const u32 data[][2], u32 len);

/// Read registers on a specific bridge 
/// @param[in] bridge Pointer to a TMipiBridge structure which has already been initialised
/// @param[in] data pointer where to store the registers
/// @param[in] len number of registers to be written 
int icTC358746GetRegs(TMipiBridge *bridge, u32 data[][2], u32 len);

/// Checks registers on a specific bridge.
///
/// It returns 0 if everything has the same value as specified
/// Otherwise it returns 0x00010000 | address. Where address is the first
/// register which differs. It may also return 0x00100000 | address in case
/// of a communication failure.
/// This function is also used for reading of simple, non-numeric field values.
/// @deprecated please use icTC358746CheckMultipleRegFields instead
/// @param[in] bridge Pointer to a TMipiBridge structure which has already been initialised
/// @param[in] data pointer to a table with a config of the MIPI bridge
/// @param[in] len number of registers to be written
int icTC358746CheckRegs(TMipiBridge *bridge, const u32 data[][2], u32 len);

/// Checks register on a specific bridge.
///
/// It returns 0 if every specified field has the same value as given in the fieldspec argument
/// Otherwise it returns a bitmask of the differing bits
/// It may also return an I2C status number, in case of communication error.
/// @param[in] bridge Pointer to a TMipiBridge structure which has already been initialised
/// @param[in] field specification for the register that needs to be checked
/// @return Returns 0 if the register is correct, a bitmask of different bits if incorrect,
///    or a negative value on communication error.
int icTC358746CheckSingleRegFields(TMipiBridge *bridge, u64 fieldSpec);

/// Checks registers on a specific bridge.
///
/// It returns 0 if everything has the same value as specified
/// Otherwise it returns 0x00010000 | address. Where address is the first
/// register which differs. It may also return a negative value in case
/// of a communication failure.
/// @param[in] bridge Pointer to a TMipiBridge structure which has already been initialised
/// @param[in] data pointer to a table with a config of the MIPI bridge
/// @param[in] len number of registers to be checked
int icTC358746CheckMultipleRegFields(TMipiBridge *bridge, const u64 data[], u32 len);

/// Updates the field(s) of a register
///
/// @param[in] bridges Pointer to an TMipiBridgeCollection structure which has already been initialized
/// @param[in] field specification for the register updates
/// @return zero if successful, negative if I2C error, positive on other error.
int icTC358746UpdateSingleRegFields(TMipiBridge *bridge, u64 fieldSpec);

/// Updates the field(s) of multiple registers
///
/// @param[in] bridges Pointer to an TMipiBridgeCollection structure which has already been initialized
/// @param[in] data pointer to a table with a config of the MIPI bridge
/// @param[in] len number of registers to be updated
/// @return zero if successful, negative if I2C error, positive on other error.
int icTC358746UpdateMultipleRegFields(TMipiBridge *bridge, const u64 data[], u32 len);

/// Reads a numeric field
/// @param[in] bridges Pointer to an TMipiBridgeCollection structure which has already been initialized
/// @param[in] read field specification for the numeric field that is being read
/// @return negative on error, field value otherwise.
int icTC358746ReadNumericField(TMipiBridge *bridge, u64 readFieldSpec);

#endif // IC_TC358746_H  
