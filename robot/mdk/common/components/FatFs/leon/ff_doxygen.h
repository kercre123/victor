///
/// @file
/// @copyright Copyright (C) 2012, ChaN, all right reserved.
///
/// @addtogroup FatFS
/// @{
/// * The FatFs module is a free software and there is NO WARRANTY.
/// * No restriction on use. You can use, modify and redistribute it for
///   personal, non-profit or commercial product UNDER YOUR RESPONSIBILITY.
/// * Redistributions of source code must retain the above copyright notice.
///
/// @brief FAT file system module.
///     Please see this URL: http://elm-chan.org/fsw/ff/00index_e.html
///     for more detailed documentation.

/// FatFs module is a generic FAT file system module for small embedded systems.
/// This is a free software that opened for education, research and commercial
/// developments under license policy of following terms.

/** @brief Multiple partition configuration */
typedef struct {
	BYTE pd;	/**< Physical drive number */
	BYTE pt;	/**< Partition: 0:Auto detect, 1-4:Forced partition) */
} PARTITION;

/** @brief Volume - Partition resolution table */
extern PARTITION VolToPart[];	
/** @brief Get physical drive number */
#define LD2PD(vol) (VolToPart[vol].pd)
/** @brief Get partition index */
#define LD2PT(vol) (VolToPart[vol].pt)

/** @brief metadata must always be big-endian*/
#define INTERNAL_METADATA_SECTOR_BUFFER_TYPE be_pointer

/** @brief for various reasons you might redefine INTERNAL_FILEDATA_SECTOR_BUFFER_TYPE
  * in the Makefile
  */
#ifndef INTERNAL_FILEDATA_SECTOR_BUFFER_TYPE
#define INTERNAL_FILEDATA_SECTOR_BUFFER_TYPE le_pointer
#endif

/** @brief File system object structure (FATFS) */
typedef struct {
	BYTE	fs_type;		/**< @brief FAT sub-type (0:Not mounted) */
	BYTE	drv;			/**< @brief Physical drive number */
	BYTE	csize;			/**< @brief Sectors per cluster (1,2,4...128) */
	BYTE	n_fats;			/**< @brief Number of FAT copies (1,2) */
	BYTE	wflag;			/**< @brief win[] dirty flag (1:must be written back) */
	BYTE	fsi_flag;		/**< @brief fsinfo dirty flag (1:must be written back) */
	WORD	id;			/**< @brief File system mount ID */
	WORD	n_rootdir;		/**< @brief Number of root directory entries (FAT12/16) */
#if _MAX_SS != 512
	WORD	ssize;			/**< @brief Bytes per sector (512, 1024, 2048 or 4096) */
#endif
#if _FS_REENTRANT
	_SYNC_t	sobj;			/**< @brief Identifier of sync object */
#endif
#if !_FS_READONLY
	DWORD	last_clust;		/**< @brief Last allocated cluster */
	DWORD	free_clust;		/**< @brief Number of free clusters */
	DWORD	fsi_sector;		/**< @brief fsinfo sector (FAT32) */
#endif
#if _FS_RPATH
	DWORD	cdir;			/**< @brief Current directory start cluster (0:root) */
#endif
	DWORD	n_fatent;		/**< @brief Number of FAT entries (= number of clusters + 2) */
	DWORD	fsize;			/**< @brief Sectors per FAT */
	DWORD	fatbase;		/**< @brief FAT start sector */
	DWORD	dirbase;		/**< @brief Root directory start sector (FAT32:Cluster#) */
	DWORD	database;		/**< @brief Data start sector */
	DWORD	winsect;		/**< @brief Current sector appearing in the win[] */
	BYTE	win[_MAX_SS];		/**< @brief Disk access window for Directory, FAT (and Data on tiny cfg) */
	pointer_type win_pt;		/**< @brief This holds the data type of the information that is stored in the window (DT_FILEDATA/DT_METADATA) */
} FATFS;



/** @brief File object structure (FIL) */
typedef struct {
	FATFS*	fs;			/**< @brief Pointer to the related file system object */
	WORD	id;			/**< @brief File system mount ID of the related file system object */
	BYTE	flag;			/**< @brief File status flags */
	BYTE	pad1;
	DWORD	fptr;			/**< @brief File read/write pointer (0ed on file open) */
	DWORD	fsize;			/**< @brief File size */
	DWORD	sclust;			/**< @brief File data start cluster (0:no data cluster, always 0 when fsize is 0) */
	DWORD	clust;			/**< @brief Current cluster of fpter */
	DWORD	dsect;			/**< @brief Current data sector of fpter */
#if !_FS_READONLY
	DWORD	dir_sect;		/**< @brief Sector containing the directory entry */
	BYTE*	dir_ptr;		/**< @brief Pointer to the directory entry in the window */
#endif
#if _USE_FASTSEEK
	DWORD*	cltbl;			/**< @brief Pointer to the cluster link map table (null on file open) */
#endif
#if _FS_LOCK
	UINT	lockid;			/**< @brief File lock ID (index of file semaphore table Files[]) */
#endif
#if !_FS_TINY
	BYTE	buf[_MAX_SS];		/**< @brief File data read/write buffer
                                             the endianness of buf is also INTERNAL_SECTOR_BUFFER_TYPE */
#endif
} FIL;



/** @brief Directory object structure (DIR) */
typedef struct {
	FATFS*	fs;			/**< @brief Pointer to the owner file system object */
	WORD	id;			/**< @brief Owner file system mount ID */
	WORD	index;			/**< @brief Current read/write index number */
	DWORD	sclust;			/**< @brief Table start cluster (0:Root dir) */
	DWORD	clust;			/**< @brief Current cluster */
	DWORD	sect;			/**< @brief Current sector */
	BYTE*	dir;			/**< @brief Pointer to the current SFN entry in the win[] */
	BYTE*	fn;			/**< @brief Pointer to the SFN (in/out) {file[8],ext[3],status[1]} */
#if _USE_LFN
	WCHAR*	lfn;			/**< @brief Pointer to the LFN working buffer */
	WORD	lfn_idx;		/**< @brief Last matched LFN index number (0xFFFF:No LFN) */
#endif
} DIR;



/** @brief File status structure (FILINFO) */
typedef struct {
	DWORD	fsize;			/**< @brief File size */
	WORD	fdate;			/**< @brief Last modified date */
	WORD	ftime;			/**< @brief Last modified time */
	BYTE	fattrib;		/**< @brief Attribute */
	TCHAR	fname[13];		/**< @brief Short file name (8.3 format) */
#if _USE_LFN
	TCHAR*	lfname;			/**< @brief Pointer to the LFN buffer */
	UINT 	lfsize;			/**< @brief Size of LFN buffer in TCHAR */
#endif
} FILINFO;



/** @brief File function return code (FRESULT) */
typedef enum {
	FR_OK = 0,			/**< @brief (0) Succeeded */
	FR_DISK_ERR,			/**< @brief (1) A hard error occurred in the low level disk I/O layer */
	FR_INT_ERR,			/**< @brief (2) Assertion failed */
	FR_NOT_READY,			/**< @brief (3) The physical drive cannot work */
	FR_NO_FILE,			/**< @brief (4) Could not find the file */
	FR_NO_PATH,			/**< @brief (5) Could not find the path */
	FR_INVALID_NAME,		/**< @brief (6) The path name format is invalid */
	FR_DENIED,			/**< @brief (7) Access denied due to prohibited access or directory full */
	FR_EXIST,			/**< @brief (8) Access denied due to prohibited access */
	FR_INVALID_OBJECT,		/**< @brief (9) The file/directory object is invalid */
	FR_WRITE_PROTECTED,		/**< @brief (10) The physical drive is write protected */
	FR_INVALID_DRIVE,		/**< @brief (11) The logical drive number is invalid */
	FR_NOT_ENABLED,			/**< @brief (12) The volume has no work area */
	FR_NO_FILESYSTEM,		/**< @brief (13) There is no valid FAT volume */
	FR_MKFS_ABORTED,		/**< @brief (14) The f_mkfs() aborted due to any parameter error */
	FR_TIMEOUT,			/**< @brief (15) Could not get a grant to access the volume within defined period */
	FR_LOCKED,			/**< @brief (16) The operation is rejected according to the file sharing policy */
	FR_NOT_ENOUGH_CORE,		/**< @brief (17) LFN working buffer could not be allocated */
	FR_TOO_MANY_OPEN_FILES,		/**< @brief (18) Number of open files > _FS_SHARE */
	FR_INVALID_PARAMETER		/**< @brief (19) Given parameter is invalid */
} FRESULT;

/*--------------------------------------------------------------*/
/* FatFs module application interface                           */

/** @brief Mount/Unmount a logical drive */
FRESULT f_mount (BYTE, FATFS*);
/** @brief Open or create a file */
FRESULT f_open (FIL*, const TCHAR*, BYTE);
/** @brief Read data from a file */
FRESULT f_read (FIL*, void*, UINT, UINT*, pointer_type);
#define f_read_be(FIL,BUF,COUNT,BYTES) f_read(FIL,BUF,COUNT,BYTES,be_pointer)
#define f_read_le(FIL,BUF,COUNT,BYTES) f_read(FIL,BUF,COUNT,BYTES,le_pointer)
/** @brief Move file pointer of a file object */
FRESULT f_lseek (FIL*, DWORD);
/** @brief Close an open file object */
FRESULT f_close (FIL*);
/** @brief Open an existing directory */
FRESULT f_opendir (DIR*, const TCHAR*);
/** @brief Read a directory item */
FRESULT f_readdir (DIR*, FILINFO*);
/** @brief Get file status */
FRESULT f_stat (const TCHAR*, FILINFO*);
/** @brief Write data to a file */
FRESULT f_write (FIL*, const void*, UINT, UINT*, pointer_type);
#define f_write_be(FIL,BUF,COUNT,BYTES) f_write(FIL,BUF,COUNT,BYTES,be_pointer)
#define f_write_le(FIL,BUF,COUNT,BYTES) f_write(FIL,BUF,COUNT,BYTES,le_pointer)
/** @brief Get number of free clusters on the drive */
FRESULT f_getfree (const TCHAR*, DWORD*, FATFS**);
/** @brief Truncate file */
FRESULT f_truncate (FIL*);
/** @brief Flush cached data of a writing file */
FRESULT f_sync (FIL*);
/** @brief Delete an existing file or directory */
FRESULT f_unlink (const TCHAR*);
/** @brief Create a new directory */
FRESULT	f_mkdir (const TCHAR*);	
/** @brief Change attribute of the file/dir */
FRESULT f_chmod (const TCHAR*, BYTE, BYTE);
/** @brief Change times-tamp of the file/dir */
FRESULT f_utime (const TCHAR*, const FILINFO*);
/** @brief Rename/Move a file or directory */
FRESULT f_rename (const TCHAR*, const TCHAR*);
/** @brief Change current drive */
FRESULT f_chdrive (BYTE);
/** @brief Change current directory */
FRESULT f_chdir (const TCHAR*);
/** @brief Get current directory */
FRESULT f_getcwd (TCHAR*, UINT);
/** @brief Forward data to the stream */
FRESULT f_forward (FIL*, UINT(*)(const BYTE*,UINT), UINT, UINT*);
/** @brief Create a file system on the drive */
FRESULT f_mkfs (BYTE, BYTE, UINT);
/** @brief Divide a physical drive into some partitions */
FRESULT	f_fdisk (BYTE, const DWORD[], void*);
/** @brief Put a character to the file */
int f_putc (TCHAR, FIL*);
/** @brief Put a string to the file */
int f_puts (const TCHAR*, FIL*);
/** @brief Put a formatted string to the file */
int f_printf (FIL*, const TCHAR*, ...);
/** @brief Get a string from the file */
TCHAR* f_gets (TCHAR*, int, FIL*);

#define f_eof(fp) (((fp)->fptr == (fp)->fsize) ? 1 : 0)
#define f_error(fp) (((fp)->flag & FA__ERROR) ? 1 : 0)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->fsize)

#ifndef EOF
#define EOF (-1)
#endif




/*--------------------------------------------------------------*/
/* Additional user defined functions                            */

#if !_FS_READONLY
/** RTC function */
DWORD get_fattime (void);
#endif

/* Unicode support functions */
#if _USE_LFN						/* Unicode - OEM code conversion */
/** @brief OEM-Unicode bidirectional conversion */
WCHAR ff_convert (WCHAR, UINT);	
/** @brief Unicode upper-case conversion */
WCHAR ff_wtoupper (WCHAR);
#if _USE_LFN == 3					/* Memory functions */
/** @brief Allocate memory block */
void* ff_memalloc (UINT);
/** @brief Free memory block */
void ff_memfree (void*);
#endif
#endif

/* Sync functions */
#if _FS_REENTRANT
/** @brief Create a sync object */
int ff_cre_syncobj (BYTE, _SYNC_t*);
/** @brief Lock sync object */
int ff_req_grant (_SYNC_t);
/** @brief Unlock sync object */
void ff_rel_grant (_SYNC_t);
/** @brief Delete a sync object */
int ff_del_syncobj (_SYNC_t);
#endif




/*--------------------------------------------------------------*/
/* Flags and offset address                                     */


/* File access control and file status flags (FIL.flag) */

#define	FA_READ				0x01
#define	FA_OPEN_EXISTING	0x00
#define FA__ERROR			0x80

#if !_FS_READONLY
#define	FA_WRITE		0x02
#define	FA_CREATE_NEW		0x04
#define	FA_CREATE_ALWAYS	0x08
#define	FA_OPEN_ALWAYS		0x10
#define FA__WRITTEN		0x20
#define FA__DIRTY		0x40
#endif


/* FAT sub type (FATFS.fs_type) */

#define FS_FAT12	1
#define FS_FAT16	2
#define FS_FAT32	3


/* File attribute bits for directory entry */
/** @brief Read only file attribute bit for directory entry */
#define	AM_RDO	0x01
/** @brief Hidden file attribute bit for directory entry */
#define	AM_HID	0x02
/** @brief System file attribute bit for directory entry */
#define	AM_SYS	0x04
/** @brief Volume label file attribute bit for directory entry */
#define	AM_VOL	0x08
/** @brief LFN entry file attribute bit for directory entry */
#define AM_LFN	0x0F
/** @brief Directory file attribute bit for directory entry */
#define AM_DIR	0x10
/** @brief Archive file attribute bit for directory entry */
#define AM_ARC	0x20
/** @brief Mask of defined bits file attribute bit for directory entry */
#define AM_MASK	0x3F


/** @brief Fast seek feature */
#define CREATE_LINKMAP	0xFFFFFFFF



/*--------------------------------*/
/* Multi-byte word access macros  */

#if _WORD_ACCESS == 1	/* Enable word access to the FAT structure */
#define	LD_WORD(ptr)		(WORD)(*(WORD*)(BYTE*)(ptr))
#define	LD_DWORD(ptr)		(DWORD)(*(DWORD*)(BYTE*)(ptr))
#define	ST_WORD(ptr,val)	*(WORD*)(BYTE*)(ptr)=(WORD)(val)
#define	ST_DWORD(ptr,val)	*(DWORD*)(BYTE*)(ptr)=(DWORD)(val)
#else					/* Use byte-by-byte access to the FAT structure */
#define	LD_WORD(ptr)		(WORD)(((WORD)*((BYTE*)(ptr)+1)<<8)|(WORD)*(BYTE*)(ptr))
#define	LD_DWORD(ptr)		(DWORD)(((DWORD)*((BYTE*)(ptr)+3)<<24)|((DWORD)*((BYTE*)(ptr)+2)<<16)|((WORD)*((BYTE*)(ptr)+1)<<8)|*(BYTE*)(ptr))
#define	ST_WORD(ptr,val)	*(BYTE*)(ptr)=(BYTE)(val); *((BYTE*)(ptr)+1)=(BYTE)((WORD)(val)>>8)
#define	ST_DWORD(ptr,val)	*(BYTE*)(ptr)=(BYTE)(val); *((BYTE*)(ptr)+1)=(BYTE)((WORD)(val)>>8); *((BYTE*)(ptr)+2)=(BYTE)((DWORD)(val)>>16); *((BYTE*)(ptr)+3)=(BYTE)((DWORD)(val)>>24)
#endif

#ifdef __cplusplus
}
#endif
/// @}
//#endif /* _FATFS */
