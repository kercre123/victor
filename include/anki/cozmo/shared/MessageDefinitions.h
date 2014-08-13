// Hold on to whatever MESSAGE_DEFINITION_MODE is since it is undef at the end of the MessageDef_Macro*.h files

#define SKIP_MESSAGE_DEFINITION_MODE_UNDEF

#include "anki/cozmo/shared/MessageDefinitionsB2R.h"

#undef SKIP_MESSAGE_DEFINITION_MODE_UNDEF

#include "anki/cozmo/shared/MessageDefinitionsR2B.h"

