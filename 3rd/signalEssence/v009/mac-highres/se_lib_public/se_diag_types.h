#ifndef _SE_DIAG_TYPES_H_
#define _SE_DIAG_TYPES_H_
#define MAX_SE_DIAG_NAME_LENGTH 90

typedef enum {
    SE_DIAG_FIRST_DONT_USE,
    SE_DIAG_CHAR,
    SE_DIAG_INT16,
    SE_DIAG_INT32,
    SE_DIAG_UINT16,
    SE_DIAG_UINT32,
    SE_DIAG_INT64,
    SE_DIAG_UINT64,
    SE_DIAG_FLOAT32,
    SE_DIAG_ENUM,
    SE_DIAG_LAST
} SEDiagType_t;

typedef enum {
    SE_DIAG_LEVEL_FIRST_DONT_USE,
    SE_DIAG_BASIC,        /** A Parameter/diag that pretty much everybody will want to look at and use. */
    SE_DIAG_INTERMEDIATE, /** Parameter/diag that is a little less common to touch, may be touched by customers. */
    SE_DIAG_ADVANCED,     /** The diag is unlikely to be touched */
    SE_DIAG_DEBUG,        /** This diag is really only for debugging, though we may want to set or examine it. */
    SE_DIAG_LEVEL_LAST
} SEDiagLevel_t;

// Whether this is a read/or write.  Maybe both
typedef enum {
    SE_DIAG_RW_FIRST_DONT_USE,
    SE_DIAG_R  = 0x1, // <- these are usable as bit-fields
    SE_DIAG_W  = 0x2, // <- SO, SE_DIAG_R | SE_DIAG_W is the same as
    SE_DIAG_RW = 0x3, // <- SE_DIAG_RW
    SE_DIAG_RW_LAST
} SEDiagRW_t;  

typedef struct {
    int   value;
    const char *value_str;
    const char *description;
} SEDiagEnumDescriptor_t;

#define DIAG_ENUM_END                    {0, 0, 0}
#define DIAG_ENUM_DECLARE(ENUM, DESCR)   {ENUM, #ENUM, DESCR}

struct SEDiag_t;
/* The type of the callback function  */
typedef void (*SEDiagCallback_t)(struct SEDiag_t *diag, void *value, int elem_index);

typedef struct
{
    int skipExistenceCheck;
    int skipValidNameCheck;
    int disableCopyHelpString;
    int maxNumDiags;
} SEDiagConfig_t;


/** 
 * This is the diagnostic type for a single diag.
 * it encompases scalar values and arrays of all basic se_types.h types.
 */
typedef struct SEDiag_t {
    char           name[MAX_SE_DIAG_NAME_LENGTH];       /** the name of the diagnostic. */
    SEDiagType_t   type;       /** the type of the diagnostic. */
    int            length;     /** if this is an array type, it's the length
                                *  of the array in array elements, not
                                *  chars. */
    SEDiagRW_t                rw; /** Is this a read or write diag? */
    SEDiagLevel_t          level; /** What level is this diag? */
    unsigned int excludeFromDump; /** Should this diag be excluded from a dump? */
    /** The callback is used during write operations.  If this callback field is non-null 
     *  the function is called with a pointer to this diag struct, as well as the user argument. */
    SEDiagCallback_t wrtCallback;
    char      help[MAX_SE_DIAG_NAME_LENGTH];
    SEDiagEnumDescriptor_t *enum_descriptors;
    union {
        char    *achar;
        int16   *ai16;
        uint16  *au16;
        int32   *ai32;
        uint32  *au32;
        int64   *ai64;
        uint64  *au64;
        float32 *af32;
        void    *vp;
        enum_t  *aenum;
    } u;
} SEDiag_t;

#endif
