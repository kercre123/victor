#ifdef __cplusplus
extern "C" {
#endif
#if 0
} // make emacs happy
#endif

/**
 * se_diag.h
 * 
 * A Module to keep track of diagnostics. 
 *
 * Usage:
 * 
 * int mdit, mdiat1, mdiat2;
 * int control_id;
 * #define LEN1 30
 * #define LEN2 80
 * static int32       some_variable;
 * static int16[LEN1] int_arrray;
 * MyInit() {
 *   // Initialzie the individual diags.
 *   mdit   = SEDiagNewInt16          ("my-diag-int-thing", SE_DIAG_R, 0); // no need for size
 *   mdiat1 = SEDiagNewInt16Array     ("my-diag-int16-thing", SE_DIAG_COPY     , LEN1, SE_DIAG_R, 0); // this will allocate memory & copy the array
 *   mdiat2 = SEDiagNewInt16Array     ("my-diag-int16-thing", SE_DIAG_REFERENCE, LEN2, SE_DIAG_R, 0); // this will just point to an array that already exists.

 *   // READ/Write -- controls
 *   control_id = SEDiagNewInt32 ("my-control", SE_DIAG_RW, &some_variable, 1, NULL);        // Create a simple read/write variable, with no callback.
 *   control_id = SEDiagNewInt32 ("my-control", SE_DIAG_RW, NULL,           0, my_rw_callback); // Create a callback based read/write variable.  Length is returned by the callback.
 * 
 *   // READ only
 *   control_id = SEDiagNewInt32 ("my-control", SE_DIAG_R,  &some_variable, 1, NULL);         // Create a simple read only variable, with no callback.
 *   control_id = SEDiagNewInt32 ("my-control", SE_DIAG_R,  &some_variable, 1, my_r_callback);// Create a read-only variable with callback.
 *   control_id = SEDiagNewInt32 ("my-control", SE_DIAG_R,  &some_variable, 1, my_r_callback);// invalid  -- you don't specify both a callback and storage.
 * 
 * }
 * 
 * static int16 staticIntArray[LEN2];
 * MyRun() {
 *    ...  // calculate something
 *    int16 theDiagValue = something();
 *    int16 dynamicIntArray[LEN1] = { some diag array }
 *    SEDiagReportInt16     (mdit  , theDiagValue   );
 *    SEDiagReportInt16Array(mdiat1, dynamicIntArray);
 *    SEDiagReportInt16Array(mdiat2, staticIntArray );
 * }
 * 
 *  
 * void MyDump(int block) {
 *     int i;
 *     for (i = 0; i < SEDiagGetNumDiags(); i++) {
 *         SEDiag_t *diag = SEDiagGet(i);
 *         // now, do something with the diag.  The diag is a union 
 *         // that will contain all informatino needed to decode what the
 *         // data type is.
 *     }
 * }
 * #include <stdlib.h>
 * main() {
 *    SEDiagInit(malloc, free);
 *    
 *    MyInit();
 *    ... 
 *    block = 0;
 *    while (1) {
 *        MyRun();
 *      #if HAVE_PRINTF
 *        FILE *f = fopen(GetFileName(block), "w");
 *        SEDiagDump(stdout);
 *        SEDiagDump(f);
 *        fclose(f);
 *      #else
 *        MyDump(block);
 *      #endif
 *      }
 *  }
 * // SEDiagDump will print this
 * // my-diag-int-thing,32
 * // my-diag-int16-thing-copy,0,1,2,3,4,5,6,7,8,9
 * // my-diag-int16-thing-byref,0,10,20,30,40
 */
#ifndef SE_DIAG_H
#define SE_DIAG_H
#include <stdlib.h>
#include "se_types.h"
#include <stdio.h>

#include "se_diag_types.h"

//
// set default configuration
void SEDiagSetDefaultConfig(SEDiagConfig_t *pConfig);

/**
 * Initialize the SEDiag module 
 */
void SEDiagInit(SEDiagConfig_t *pConfig);


/** 
 * Un-init SEDiag.  
 */
void SEDiagClose(void);


/**
 * 
 * hack for platform/pa per-block callback demo:
 * get and set singleton pointers
 **/
void *SEDiagGetSingletonPntr(void);
void SEDiagSetSingletonPntr(void *p);

/** Create new SEDiag entries */
int SEDiagNewInt16__        (const char *name, int16   *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt16__       (const char *name, uint16  *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt32__        (const char *name, int32   *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt32__       (const char *name, uint32  *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt64__        (const char *name, int64   *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt64__       (const char *name, uint64  *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewFloat32__      (const char *name, float32 *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewCharArray__    (const char *name, char    *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt16Array__   (const char *name, int16   *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt16Array__  (const char *name, uint16  *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt32Array__   (const char *name, int32   *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt32Array__  (const char *name, uint32  *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt64Array__   (const char *name, int64   *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt64Array__  (const char *name, uint64  *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewFloat32Array__ (const char *name, float32 *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewEnum__         (const char *name, enum_t  *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help, SEDiagEnumDescriptor_t *descriptors);
int SEDiagNewEnumArray__    (const char *name, enum_t  *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help, SEDiagEnumDescriptor_t *descriptors);

int SEDiagNewInt16WithPrefix__        (const char *prefix, const char *name, int16   *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt16WithPrefix__       (const char *prefix, const char *name, uint16  *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt32WithPrefix__        (const char *prefix, const char *name, int32   *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt32WithPrefix__       (const char *prefix, const char *name, uint32  *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt64WithPrefix__        (const char *prefix, const char *name, int64   *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt64WithPrefix__       (const char *prefix, const char *name, uint64  *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewFloat32WithPrefix__      (const char *prefix, const char *name, float32 *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt16ArrayWithPrefix__   (const char *prefix, const char *name, int16   *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt16ArrayWithPrefix__  (const char *prefix, const char *name, uint16  *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt32ArrayWithPrefix__   (const char *prefix, const char *name, int32   *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt32ArrayWithPrefix__  (const char *prefix, const char *name, uint32  *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewInt64ArrayWithPrefix__   (const char *prefix, const char *name, int64   *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewUInt64ArrayWithPrefix__  (const char *prefix, const char *name, uint64  *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewFloat32ArrayWithPrefix__ (const char *prefix, const char *name, float32 *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help);
int SEDiagNewEnumWithPrefix__         (const char *prefix, const char *name, enum_t  *ptr, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help, SEDiagEnumDescriptor_t *descriptors);
int SEDiagNewEnumArrayWithPrefix__    (const char *prefix, const char *name, enum_t  *ptr, int length, SEDiagRW_t rw, SEDiagCallback_t wrtCallback, const char *help, SEDiagEnumDescriptor_t *descriptors);

#if !defined(SE_DIAG_NO_HELP_STRINGS)
//
// create new sediag with help string
#define SEDiagNewInt16(name, ptr, rw, callback, help)      SEDiagNewInt16__(name, ptr, rw, callback, help)
#define SEDiagNewUInt16(name, ptr, rw, callback, help)     SEDiagNewUInt16__(name, ptr, rw, callback, help)
#define SEDiagNewInt32(name, ptr, rw, callback, help)      SEDiagNewInt32__(name, ptr, rw, callback, help)
#define SEDiagNewUInt32(name, ptr, rw, callback, help)     SEDiagNewUInt32__(name, ptr, rw, callback, help)
#define SEDiagNewInt64(name, ptr, rw, callback, help)      SEDiagNewInt64__(name, ptr, rw, callback, help)
#define SEDiagNewUInt64(name, ptr, rw, callback, help)     SEDiagNewUInt64__(name, ptr, rw, callback, help)
#define SEDiagNewFloat32(name, ptr, rw, callback, help)    SEDiagNewFloat32__(name, ptr, rw, callback, help)
#define SEDiagNewCharArray(name, ptr, rw, callback, help)      SEDiagNewCharArray__(name, ptr, rw, callback, help)
#define SEDiagNewInt16Array(name, ptr, length, rw, callback, help)      SEDiagNewInt16Array__(name, ptr, length, rw, callback, help)
#define SEDiagNewUInt16Array(name, ptr, length, rw, callback, help)     SEDiagNewUInt16Array__(name, ptr, length, rw, callback, help)
#define SEDiagNewInt32Array(name, ptr, length, rw, callback, help)      SEDiagNewInt32Array__(name, ptr, length, rw, callback, help)
#define SEDiagNewUInt32Array(name, ptr, length, rw, callback, help)     SEDiagNewUInt32Array__(name, ptr, length, rw, callback, help)
#define SEDiagNewInt64Array(name, ptr, length, rw, callback, help)      SEDiagNewInt64Array__(name, ptr, length, rw, callback, help)
#define SEDiagNewUInt64Array(name, ptr, length, rw, callback, help)     SEDiagNewUInt64Array__(name, ptr, length, rw, callback, help)
#define SEDiagNewFloat32Array(name, ptr, length, rw, callback, help)    SEDiagNewFloat32Array__(name, ptr, length, rw, callback, help)
#define SEDiagNewEnum(name, ptr, rw, callback, help, descriptors)       SEDiagNewEnum__(name, ptr, rw, callback, help, descriptors)
#define SEDiagNewEnumArray(name, ptr, length, rw, callback, help, descriptors)       SEDiagNewEnumArray__(name, ptr, length, rw, callback, help, descriptors)

#define SEDiagNewInt16WithPrefix(prefix, name, ptr, rw, callback, help)      SEDiagNewInt16WithPrefix__(prefix, name, ptr, rw, callback, help)
#define SEDiagNewUInt16WithPrefix(prefix, name, ptr, rw, callback, help)     SEDiagNewUInt16WithPrefix__(prefix, name, ptr, rw, callback, help)
#define SEDiagNewInt32WithPrefix(prefix, name, ptr, rw, callback, help)      SEDiagNewInt32WithPrefix__(prefix, name, ptr, rw, callback, help)
#define SEDiagNewUInt32WithPrefix(prefix, name, ptr, rw, callback, help)     SEDiagNewUInt32WithPrefix__(prefix, name, ptr, rw, callback, help)
#define SEDiagNewInt64WithPrefix(prefix, name, ptr, rw, callback, help)      SEDiagNewInt64WithPrefix__(prefix, name, ptr, rw, callback, help)
#define SEDiagNewUInt64WithPrefix(prefix, name, ptr, rw, callback, help)     SEDiagNewUInt64WithPrefix__(prefix, name, ptr, rw, callback, help)
#define SEDiagNewFloat32WithPrefix(prefix, name, ptr, rw, callback, help)    SEDiagNewFloat32WithPrefix__(prefix, name, ptr, rw, callback, help)
#define SEDiagNewInt16ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)      SEDiagNewInt16ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, help)
#define SEDiagNewUInt16ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)     SEDiagNewUInt16ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, help)
#define SEDiagNewInt32ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)      SEDiagNewInt32ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, help)
#define SEDiagNewUInt32ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)     SEDiagNewUInt32ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, help)
#define SEDiagNewInt64ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)      SEDiagNewInt64ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, help)
#define SEDiagNewUInt64ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)     SEDiagNewUInt64Array__(prefix, name, ptr, length, rw, callback, help)
#define SEDiagNewFloat32ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)    SEDiagNewFloat32ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, help)
#define SEDiagNewEnumWithPrefix(prefix, name, ptr, rw, callback, help, descriptors)       SEDiagNewEnumWithPrefix__(prefix, name, ptr, rw, callback, help, descriptors)
#define SEDiagNewEnumArrayWithPrefix(prefix, name, ptr, length, rw, callback, help, descriptors)       SEDiagNewEnumArrayWithPrefix__(prefix, name, ptr, length, rw, callback, help, descriptors)

#else
//
// replace help string with "" to conserve memory
#define SEDiagNewInt16(name, ptr, rw, callback, help)      SEDiagNewInt16__(name, ptr, rw, callback, "")
#define SEDiagNewUInt16(name, ptr, rw, callback, help)     SEDiagNewUInt16__(name, ptr, rw, callback, "")
#define SEDiagNewInt32(name, ptr, rw, callback, help)      SEDiagNewInt32__(name, ptr, rw, callback, "")
#define SEDiagNewUInt32(name, ptr, rw, callback, help)     SEDiagNewUInt32__(name, ptr, rw, callback, "")
#define SEDiagNewInt64(name, ptr, rw, callback, help)      SEDiagNewInt64__(name, ptr, rw, callback, "")
#define SEDiagNewUInt64(name, ptr, rw, callback, help)     SEDiagNewUInt64__(name, ptr, rw, callback, "")
#define SEDiagNewFloat32(name, ptr, rw, callback, help)    SEDiagNewFloat32__(name, ptr, rw, callback, "")
#define SEDiagNewInt16Array(name, ptr, length, rw, callback, help)      SEDiagNewInt16Array__(name, ptr, length, rw, callback, "")
#define SEDiagNewUInt16Array(name, ptr, length, rw, callback, help)     SEDiagNewUInt16Array__(name, ptr, length, rw, callback, "")
#define SEDiagNewInt32Array(name, ptr, length, rw, callback, help)      SEDiagNewInt32Array__(name, ptr, length, rw, callback, "")
#define SEDiagNewUInt32Array(name, ptr, length, rw, callback, help)     SEDiagNewUInt32Array__(name, ptr, length, rw, callback, "")
#define SEDiagNewInt64Array(name, ptr, length, rw, callback, help)      SEDiagNewInt64Array__(name, ptr, length, rw, callback, "")
#define SEDiagNewUInt64Array(name, ptr, length, rw, callback, help)     SEDiagNewUInt64Array__(name, ptr, length, rw, callback, "")
#define SEDiagNewFloat32Array(name, ptr, length, rw, callback, help)    SEDiagNewFloat32Array__(name, ptr, length, rw, callback, "")
#define SEDiagNewEnum(name, ptr, rw, callback, help, descriptors)       SEDiagNewEnum__(name, ptr, rw, callback, "", descriptors)
#define SEDiagNewEnumArray(name, ptr, length, rw, callback, help, descriptors)       SEDiagNewEnumArray__(name, ptr, length, rw, callback, "", descriptors);

#define SEDiagNewInt16WithPrefix(prefix, name, ptr, rw, callback, help)      SEDiagNewInt16WithPrefix__(prefix, name, ptr, rw, callback, "")
#define SEDiagNewUInt16WithPrefix(prefix, name, ptr, rw, callback, help)     SEDiagNewUInt16WithPrefix__(prefix, name, ptr, rw, callback, "")
#define SEDiagNewInt32WithPrefix(prefix, name, ptr, rw, callback, help)      SEDiagNewInt32WithPrefix__(prefix, name, ptr, rw, callback, "")
#define SEDiagNewUInt32WithPrefix(prefix, name, ptr, rw, callback, help)     SEDiagNewUInt32WithPrefix__(prefix, name, ptr, rw, callback, "")
#define SEDiagNewInt64WithPrefix(prefix, name, ptr, rw, callback, help)      SEDiagNewInt64WithPrefix__(prefix, name, ptr, rw, callback, "")
#define SEDiagNewUInt64WithPrefix(prefix, name, ptr, rw, callback, help)     SEDiagNewUInt64WithPrefix__(prefix, name, ptr, rw, callback, "")
#define SEDiagNewFloat32WithPrefix(prefix, name, ptr, rw, callback, help)    SEDiagNewFloat32WithPrefix__(prefix, name, ptr, rw, callback, "")
#define SEDiagNewInt16ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)      SEDiagNewInt16ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, "")
#define SEDiagNewUInt16ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)     SEDiagNewUInt16ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, "")
#define SEDiagNewInt32ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)      SEDiagNewInt32ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, "")
#define SEDiagNewUInt32ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)     SEDiagNewUInt32ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, "")
#define SEDiagNewInt64ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)      SEDiagNewInt64ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, "")
#define SEDiagNewUInt64ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)     SEDiagNewUInt64Array__(prefix, name, ptr, length, rw, callback, "")
#define SEDiagNewFloat32ArrayWithPrefix(prefix, name, ptr, length, rw, callback, help)    SEDiagNewFloat32ArrayWithPrefix__(prefix, name, ptr, length, rw, callback, "")
#define SEDiagNewEnumWithPrefix(prefix, name, ptr, rw, callback, help, descriptors)       SEDiagNewEnumWithPrefix__(prefix, name, ptr, rw, callback, "", descriptors)
#define SEDiagNewEnumArrayWithPrefix(prefix, name, ptr, length, rw, callback, help, descriptors)       SEDiagNewEnumArrayWithPrefix__(prefix, name, ptr, length, rw, callback, "", descriptors)
#endif // se_diag_no_help_strings

/** Get a value (or array of values) from a Diag.  This is a reference.*/
char   *SEDiagGetCharArray   (int   index);
int16  *SEDiagGetInt16Array  (int   index);
int16  *SEDiagGetInt16ArrayI (int   index, int i); // if there are multiple of the same name initialized, retrieve the nth one.
uint16 *SEDiagGetUInt16Array (int   index);
int32  *SEDiagGetInt32Array  (int   index);
uint32 *SEDiagGetUInt32Array (int   index);
int64  *SEDiagGetInt64Array  (int   index);
uint64 *SEDiagGetUInt64Array (int   index);
float  *SEDiagGetFloat32Array  (int   index);

/** Get single values from a Diag.  Casting is done if necessary. This does a copy */
int16  SEDiagGetInt16        (int   index);
uint16 SEDiagGetUInt16       (int   index);
int32  SEDiagGetInt32        (int   index);
uint32 SEDiagGetUInt32       (int   index);
int64  SEDiagGetInt64        (int   index);
uint64 SEDiagGetUInt64       (int   index);
float32      SEDiagGetFloat32    (int   index);

const char  *SEDiagGetEnumValueAsString    (int index, int enum_index); // get the string value of the enum_index.  return NULL if enum_index is out of bounds.
int          SEDiagGetEnumAsInt            (int index);
const char  *SEDiagGetEnumAsString         (int index);
int          SEDiagGetEnumAsIntN           (int index, int n);
const char  *SEDiagGetEnumAsStringN        (int index, int n);
void         SEDiagSetEnumAsInt            (int index, int value);
void         SEDiagSetEnumAsIntN           (int index, int value, int n);
void         SEDiagSetEnumAsString         (int index, const char *value);
void         SEDiagSetEnumAsStringN        (int index, const char *value, int n);
const char  *SEDiagTranslateEnumIntToString(int index, int value);
int          SEDiagTranslateEnumStringToInt(int index, const char *value);
 
void    SEDiagNextEnum        (int   index);


void   SEDiagSetInt16        (int   index, int16  value);
void   SEDiagSetUInt16       (int   index, uint16 value);
void   SEDiagSetInt32        (int   index, int32  value);
void   SEDiagSetUInt32       (int   index, uint32 value);
void   SEDiagSetInt64        (int   index, int64  value);
void   SEDiagSetUInt64       (int   index, uint64 value);
void   SEDiagSetFloat32      (int   index, float32  value);

void   SEDiagSetInt16N        (int   index, int16   value, int n);
void   SEDiagSetUInt16N       (int   index, uint16  value, int n);
void   SEDiagSetInt32N        (int   index, int32   value, int n);
void   SEDiagSetUInt32N       (int   index, uint32  value, int n);
void   SEDiagSetInt64N        (int   index, int64   value, int n);
void   SEDiagSetUInt64N       (int   index, uint64  value, int n);
void   SEDiagSetFloat32N      (int   index, float32 value, int n);


    void SEDiagGetValueAsString(int index, char *buffer, int buffer_len);

/** 'Set' the array.  This is used when you have already done an
 *  SEDiagGEt<type>Array.  You can set the values in the array that
 *  you got.  This command 'activates' the values by calling any
 *  callback that was registered at init time.  This may or may not do
 *  anything useful.  
 */
//int   SEDiagSetArray        (int index);

int          SEDiagGetNumDiags(void);
int          SEDiagGetIndex   (const char *name);
int          SEDiagGetIndexAssert   (const char *name);
int          SEDiagExists(const char *name);
SEDiag_t    *SEDiagGet        (int index);
int          SEDiagGetLen     (int index);
SEDiagLevel_t SEDiagGetLevel(int index);
SEDiagType_t SEDiagGetType    (int index);
SEDiagRW_t   SEDiagGetRW      (int index);
const char  *SEDiagGetName    (int index);
const char  *SEDiagGetHelp    (int index);
void         SEDiagDump       (FILE *f);

/**
 * Exclude a diagnostic during dump
 * @param index index of the parameter to ignore.
 */
void      SEDiagDumpExclude(int index);

/**
 * Exclude a diagnostic during dump
 * @param name name of diagnostic to ignore.
 */
void      SEDiagDumpExcludeName(const char *name);

void      SEDiagPrintDiag(FILE *f, SEDiag_t *d);

int SEDiagIsInitialized(void);

#endif

#if 0
{ // make emacs happy
#endif
#ifdef __cplusplus
}
#endif

