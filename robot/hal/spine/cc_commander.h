#ifndef CC_COMMANDER_H_
#define CC_COMMANDER_H_


#include <stdbool.h>

#include "schema/messages.h"

#ifdef __cplusplus
extern "C" {
#endif

  typedef void (*ShutdownFunction)(int);



bool ccc_commander_is_active(void);
void ccc_data_process(const struct ContactData* data);
void ccc_payload_process(const struct BodyToHead* data);
struct HeadToBody* ccc_data_get_response(void);
struct ContactData* ccc_text_response(void);

void ccc_text_process(const char* text);

void ccc_parse_command_line(int argc, const char* argv[]);
void ccc_set_shutdown_function(ShutdownFunction fp);

#ifdef __cplusplus
}
#endif


#endif//CC_COMMANDER_H_
