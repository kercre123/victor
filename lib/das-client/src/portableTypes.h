#include <uuid/uuid.h>

#ifdef __linux__
typedef char __darwin_uuid_string_t[37];
typedef __darwin_uuid_string_t uuid_string_t;
#endif