
#ifndef _PROCESS_JSON_H_
#define _PROCESS_JSON_H_

#include "cjson.h"
#include <stdio.h>

/*
API that parses json files with se_diags for setting and getting.

The format of the json file expected is:
{
    "set" : [{ "at_block": <at_block_spec>, "diags": <dict_of_diags_and_values_to_set>}
               <more set blocks allowed...>],
    "dump": [{ "at_block": <at_block_spec>, "diags": <list_of_diags_to_dump>}
               <more dump blocks allowed...>],
    "dump_filename" : "some_filename.json"
}
    <at_block_spec>  : "*"          	     <--- every block
                     : [0, 10, 30]  	     <--- before blocks 0, 10, 30
                     : "end"        	     <--- after last block
                     : [0, 10, 30, "end"] <--- before blocks 0, 10, 30, and after the last.
    <dict_of_diags_and_values_to_set> :
                     : { "my_diag_1" : 32 }
                     : { "my_diag_2" : [3, 4, 6.3],
    <list_of_diags_to_dump> :
                     : [ "my_diag_1", "my_diag_2", ... ]

So, a typical file might look like this:
{
    set : [ { "at_block" : 0,
              "diags"    : {  "senr_blah_scalar" : 3.0,
                            "senr_blurg_vector": [4.3, 2, 6],
                           }
            },
            { "at_block" : 43,            <----  force something at block 43
              "diags"    : { "senr_force_thingy" : 38 }  }
           ],
    "dump_filename" : "myfile.json",
    "dump"          : [ { "at_block" : "*",
                          "diags"    : [ "current_beam", "erle" ] <--- dump every block these diags.
                        },
                        { "at_block" : "end",
                          "digas"    : [ "erle" ]
                        }
                      ]
*/
                      

struct process_file_dumper;

typedef struct {
    // The root node of the json file, after parsing.
    cJSON *root;

    FILE *dump_file;
    int    to_set_n;
    cJSON *to_set[100];
    cJSON *to_dump;
    struct process_file_dumper *dumper;
    int initialized;
    int flush_after_write;
} process_json_t;

typedef void (*dump_file_start_t       )(struct process_file_dumper *self, process_json_t *pj, const char *dump_filename); // called once at beginning processing (init time)
typedef void (*dump_file_end_t         )(struct process_file_dumper *self, process_json_t *pj); // called once at end of processing    (destruct time)
typedef void (*dump_file_begin_block_t )(struct process_file_dumper *self, process_json_t *pj, int block, int is_last_block); // called once for each block, at start of block processing.
typedef void (*dump_file_end_block_t   )(struct process_file_dumper *self, process_json_t *pj, int block, int is_last_block); // called once for each block, at end of block processing.
typedef void (*dump_file_sediag_start_t)(struct process_file_dumper *self, process_json_t *pj, const char *name); // called once for each diag, for each block
typedef void (*dump_file_sediag_item_t) (struct process_file_dumper *self, process_json_t *pj, const char *valuestring, int is_last); // called once for each diag, for each block
typedef void (*dump_file_sediag_end_t)  (struct process_file_dumper *self, process_json_t *pj, int is_last_diag_this_block); // called once for each diag, for each block

struct process_file_dumper {
    dump_file_start_t        dump_file_start;
    dump_file_end_t          dump_file_end;
    dump_file_begin_block_t  dump_file_begin_block;
    dump_file_end_block_t    dump_file_end_block;
    dump_file_sediag_start_t dump_file_sediag_start;
    dump_file_sediag_item_t  dump_file_sediag_item;
    dump_file_sediag_end_t   dump_file_sediag_end;  
    void *private; // used for each dumper to use as it wishes.
};

// process_json
//  call this once at startup.  This will slurp up the json file and check it out.
int  process_json_init(process_json_t *pj, const char *json_settings_filename);

// Cleans up and frees used memory
void process_json_done(process_json_t *pj);

// process_json_set
// call this before every block.
// this will do any setting necessary.
int  process_json_set(process_json_t *pj, int block_number);

// process_json_dump
// call this after every block
// Also, call this one more time at the very end, with is_final_call set to 1.
int  process_json_dump(process_json_t *pj, int block_number, int is_final_call);

// add a single item to set.  useful for processing command line options rather
// than using a json "set" command
int process_json_set_append(process_json_t *pj, const char *str);
#endif
