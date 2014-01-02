/**
 * This is the private header of the shave loader module
 * 
 * @File                                        
 * @Author    Cormac Brick                     
 * @Brief     Simple Shave loader and window manager  private header
 * @copyright All code copyright Movidius Ltd 2012, all rights reserved 
 *            For License Warranty see: common/license.txt   
 * 
 */
#ifndef SWC_SHAVE_LOADER_PRIVATE_H
#define SWC_SHAVE_LOADER_PRIVATE_H

//Some Dma defines stay in this file
#include <swcDmaTypes.h>

/* ELF Header */

#define EI_NIDENT   16      /* Size of e_ident[] */

/* Values for section header, sh_type field.  */
#define SHT_NULL        0       /* Section header table entry unused */
#define SHT_PROGBITS    1       /* Program specific (private) data */
#define SHT_SYMTAB      2       /* Link editing symbol table */
#define SHT_STRTAB      3       /* A string table */
#define SHT_RELA        4       /* Relocation entries with addends */
#define SHT_HASH        5       /* A symbol hash table */
#define SHT_DYNAMIC     6       /* Information for dynamic linking */
#define SHT_NOTE        7       /* Information that marks file */
#define SHT_NOBITS      8       /* Section occupies no space in file */
#define SHT_REL         9       /* Relocation entries, no addends */
#define SHT_SHLIB       10      /* Reserved, unspecified semantics */
#define SHT_DYNSYM      11      /* Dynamic linking symbol table */
#define SHT_LOPROC  0x70000000  /* First of OS specific semantics */
#define SHT_HIPROC  0x7fffffff  /* Last of OS specific semantics */
#define SHT_LOUSER  0x80000000  /* First of OS specific semantics */
#define SHT_HIUSER  0xffffffff  /* Last of OS specific semantics */

typedef struct elfEhdr {
    u8  eIdent[EI_NIDENT]; /* ELF "magic number" */
    u16 eType;
    u16 eMachine;
    u32 eVersion;/*32 bit type here*/
    u32 eEntry;/*32 bit type*/
    u32 ePhoff;/*Program header offset*/
    u32 eShoff;/*Section header offset*/
    u32 eFlags;
    u16 eEhsize;
    u16 ePhentsize;
    u16 ePhnum;
    u16 eShentsize;
    u16 eShnum;
    u16 eShstrndx;
} Elf32Ehdr;

/* ELF Section header*/
typedef struct elfSectionHeader {
    u32 shName;
    u32 shType;
    u32 shFlags;
    u32 shAddr;
    u32 shOffset;
    u32 shSize;
    u32 shLink;
    u32 shInfo;
    u32 shAddralign;
    u32 shEntsize;
} Elf32Section;

/* ELF Program header */
typedef struct elfPhdr {
    u32   pType;         /* Identifies program segment type */
    //  u32 pFlags;        /* Flags position from binutils*/
    u32   pOffset;       /* Segment file offset */
    u32   pVaddr;        /* Segment virtual address */
    u32   pPaddr;        /* Segment physical address */
    u32   pFilesz;       /* Segment size in file */
    u32   pMemsz;        /* Segment size in memory */
    u32   pFlags;        /* Flags position from ELF standard spec*/
    u32   pAlign;        /* Segment alignment, file & memory */
} Elf32Phdr;

// Program space with no data (bss)
#define MSHT_NOBITS         8


typedef struct
{
    unsigned char     id[16];           // ID of the MOF file
    unsigned short    type;             // Type of the object file
    unsigned short    target;           // Target processor of the .mof file
    unsigned int      version;          // ISA version file version 
    unsigned int      entry;            // Entry point virtual address
    unsigned int      phOffset;         // Program header table file offset
    unsigned int      shOffset;         // Section header table file offset
    unsigned int      flags;            // Processor-specific flags
    unsigned short    phSize;           // MOF header size in bytes
    unsigned short    phEntrySize;      // Program header table entry size
    unsigned short    phCount;          // Program header table entry count
    unsigned short    shEntrySize;      // Section header table entry size
    unsigned short    shCount;          // Section header table entry count
    unsigned short    shStringIndex;    // Section header string table index
} tMofFileHeader;

//MOF Section Header
typedef struct
{
    unsigned int    name;               // Section name (string tbl index)
    unsigned int    type;               // Section type
    unsigned int    flags;              // Section flags
    unsigned int    address;            // Section virtual addr at execution
    unsigned int    offset;             // Section file offset
    unsigned int    size;               // Section size in bytes
    unsigned int    link;               // Link to another section
    unsigned int    info;               // Additional section information
    unsigned int    addralign;          // Section alignment
    unsigned int    entsize;            // Entry size if section holds table
    unsigned int    core;               // core of section used by linker
} tMofSectionHeader;


#endif



