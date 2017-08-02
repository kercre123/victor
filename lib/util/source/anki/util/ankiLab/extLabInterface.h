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

#ifndef __util_ankiLab_extLabInterface_H_
#define __util_ankiLab_extLabInterface_H_

#include "util/export/export.h"
#include <functional>
#include <map>
#include <string>

namespace Anki {
namespace Util {
namespace AnkiLab {

class AnkiLab;

using LabOperation = std::function<void(AnkiLab*)>;
using LabOpRunner = std::function<void(const LabOperation&)>;
using GetUserIdFunction = std::function<std::string()>;

// The external interface for AnkiLab needs to be initialized with...
// - a LabOpRunner - a functor that, given a LabOperation to run, will execute it and pass it an
//   instance of AnkiLab (example: dispatch to a queue, then run the operation)
// - a GetUserIdFunction that will return the current user id
void InitializeABInterface(const LabOpRunner& opRunner, const GetUserIdFunction& userIdRetriever);

ANKI_EXPORT void EnableABTesting(const bool enable);

ANKI_EXPORT bool ShouldABTestingBeDisabled();

ANKI_EXPORT void AddABTestingForcedAssignment(const std::string& experimentKey,
                                              const std::string& variationKey);

ANKI_EXPORT const std::map<std::string, std::string>& GetABTestingForcedAssignments();

ANKI_EXPORT void HandleABTestingForceURL(const std::string& urlQueryParams);

} // namespace AnkiLab
} // namespace Util
} // namespace Anki

#endif // __util_ankiLab_extLabInterface_H_
