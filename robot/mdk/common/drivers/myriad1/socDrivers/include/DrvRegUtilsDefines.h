///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt              
///
/// @brief     Definitions and types needed by Reg utils library
/// 
#ifndef REG_UTILS_DEF_H
#define REG_UTILS_DEF_H 

// 1: Defines
// ----------------------------------------------------------------------------

// Register Manipulatin Macros
// ----------------------------------------------------------------------------

#define SET_REG_DWORD(a,x)  asm volatile( "std\t%0, [%1]" :: "r"((u64)(x)), "r"((unsigned)(a)) : "memory")
#define SET_REG_WORD(a,x)   ((void)(*(volatile u32*)(((unsigned)(a))) = (u32)(x)))
#define SET_REG_HALF(a,x)   ((void)(*(volatile u16*)(((unsigned)(a))) = (u16)(x)))
#define SET_REG_BYTE(a,x)   ((void)(*(volatile u8*)(a) = (u8)(x)))


#define GET_REG_DWORD(a,x)  ((void)((x) = *(volatile u64*)(((unsigned)(a)))))
#define GET_REG_WORD(a,x)   ((void)((x) = *(volatile u32*)(((unsigned)(a)))))
#define GET_REG_HALF(a,x)   ((void)((x) = *(volatile u16*)(((unsigned)(a)))))
#define GET_REG_BYTE(a,x)   ((void)((x) = *(volatile u8*)(a)))

#define GET_REG_DWORD_VAL(a) (*(volatile u64*)(((unsigned)(a))))
#define GET_REG_WORD_VAL(a) (*(volatile u32*)(((unsigned)(a))))
#define GET_REG_HALF_VAL(a) (*(volatile u16*)(((unsigned)(a))))
#define GET_REG_BYTE_VAL(a) (*(volatile u8*)(a))

#define SET_REG_BIT_NUM(a,offset)   ((void)(*(volatile u32*)(((unsigned)(a))) |= (u32)(1<<offset)))
#define CLR_REG_BIT_NUM(a,offset)   ((void)(*(volatile u32*)(((unsigned)(a))) &= (u32)~(1<<offset)))
#define SET_REG_BITS_MASK(a,set_mask)   ((void)(*(volatile u32*)(((unsigned)(a))) |= (u32)set_mask))
#define CLR_REG_BITS_MASK(a,clr_mask)   ((void)(*(volatile u32*)(((unsigned)(a))) &= ~(u32)clr_mask))

#define MS_WORD_64(a) (*(((volatile u32*)(&a)) + 0))
#define LS_WORD_64(a) (*(((volatile u32*)(&a)) + 1))

#define GEN_BIT_MASK(numBits) ( numBits >= 32U ? 0xFFFFFFFF : ((1U << numBits) - 1U) ) 

// 2: Typedefs (types, enums, structs)
// ----------------------------------------------------------------------------

// 3: Local const declarations     NB: ONLY const declarations go here
// ----------------------------------------------------------------------------

#endif // REG_UTILS_DEF_H
