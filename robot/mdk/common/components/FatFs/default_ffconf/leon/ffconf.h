#ifndef CUSTOMIZED_FFCONF
#define CUSTOMIZED_FFCONF

#include "ffconf_defaults.h"
// Sensible defaults are in ffconf_defaults.h.
// If you want to customize anything, here you can #undef it, and then redfine it:

/*---------------------------------------------------------------------------/
/ Functions and Buffer Configurations
/----------------------------------------------------------------------------*/

//#undef _FS_TINY
//#define _FS_TINY		0	/* 0:Normal or 1:Tiny */
/* When _FS_TINY is set to 1, FatFs uses the sector buffer in the file system
/  object instead of the sector buffer in the individual file object for file
/  data transfer. This reduces memory consumption 512 bytes each file object. */

//#undef _FS_READONLY
//#define _FS_READONLY	0	/* 0:Read/Write or 1:Read only */
/* Setting _FS_READONLY to 1 defines read only configuration. This removes
/  writing functions, f_write, f_sync, f_unlink, f_mkdir, f_chmod, f_rename,
/  f_truncate and useless f_getfree. */

//#undef _FS_MINIMIZE
//#define _FS_MINIMIZE	3	/* 0 to 3 */
/* The _FS_MINIMIZE option defines minimization level to remove some functions.
/
/   0: Full function.
/   1: f_stat, f_getfree, f_unlink, f_mkdir, f_chmod, f_truncate and f_rename
/      are removed.
/   2: f_opendir and f_readdir are removed in addition to 1.
/   3: f_lseek is removed in addition to 2. */


//#undef _USE_STRFUNC
//#define _USE_STRFUNC	0	/* 0:Disable or 1-2:Enable */
/* To enable string functions, set _USE_STRFUNC to 1 or 2. */

//#undef _USE_MKFS
//#define _USE_MKFS		0	/* 0:Disable or 1:Enable */
/* To enable f_mkfs function, set _USE_MKFS to 1 and set _FS_READONLY to 0 */

//#undef _USE_FORWARD
//#define _USE_FORWARD	0	/* 0:Disable or 1:Enable */
/* To enable f_forward function, set _USE_FORWARD to 1 and set _FS_TINY to 1. */

//#undef _USE_FASTSEEK
//#define _USE_FASTSEEK	0	/* 0:Disable or 1:Enable */
/* To enable fast seek feature, set _USE_FASTSEEK to 1. */



/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/----------------------------------------------------------------------------*/

//#undef _CODE_PAGE
//#define _CODE_PAGE	932
/* The _CODE_PAGE specifies the OEM code page to be used on the target system.
/  Incorrect setting of the code page can cause a file open failure.
/
/   932  - Japanese Shift-JIS (DBCS, OEM, Windows)
/   936  - Simplified Chinese GBK (DBCS, OEM, Windows)
/   949  - Korean (DBCS, OEM, Windows)
/   950  - Traditional Chinese Big5 (DBCS, OEM, Windows)
/   1250 - Central Europe (Windows)
/   1251 - Cyrillic (Windows)
/   1252 - Latin 1 (Windows)
/   1253 - Greek (Windows)
/   1254 - Turkish (Windows)
/   1255 - Hebrew (Windows)
/   1256 - Arabic (Windows)
/   1257 - Baltic (Windows)
/   1258 - Vietnam (OEM, Windows)
/   437  - U.S. (OEM)
/   720  - Arabic (OEM)
/   737  - Greek (OEM)
/   775  - Baltic (OEM)
/   850  - Multilingual Latin 1 (OEM)
/   858  - Multilingual Latin 1 + Euro (OEM)
/   852  - Latin 2 (OEM)
/   855  - Cyrillic (OEM)
/   866  - Russian (OEM)
/   857  - Turkish (OEM)
/   862  - Hebrew (OEM)
/   874  - Thai (OEM, Windows)
/	1    - ASCII only (Valid for non LFN cfg.)
*/

//#undef _USE_LFN
//#define _USE_LFN	0		/* 0 to 3 */
//#undef _MAX_LFN
//#define _MAX_LFN	255		/* Maximum LFN length to handle (12 to 255) */
/* The _USE_LFN option switches the LFN support.
/
/   0: Disable LFN feature. _MAX_LFN and _LFN_UNICODE have no effect.
/   1: Enable LFN with static working buffer on the BSS. Always NOT reentrant.
/   2: Enable LFN with dynamic working buffer on the STACK.
/   3: Enable LFN with dynamic working buffer on the HEAP.
/
/  The LFN working buffer occupies (_MAX_LFN + 1) * 2 bytes. To enable LFN,
/  Unicode handling functions ff_convert() and ff_wtoupper() must be added
/  to the project. When enable to use heap, memory control functions
/  ff_memalloc() and ff_memfree() must be added to the project. */

//#undef _LFN_UNICODE
//#define _LFN_UNICODE	0	/* 0:ANSI/OEM or 1:Unicode */
/* To switch the character code set on FatFs API to Unicode,
/  enable LFN feature and set _LFN_UNICODE to 1. */

//#undef _FS_RPATH
//#define _FS_RPATH		0	/* 0 to 2 */
/* The _FS_RPATH option configures relative path feature.
/
/   0: Disable relative path feature and remove related functions.
/   1: Enable relative path. f_chdrive() and f_chdir() are available.
/   2: f_getcwd() is available in addition to 1.
/
/  Note that output of the f_readdir fnction is affected by this option. */



/*---------------------------------------------------------------------------/
/ Physical Drive Configurations
/----------------------------------------------------------------------------*/

//#undef _VOLUMES
//#define _VOLUMES	1
/* Number of volumes (logical drives) to be used. */

//#undef _MAX_SS
//#define _MAX_SS		512		/* 512, 1024, 2048 or 4096 */
/* Maximum sector size to be handled.
/  Always set 512 for memory card and hard disk but a larger value may be
/  required for on-board flash memory, floppy disk and optical disk.
/  When _MAX_SS is larger than 512, it configures FatFs to variable sector size
/  and GET_SECTOR_SIZE command must be implememted to the disk_ioctl function. */

//#undef _MULTI_PARTITION
//#define _MULTI_PARTITION	0	/* 0:Single partition, 1/2:Enable multiple partition */
/* When set to 0, each volume is bound to the same physical drive number and
/ it can mount only first primaly partition. When it is set to 1, each volume
/ is tied to the partitions listed in VolToPart[]. */


#endif /* CUSTOMIZED_FFCONF */
