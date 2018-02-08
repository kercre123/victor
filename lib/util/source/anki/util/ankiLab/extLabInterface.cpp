/**
 * File: extLabInterface
 *
 * Author: baustin
 * Created: 7/28/2017
 *
 * Description: External C interface for outside services to interact with AnkiLab
 *              Needs outside initialization to inject ability to access an AnkiLab instance
 *
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "util/ankiLab/extLabInterface.h"

#include "util/ankiLab/ankiLab.h"
#include "util/console/consoleInterface.h"
#include "util/helpers/ankiDefines.h"
#include "util/string/stringUtils.h"


namespace Anki {
namespace Util {
namespace AnkiLab {

static LabOpRunner sOpRunner;
static GetUserIdFunction sUserIdRetriever;

void InitializeABInterface(const LabOpRunner& opRunner, const GetUserIdFunction& userIdRetriever)
{
  sOpRunner = opRunner;
  sUserIdRetriever = userIdRetriever;
}

static void EnableAnkiLab(const bool enable) {
  if (sOpRunner) {
    sOpRunner([enable] (AnkiLab* lab) {
      lab->Enable(enable);
    });
  }
}

class DisableABTestingConsoleVar : public Anki::Util::ConsoleVar<bool>
{
public:
  DisableABTestingConsoleVar( bool& value, const char* id, const char* category, bool unregisterInDestructor )
  : ConsoleVar<bool>(value, id, category, unregisterInDestructor) { }
  void ToggleValue() override
  {
    _value = !((bool)_value);
    EnableAnkiLab(!_value);
  }
};

bool kForceDisableABTesting = (ANKI_DEVELOPER_CODE == 1);

#if REMOTE_CONSOLE_ENABLED
namespace {
  static DisableABTestingConsoleVar cvar__kForceDisableABTesting(kForceDisableABTesting,
                                                                 "kForceDisableABTesting",
                                                                 "A/B Testing",
                                                                 false);

  void ForceAssignExperiment( ConsoleFunctionContextRef context )
  {
    const char* experimentKey = ConsoleArg_Get_String(context, "experimentKey");
    const char* variationKey = ConsoleArg_Get_String(context, "variationKey");
    AddABTestingForcedAssignment(experimentKey, variationKey);
    context->channel->WriteLog("ForceAssignExperiment %s => %s", experimentKey, variationKey);
  }
  CONSOLE_FUNC(ForceAssignExperiment, "A/B Testing", const char* experimentKey, const char* variationKey );
}
#endif

void EnableABTesting(const bool enable)
{
  kForceDisableABTesting = !enable;
  EnableAnkiLab(enable);
}

static std::map<std::string, std::string> sABTestingForcedAssignments;

bool ShouldABTestingBeDisabled()
{
  return (kForceDisableABTesting && sABTestingForcedAssignments.empty());
}

void AddABTestingForcedAssignment(const std::string& experimentKey,
                                  const std::string& variationKey)
{
  EnableABTesting(true);
  if (!sOpRunner) {
    // After the engine is created and is executing its LoadData routine, it will
    // read the contents of this map and use it to force assignments.
    sABTestingForcedAssignments[experimentKey] = variationKey;
    return;
  }

  sOpRunner([experimentKey, variationKey] (AnkiLab* lab) {
    std::string userId = sUserIdRetriever();
    (void) lab->ForceActivateExperiment(experimentKey, userId, variationKey);
  });

}

const std::map<std::string, std::string>& GetABTestingForcedAssignments()
{
  return sABTestingForcedAssignments;
}

void HandleABTestingForceURL(const std::string& urlQueryParams)
{
  const std::vector<std::string> keyValStrings = StringSplit(urlQueryParams, '&');
  for (auto const& keyVal : keyValStrings) {
    const std::vector<std::string> args = StringSplit(keyVal, '=');
    if (args.size() == 2) {
      const std::string& experimentKey = args.at(0);
      const std::string& variationKey = args.at(1);
      AddABTestingForcedAssignment(experimentKey, variationKey);
    }
  }
}


} // namespace AnkiLab
} // namespace Util
} // namespace Anki
