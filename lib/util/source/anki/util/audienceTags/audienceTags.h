/**
 * File: audienceTags
 *
 * Author: baustin
 * Created: 4/6/17
 *
 * Description: Utility for keeping track which of the arbitrary categories we
 *              define users fall into
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Util_AudienceTags_AudienceTags_H__
#define __Util_AudienceTags_AudienceTags_H__

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Anki {
namespace Util {

class AudienceTags {
public:
  using TagHandler = std::function<bool()>;
  using DynamicTagHandler = std::function<std::string()>;

  void RegisterTag(std::string tag, TagHandler handler);
  bool IsTagRegistered(const std::string& tag) const;

  // Dynamic tags calculate their value and return it as a string
  // Useful for tags about locale or location where there are potentially 100s of tags
  void RegisterDynamicTag(DynamicTagHandler handler);

  // Get a set of all tags the user belongs to (can returned cached results previously calculated)
  const std::vector<std::string>& GetQualifiedTags() const;

  // Get a set of all tags the user belongs to (force re-generates the cache even if available)
  const std::vector<std::string>& CalculateQualifiedTags() const;

  // Verify that the given tags are known to exist by us
  bool VerifyTags(const std::vector<std::string>& tags) const;

private:
  std::map<std::string, TagHandler> _tagHandlers;
  std::vector<DynamicTagHandler> _dynamicTagHandlers;

  // cache results once generated
  mutable std::vector<std::string> _qualifiedTags;
};

} // namespace Util
} // namespace Anki

#endif // __Util_AudienceTags_AudienceTags_H__
