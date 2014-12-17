// Hold on to whatever MESSAGE_DEFINITION_MODE is since it is undef at the end of the MessageDef_Macro*.h files

#define SKIP_MESSAGE_DEFINITION_MODE_UNDEF

#include "anki/cozmo/basestation/ui/messaging/UiMessageDefinitionsU2G.h"

#undef SKIP_MESSAGE_DEFINITION_MODE_UNDEF

#include "anki/cozmo/basestation/ui/messaging/UiMessageDefinitionsG2U.h"

