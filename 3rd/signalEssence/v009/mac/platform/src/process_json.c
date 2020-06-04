#include "process_json.h"
#include "cjson.h"
#include "se_error.h"
#include "string.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "se_snprintf.h"
#if defined(USE_META_MMIF) // use alternate mmif & sediag prototypes
#include "meta_mmif.h"
#else
#include "se_diag.h"
#endif
#ifdef _MSC_VER
#include <direct.h>
#endif

//
// msvc doesnt support posix mkdir
#ifdef _MSC_VER
#define MKDIR(x) _mkdir(x)
#else
// assume posix
#define MKDIR(x) mkdir(x,0777)
#endif

/*

INTEGRATION
===========
In your program, initialize the json processor like this AFTER calling MMIfInit():

    process_json_t pj;
    ...
    err = process_json_init(&pj, "settings.json");
    if (err) {
        printf("Error processing your json file.\n");
        exit(-1);
    }

where "settings.json" is a JSON file with the desired sediag settings (see below).

Then while processing sample blocks, call:
    process_json_set(&pj, N);    // set sediag variables for block_index N
    process_json_dump(&pj, N, is_last_call); // write sediag variables for block_index N

OUTPUT FILE
============
Outputs are written to an output directory, specified in the JSON file,
one file per sediag variable.

EXAMPLE JSON FILE
==================
The "set" block lists sample block indices during which to set variables, and
the name/value pairs of sediag variables to set.

The "dump_filename" block specifies the output directory.

The "dump" block lists sample block indices during which to read/dump variables,
and the names of sediag variables to read/dump.

{
    "set" : 
    [
        {
            "at_block" : 0,
            "diags"    : 
            {
                "preproc_gsin_per_mic_q12" : [1024, 4096, 4096, 4096, 4096, 2048, 8192]
            }
        },
        {
            "at_block" : [5, 6],
            "diags"    : 
            {
                "preproc_dc_rmv_num_poles" : 2
            }  
        }
    ],
    "dump_filename" : "diag_dump_directory",
    "dump"          :
    [
        {
            "comment"  : "Dump all diags at all blocks",
            "at_block" : "*",
            "diags"    : ["*"]
        },
        {
            "comment"  : "dump mmfx_blockszie at blocks 0.  No need to do it every block... ",
            "at_block" : 0,
            "diags"    : ["mmfx_blocksize"]
        },
        {
            "comment"  : "dump AFTER blocks 1, 2, 3.  Dumps happen after the block numbered.",
            "at_block" : [1, 2, 3],
            "diags"    : 
            [
                "fdsearch_best_beam_index", 
                "fdsearch_best_beam_confidence" 
            ]
        },
        {
            "comment"  : "dump after block 32",
            "at_block" : 32,
            "diags"    : [ "multiaec_filter_coefs_00" ]
        },
            {
                "comment"  : "dumps only after the last block",
                "at_block" : "end",
                "diags"    : 
            [
                "aecmon_num_resets" 
            ]
        }
    ]
}

 */

// ******************* JSON DUMPER OBJECT. ***********************
//  These methods below will dump in the following JSON format
// 
// [   //block
//     [0 , [ "diag_name" , [item, item, item, ...]]],
//     [1 , [ "diag_name" , [item, item, item, ...]]]
// ]
typedef struct json_dump_private_t {
    int has_previous_block;
    int has_previous_diag;
    int has_previous_value;
} json_dump_private_t;

static void json_dump_file_start(struct process_file_dumper *self, process_json_t *pj, const char *dump_filename) {
    json_dump_private_t *p;
    pj->dump_file = fopen(dump_filename, "w");
    self->private = malloc(sizeof(json_dump_private_t));
    p = (json_dump_private_t *) self->private;
    p->has_previous_block = 0;
    memset(self->private, 0, sizeof(json_dump_private_t));
    SeAssertString((NULL!=pj->dump_file), "Couldn't open the file pointed to by dump_filename for writing.");
    fprintf(pj->dump_file, "[\n");
    if (1==pj->flush_after_write) {
        fflush(pj->dump_file);
    }
}

static void json_dump_file_end (struct process_file_dumper *self, process_json_t *pj) {
    fprintf(pj->dump_file, "\n]\n");
    fclose(pj->dump_file);
    free(self->private);
}

static void json_begin_block(struct process_file_dumper *self, process_json_t *pj, int block, int is_last_block) {
    json_dump_private_t *p = (json_dump_private_t *) self->private;
    UNUSED(is_last_block);
    if (p->has_previous_block) 
        fprintf(pj->dump_file, ",\n");
    if (1==pj->flush_after_write) {
        fflush(pj->dump_file);
    }
    p->has_previous_block = 1;
    p->has_previous_diag = 0;
    fprintf(pj->dump_file, "  [ %7d , [\n", block);
    if (1==pj->flush_after_write) {
        fflush(pj->dump_file);
    }
}

static void json_end_block(struct process_file_dumper *self, process_json_t *pj, int block, int is_last_block) {
    UNUSED(self);
    UNUSED(block);
    UNUSED(is_last_block);
    //json_dump_private_t *p = (json_dump_private_t *) self->private;
    fprintf(pj->dump_file, "\n             ]\n  ]");
    if (1==pj->flush_after_write) {
        fflush(pj->dump_file);
    }
}

static void json_sediag_start(struct process_file_dumper *self, process_json_t *pj, const char *name) {
    json_dump_private_t *p = (json_dump_private_t *) self->private;
    if (p->has_previous_diag) 
        fprintf(pj->dump_file, ",\n");
    if (1==pj->flush_after_write) {
        fflush(pj->dump_file);
    }
    p->has_previous_diag = 1;
    p->has_previous_value = 0;
    fprintf(pj->dump_file, "                   [\"%s\" , [", name);
    if (1==pj->flush_after_write) {
        fflush(pj->dump_file);
    }
}

static void json_sediag_item(struct process_file_dumper *self, process_json_t *pj, const char *valuestring, int is_last) {
    json_dump_private_t *p = (json_dump_private_t *) self->private;
    UNUSED(is_last);
    if (p->has_previous_value)
        fprintf(pj->dump_file, ", ");
    p->has_previous_value = 1;
    fprintf(pj->dump_file, "%s", valuestring);
    if (1==pj->flush_after_write) {
        fflush(pj->dump_file);
    }
}

static void json_sediag_end(struct process_file_dumper *self, process_json_t *pj, int is_last) {
    //json_dump_private_t *p = (json_dump_private_t *) self->private;
    UNUSED(self);
    UNUSED(is_last);
    fprintf(pj->dump_file, "]]");
    if (1==pj->flush_after_write) {
        fflush(pj->dump_file);
    }
}

struct process_file_dumper json_dumper;

// init singleton process_file_dumper
static void json_dumper_init(struct process_file_dumper *dumper_ptr) {
    dumper_ptr->dump_file_start        = json_dump_file_start;
    dumper_ptr->dump_file_end          = json_dump_file_end;
    dumper_ptr->dump_file_begin_block  = json_begin_block;
    dumper_ptr->dump_file_end_block    = json_end_block;
    dumper_ptr->dump_file_sediag_start = json_sediag_start;
    dumper_ptr->dump_file_sediag_item  = json_sediag_item;
    dumper_ptr->dump_file_sediag_end   = json_sediag_end;
}

// struct process_file_dumper_t json_dumper = {
//     .dump_file_start        = json_dump_file_start,
//     .dump_file_end          = json_dump_file_end,
//     .dump_file_begin_block  = json_begin_block,
//     .dump_file_end_block    = json_end_block,
//     .dump_file_sediag_start = json_sediag_start,
//     .dump_file_sediag_item  = json_sediag_item,
//     .dump_file_sediag_end   = json_sediag_end,
// };  
// ******************* END JSON DUMPER OBJECT. ***********************

// ******************* CSV DUMPER OBJECT *****************************
// This object will dump in CSV format, in many files.
// each file will contain only 1 variable

#define MAX_DIAGS   1000 /* we currently have over 400.  1000 is not unreasonable... */
#define MAX_NAME_LEN 200

typedef char diag_name_t[MAX_NAME_LEN];

typedef struct csv_dump_private_t {
    const char *dirname;
    int has_previous_block;
    int has_previous_diag;
    int has_previous_value;
    int num_diags;
    diag_name_t diag_names [MAX_DIAGS];
    FILE        *diag_files[MAX_DIAGS];
    FILE        *current_file;
    int          current_block;
} csv_dump_private_t;

FILE *csv_dump_create_file(csv_dump_private_t *self, const char *diag_name)
{
    char filename[MAX_NAME_LEN+1000];
    FILE *f;
    se_snprintf(filename, MAX_NAME_LEN+1000, "%s/%s.csv", self->dirname, diag_name);
    f = fopen(filename, "w");
    if (f == NULL) {
        fprintf(stderr, "Couldn't open filename %s for writing.\n", filename);
        exit(-1);
    }
    se_snprintf(self->diag_names[self->num_diags], MAX_NAME_LEN, "%s", diag_name);
    self->diag_files[self->num_diags] = f;
    self->num_diags++;
    return f;
}
static FILE *csv_dump_lookup_file(csv_dump_private_t *self, const char *name_to_find)
{
    int i;
    int found = -1;
    for (i = 0; i < self->num_diags; i++) {
        if (strncmp(self->diag_names[i], name_to_find, MAX_NAME_LEN) == 0) {
            found = i;
            break;
        }
    }
    if (found >= 0) {
        return self->diag_files[found];
    } else {
        return NULL;
    }
}

static void csv_dump_file_start(struct process_file_dumper *self, process_json_t *pj, const char *dump_filename) {
    csv_dump_private_t *p;
    int err;
    // Treat dump_filename as a directory name.
    // make the directory if possible.  bail out if not possible.
    self->private = malloc(sizeof(csv_dump_private_t));
    memset(self->private, 0, sizeof(csv_dump_private_t));
    p = (csv_dump_private_t *) self->private;
    p->dirname = dump_filename;
    err = MKDIR(dump_filename);
    if ((err) && (errno != EEXIST)) {
        // Got an error making the directory, and not just because it doesn't already exist.
        fprintf(stderr, "Couldn't create directory \"%s\".  The reason is %s", dump_filename, strerror(err));
        exit(-1);
    }
    p->has_previous_block = 0;
    pj->dump_file = (FILE *)1; // set to nonzero to let the rest of the code know that we're ready to dump stuff.
}

static void csv_dump_file_end (struct process_file_dumper *self, process_json_t *pj) {
    int i;
    csv_dump_private_t *p = (csv_dump_private_t *) self->private;
    UNUSED(pj);
    for (i = 0;i < p->num_diags; i++) {
        fclose(p->diag_files[i]);
        p->diag_files[i] = NULL;
    }
    free(self->private);
}

static void csv_begin_block(struct process_file_dumper *self, process_json_t *pj, int block, int is_last_block) {
    csv_dump_private_t *p = (csv_dump_private_t *) self->private;
    UNUSED(pj);
    UNUSED(is_last_block);
//    if (p->has_previous_block) 
//  fprintf(pj->dump_file, ",\n");
    p->has_previous_block = 1;
    p->has_previous_diag = 0;
    p->current_block = block;
}

static void csv_end_block(struct process_file_dumper *self, process_json_t *pj, int block, int is_last_block) {
    UNUSED(self);
    UNUSED(pj);
    UNUSED(block);
    UNUSED(is_last_block);
    //csv_dump_private_t *p = (csv_dump_private_t *) self->private;
}

static void csv_sediag_start(struct process_file_dumper *self, process_json_t *pj, const char *name) {
    csv_dump_private_t *p = (csv_dump_private_t *) self->private;
    FILE *file = csv_dump_lookup_file(p, name);
    if (file == NULL)
        file = csv_dump_create_file(p, name);
    fprintf(file, "%7d, ", p->current_block);
    if (1==pj->flush_after_write) {
        fflush(file);
    }
    p->current_file = file;
    p->has_previous_diag = 1;
    p->has_previous_value = 0;
}

static void csv_sediag_item(struct process_file_dumper *self, process_json_t *pj, const char *valuestring, int is_last) {
    csv_dump_private_t *p = (csv_dump_private_t *) self->private;
    UNUSED(is_last);
    if (p->has_previous_value)
        fprintf(p->current_file, ", ");
    p->has_previous_value = 1;
    fprintf(p->current_file, "%s", valuestring);
    if (1==pj->flush_after_write) {
        fflush(p->current_file);
    }
}

static void csv_sediag_end(struct process_file_dumper *self, process_json_t *pj, int is_last) {
    csv_dump_private_t *p = (csv_dump_private_t *) self->private;
    UNUSED(is_last);
    fprintf(p->current_file, "\n");
    if (1==pj->flush_after_write) {
        fflush(p->current_file);
    }
    p->current_file = NULL;
}

struct process_file_dumper csv_dumper;

static void csv_dumper_init(struct process_file_dumper *dumper_ptr) {
    dumper_ptr->dump_file_start        = csv_dump_file_start;
    dumper_ptr->dump_file_end          = csv_dump_file_end;
    dumper_ptr->dump_file_begin_block  = csv_begin_block;
    dumper_ptr->dump_file_end_block    = csv_end_block;
    dumper_ptr->dump_file_sediag_start = csv_sediag_start;
    dumper_ptr->dump_file_sediag_item  = csv_sediag_item;
    dumper_ptr->dump_file_sediag_end   = csv_sediag_end;
}
// ************* END CSV DUMPER OBJECT *****************************


static long get_file_size(const char *filename) {
    long size;
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        printf("Couldn't open file %s\n", filename);
        exit(-1);
    }
    fseek(f, 0L, SEEK_END);
    size = ftell(f);
    fclose(f);
    return size;
}

static void set_sediag_n(cJSON *child, int diag_id, SEDiagType_t diag_type, int n, int current_block_number)
{
    printf("%5d: %s <- ", current_block_number, SEDiagGetName(diag_id));
    switch (diag_type) {
    case SE_DIAG_INT16:
        SeAssert(child->type == cJSON_Number);
        printf("%d\n",child->valueint);
        SEDiagSetInt16N(diag_id, (int16)child->valueint, n);
        break;
    case SE_DIAG_INT32:
        SeAssert(child->type == cJSON_Number);
        printf("%d\n",child->valueint);
        SEDiagSetInt32N(diag_id, (int32)child->valueint, n);
        break;
    case SE_DIAG_UINT16:
        SeAssert(child->type == cJSON_Number);
        printf("%d\n",child->valueint);
        SEDiagSetUInt16N(diag_id, (int16)child->valueint, n);
        break;
    case SE_DIAG_UINT32:
        SeAssert(child->type == cJSON_Number);
        printf("%d\n",child->valueint);
        SEDiagSetUInt32N(diag_id, (uint32)child->valueint, n);
        break;
    case SE_DIAG_INT64:
        SeAssert(child->type == cJSON_Number);
        printf("%d\n",child->valueint);
        SEDiagSetInt64N(diag_id, (int64)child->valuedouble, n);
        break;
    case SE_DIAG_UINT64:
        SeAssert(child->type == cJSON_Number);
        printf("%d\n",child->valueint);
        SEDiagSetUInt64N(diag_id, (uint64)child->valuedouble, n);
        break;
    case SE_DIAG_FLOAT32:
        SeAssert(child->type == cJSON_Number);
        printf("%f\n",child->valuedouble);
        SEDiagSetFloat32N(diag_id, (float)child->valuedouble, n);
        break;
    case SE_DIAG_ENUM:
        if (child->type == cJSON_Number) {
            printf("%d\n",child->valueint);
            SEDiagSetEnumAsIntN(diag_id, child->valueint, n);
        } else if (child->type == cJSON_String) {
            printf("%s\n",child->valuestring);
            SEDiagSetEnumAsStringN(diag_id, child->valuestring, n);
        } else {
            printf("For settings enums, you must use either a number, or a string, but we got cJSON type %d\n", child->type);
            SeAssert(0);
        }
        break;
    default:
        SeAssert(0);
        break;
    }
}

// decide whether to print a comma or a carriage return for arrays
#define BUFFERSIZE 1000
static void dump_sediag(process_json_t *pj, int diag_id, SEDiagType_t diag_type, int current_block_number, int is_last_diag_this_block)
{
    int i;
    int is_last = 0;
    char value_string[BUFFERSIZE];
    int diag_length = SEDiagGetLen(diag_id);
    UNUSED(current_block_number);

    pj->dumper->dump_file_sediag_start(pj->dumper, pj, SEDiagGetName(diag_id));
    switch (diag_type) {
    case SE_DIAG_INT16:
    {
        int16 *values = SEDiagGetInt16Array(diag_id);
        for (i = 0; i < diag_length; i++) {
            is_last = (i == diag_length-1);
            se_snprintf(value_string, BUFFERSIZE, "%d", values[i]);
            pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, is_last);
        }
    }
    break;
    case SE_DIAG_INT32:
    {
        int32 *values = SEDiagGetInt32Array(diag_id);
        for (i = 0; i < diag_length; i++) {
            is_last = (i == diag_length-1);
            se_snprintf(value_string, BUFFERSIZE, "%d", values[i]);
            pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, is_last);
        }
    }
    break;
    case SE_DIAG_UINT16:
    {
        uint16 *values = SEDiagGetUInt16Array(diag_id);
        for (i = 0; i < diag_length; i++) {
            is_last = (i == diag_length-1);
            se_snprintf(value_string, BUFFERSIZE, "%u", values[i]);
            pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, is_last);
        }
    }
    break;
    case SE_DIAG_UINT32:
    {
        uint32 *values = SEDiagGetUInt32Array(diag_id);
        for (i = 0; i < diag_length; i++) {
            is_last = (i == diag_length-1);
            se_snprintf(value_string, BUFFERSIZE, "%u", values[i]);
            pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, is_last);
        }
    }
    break;
    case SE_DIAG_INT64:
    {
        int64 *values = SEDiagGetInt64Array(diag_id);
        for (i = 0; i < diag_length; i++) {
            is_last = (i == diag_length-1);
            se_snprintf(value_string, BUFFERSIZE, "%lld", values[i]);
            pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, is_last);
        }
    }
    break;
    case SE_DIAG_UINT64:
    {
        uint64 *values = SEDiagGetUInt64Array(diag_id);
        for (i = 0; i < diag_length; i++) {
            is_last = (i == diag_length-1);
            se_snprintf(value_string, BUFFERSIZE, "%llu", values[i]);
            pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, is_last);
        }
    }
    break;
    case SE_DIAG_FLOAT32:
    {
        float32 *values = SEDiagGetFloat32Array(diag_id);
        for (i = 0; i < diag_length; i++) {
            is_last = (i == diag_length-1);
            se_snprintf(value_string, BUFFERSIZE, "%f", values[i]);
            pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, is_last);
        }
    }
    break;
    case SE_DIAG_ENUM:
        for (i = 0; i < diag_length; i++) {
            is_last = (i == diag_length-1);
            se_snprintf(value_string, BUFFERSIZE, "%s", SEDiagGetEnumAsStringN(diag_id, i));
            pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, is_last);
        }
        break;
    case SE_DIAG_CHAR:
    {
        char *values = SEDiagGetCharArray(diag_id);
        se_snprintf(value_string, BUFFERSIZE, "%s", values);
        pj->dumper->dump_file_sediag_item(pj->dumper, pj, value_string, 1);
    }
    break;
    default:
        SeAssert(0);
        break;
    }
    pj->dumper->dump_file_sediag_end(pj->dumper, pj, is_last_diag_this_block);
}

//
// write dump entry when the sediag doesnt exist
static void NO_WARN_UNUSED dump_invalid_sediag(process_json_t *pj, char *diag_name, int current_block_number, int is_last_diag_this_block)
{
    int is_last = 0;
    UNUSED(current_block_number);

    pj->dumper->dump_file_sediag_start(pj->dumper, pj, diag_name);
    pj->dumper->dump_file_sediag_item(pj->dumper, pj, "INVALID SEDIAG NAME", is_last);
    pj->dumper->dump_file_sediag_end(pj->dumper, pj, is_last_diag_this_block);
}

static int set_sediag_list(cJSON *node, int current_block_number) {
    int diag_length;
    int current_diag_setting = 0;
    char *diag_name;
    int diag_id;
    SEDiagType_t diag_type;
    SEDiagRW_t rw;
    int skip_this_node = 0;
    if (node==NULL)
    {
        return 0;
    }
    else
    {
        diag_name = node->string;
        if (diag_name[0] == '@') {
            // This is a comment, so don't process it.
            skip_this_node = 1;
        } else {
            diag_id = SEDiagGetIndex(diag_name);
            diag_type = SEDiagGetType(diag_id);
            rw = SEDiagGetRW(diag_id);
        }
    }
    if (!skip_this_node) {
        if ((rw == SE_DIAG_W) || 
            (rw == SE_DIAG_RW)) {
            // This is good, trying to write a W or RW variable.
        } else {
            // this is bad.  this is a non-writable variable.
            printf("You are attempting to set a read-only variable: %s\n",  diag_name);
            printf("Bailing out...\n");
            SeAssert(0);
        }
        diag_length = SEDiagGetLen(diag_id);
        current_diag_setting = 0;

        switch(node->type) {
        case cJSON_Number:
        case cJSON_String:
            set_sediag_n(node, diag_id, diag_type, 0, current_block_number);
            break;
        case cJSON_Array:
            if (node->child) {
                cJSON *child;
                for (current_diag_setting = 0; 
                     current_diag_setting < cJSON_GetArraySize(node); 
                     current_diag_setting++) {
                    child = cJSON_GetArrayItem(node, current_diag_setting);
                    if (current_diag_setting >= diag_length) {
                        printf("Attempting to set too many values in in diag %s.  The diag is of length %d, but you're trying to set more than that.\n",
                               diag_name, diag_length);
                        SeAssert(0);
                    }
                    set_sediag_n(child, diag_id, diag_type, current_diag_setting, current_block_number);
                }
            }
            break;
        default:
            printf("Found unexpected cJSON node type (%d) when setting diag %s.  Bailing...\n", node->type, diag_name);
            SeAssert(0);
            break;
        }
    }
    if (node->next) {
        // process the rest of the diags.
        set_sediag_list(node->next, current_block_number);
    }
    return 0;

}


static int dump_sediag_list(process_json_t *pj, cJSON *node, int current_block_number) {
    char *diag_name;
    int diag_id;
    switch(node->type) {
    case cJSON_String:
        diag_name = node->valuestring;
        if (diag_name[0] == '*') {
            // Loop through all diags!
            for (diag_id = 0; diag_id < SEDiagGetNumDiags(); diag_id++) {
                SEDiagType_t diag_type = SEDiagGetType(diag_id);
                dump_sediag(pj, diag_id, diag_type, current_block_number, (NULL==node->next) ? 1 : 0);
            }
        } else {
            SEDiagType_t diag_type;
            if (1==SEDiagExists(diag_name))
            {
                diag_id = SEDiagGetIndex(diag_name);
                diag_type = SEDiagGetType(diag_id);
                // Got a string name for a diag to process.  That's good.
                dump_sediag(pj, diag_id, diag_type, current_block_number, (NULL==node->next) ? 1 : 0);
            }
            else
            {
                //
                // the sediag doesnt exist
                printf("diag (%s) does not exist\n",diag_name);  // ry:  fail loudly and quickly
                SeAssert(0);     
                //dump_invalid_sediag(pj, diag_name, current_block_number, (NULL==node->next) ? 1 : 0);
            }
        }
        break;
    default:
        printf("Found unexpected cJSON node type (%d).  Bailing...\n", node->type);
        SeAssert(0);
        break;
    }
    if (node->next) {
        // process the rest of the diags.
        dump_sediag_list(pj, node->next, current_block_number);
    }
    return 0;
}

// open and parse the json file.
// then set the diags.
int process_json_init(process_json_t *pj, const char *json_settings_filename)
{
    long size; 
    char *text; 
    FILE *f; 
    int len_read;
    const char *dump_filename = NULL;

    memset(pj, 0, sizeof(*pj));
    if (json_settings_filename == NULL)  // nothing to do
        return 0;

    json_dumper_init(&json_dumper);
    csv_dumper_init(&csv_dumper);

    pj->dumper = &csv_dumper;
    size = get_file_size(json_settings_filename);
    text = malloc(size + 10);
    f = fopen(json_settings_filename, "r");
    len_read = fread(text, 1, size, f);
    UNUSED(len_read);
    text[size] = 0;
    fclose(f);
    pj->root = cJSON_Parse(text);
    free(text);
    if (pj->root == NULL) {
        printf("Error parsing JSON file %s\n", json_settings_filename);
        exit(-1);
    }
    pj->to_set[pj->to_set_n++]  = cJSON_GetObjectItem(pj->root, "set");
    pj->to_dump = cJSON_GetObjectItem(pj->root, "dump");
    
    if  (pj->to_dump)  {
	if (cJSON_GetObjectItem(pj->root, "dump_filename"))
	    dump_filename = cJSON_GetObjectItem(pj->root, "dump_filename")->valuestring;
        if (dump_filename == NULL) {
            printf("Error, if you specify a 'dump' parameter in the settings file, you must also specify the dump_filename parameter\n");
            return 1;
        }
	pj->dumper->dump_file_start(pj->dumper, pj, dump_filename);
    }
    pj->flush_after_write = 0;
    pj->initialized = 1;
    return 0;
}

int process_json_set_append(process_json_t *pj, const char *str)
{
    // string should be of the form "diagname=diag_value"
    cJSON *cj;
    char name[200];
    char *value;
    int n;
    char json[1000];
    value = strchr(str, '=');
    if (value == NULL)
        return 1;
    value++;
    if (strlen(str) > 200)
        return 1;
    n = value-str-1;
    memset(name, 0, sizeof(name));
    strncpy(name, str, n);
    name[n] = 0;
    sprintf(json, "[{ \"at_block\" : 0, \"diags\" : {\"%s\" : %s}}]", name, value);
    cj = cJSON_Parse(json);
    if (cj) {
        pj->to_set[pj->to_set_n++] = cj;
    } else {
        return 1;
    }
    return 0;
}

void process_json_done(process_json_t *pj)
{
    cJSON_Delete(pj->root);
    if (!pj->initialized)
        return;
    SeAssertString((NULL!=pj->dumper), "The json dumper object is NULL.  don't know why.");
    SeAssertString((NULL!=pj->dumper->dump_file_end), "The dump_file_end pointer is NULL.   That's not good.");
    if (pj->to_dump) 
	pj->dumper->dump_file_end(pj->dumper, pj);
}

// This function takes an at_block spec and a stuff_to_set spec, and goes ahead and deals with it.
// the at_block must be a scalar, either a number or string '*'
static void set_block_scalar(cJSON *at_block, cJSON *stuff_to_set, int current_block_number)
{
    switch(at_block->type) {
    case cJSON_String:
        if (at_block->valuestring[0] == '*') {
            set_sediag_list(stuff_to_set, current_block_number);
        } else {
            SeAssertString(0, "You must either specify an block number or '*', but you specified something else");
        }
        break;
    case cJSON_Number:
        if (current_block_number == at_block->valueint) {
            set_sediag_list(stuff_to_set, current_block_number);
        }
        break;
    }
}


static void dump_block_scalar(process_json_t *pj, cJSON *at_block, cJSON *stuff_to_dump, int current_block_number, int is_last_call)
{
    switch(at_block->type) {
    case cJSON_String:
        if (at_block->valuestring[0] == 'e') {
            if (is_last_call) {
                dump_sediag_list(pj, stuff_to_dump, current_block_number);
            }
        } else if  (at_block->valuestring[0] == '*') {
            dump_sediag_list(pj, stuff_to_dump, current_block_number);
        } else {
            SeAssertString(0, "You must either specify block number or 'end', but you specified something else for dumping");
        }
        break;
    case cJSON_Number:
        if (current_block_number == at_block->valueint) {
            dump_sediag_list(pj, stuff_to_dump, current_block_number);
        }
        break;
    }
}

int process_json_set(process_json_t *pj, int current_block_number)
{
    int tsn;
    for (tsn = 0; tsn < pj->to_set_n; tsn++) {
        if (pj->to_set[tsn]) {
            int i;
            // loop through the json file to see if we need to
            SeAssertString(pj->to_set[tsn]->type == cJSON_Array, "Error, your set specification must be an array.");
            for (i = 0; i < cJSON_GetArraySize(pj->to_set[tsn]); i++) {
                cJSON *set_spec = cJSON_GetArrayItem(pj->to_set[tsn], i);
                cJSON *at_block = cJSON_GetObjectItem(set_spec, "at_block");
                cJSON *diags = cJSON_GetObjectItem(set_spec, "diags");
                cJSON *stuff_to_set;
                SeAssertString((at_block != NULL), "Couldn't get \"at_block\" specification from file.");
                SeAssertString((NULL != diags), "If you specify an \"at_block\" spec, you must also specify a \"diags\" spec, but you didn't seem to do so.");
                stuff_to_set = diags->child;
                switch (at_block->type) {
                case cJSON_String:
                case cJSON_Number:
                    set_block_scalar(at_block, stuff_to_set, current_block_number);
                    break;
                case cJSON_Array:
                    {
                        int j;
                        for (j = 0; j < cJSON_GetArraySize(at_block); j++)
                            set_block_scalar(cJSON_GetArrayItem(at_block, j), stuff_to_set, current_block_number);
                    }
                    break;
                default:
                    SeAssertString(0, "Hmm, received a non string, non number, and non array block.  Not sure what to do here.");
                    break;
                }
            }
        }
    }
    return 0;
}
int process_json_dump(process_json_t *pj, int current_block_number, int is_last_call)
{
    int i;
    if (!pj->initialized)
        return 0;
    if (pj->to_dump && pj->dump_file) {
        pj->dumper->dump_file_begin_block(pj->dumper, pj, current_block_number, is_last_call);
        // loop through the json file to see if we need to
        SeAssertString(pj->to_dump->type == cJSON_Array, "Error, your dump specification must be an array.");
        for (i = 0; i < cJSON_GetArraySize(pj->to_dump); i++) {
            cJSON *dump_spec = cJSON_GetArrayItem(pj->to_dump, i);
            cJSON *at_block = cJSON_GetObjectItem(dump_spec, "at_block");
            cJSON *diags = cJSON_GetObjectItem(dump_spec, "diags");
            cJSON *stuff_to_dump = diags->child;
            int j;
            SeAssertString((at_block != NULL), "Couldn't get \"at_block\" specification from file.");
            SeAssertString((NULL != diags), "If you specify an \"at_block\" spec, you must also specify a \"diags\" spec, but you didn't seem to do so.");
            switch (at_block->type) {
            case cJSON_String:
            case cJSON_Number:
                dump_block_scalar(pj, at_block, stuff_to_dump, current_block_number, is_last_call);
                break;
            case cJSON_Array:
                for (j = 0; j < cJSON_GetArraySize(at_block); j++) {
                    dump_block_scalar(pj, cJSON_GetArrayItem(at_block, j), stuff_to_dump, current_block_number, is_last_call);
                }
                break;
            default:
                SeAssertString(0, "Hmm, received a non string, non number, and non array block.  Not sure what to do here.");
                break;
            }
        
        }
        pj->dumper->dump_file_end_block(pj->dumper, pj, current_block_number, is_last_call);
    }
    return 0;
}
