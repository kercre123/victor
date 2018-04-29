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

#include "util/audienceTags/audienceTags.h"

#include "util/logging/logging.h"
#include "util/string/stringUtils.h"

namespace Anki {
namespace Util {

void AudienceTags::RegisterTag(std::string tag, TagHandler handler)
{
  if (IsTagRegistered(tag)) {
    PRINT_NAMED_ERROR("AudienceTags.RegisterTag", "tag already registered: %s", tag.c_str());
  }
  _tagHandlers.emplace(std::move(tag), std::move(handler));
}

bool AudienceTags::IsTagRegistered(const std::string& tag) const
{
  return _tagHandlers.find(tag) != _tagHandlers.end();
}

void AudienceTags::RegisterDynamicTag(DynamicTagHandler handler)
{
  _dynamicTagHandlers.emplace_back(std::move(handler));
}

bool AudienceTags::VerifyTags(const std::vector<std::string>& tags) const
{
  bool allKnown = true;
  for (const auto& tag : tags) {
    if (_tagHandlers.find(tag) == _tagHandlers.end()) {
      allKnown = false;
      PRINT_NAMED_ERROR("AudienceTags.VerifyTags", "got unknown tag: %s", tag.c_str());
    }
  }
  return allKnown;
}

const std::vector<std::string>& AudienceTags::GetQualifiedTags() const
{
  if (!_qualifiedTags.empty()) {
    return _qualifiedTags;
  }
  else {
    return CalculateQualifiedTags();
  }
}

const std::vector<std::string>& AudienceTags::CalculateQualifiedTags() const
{
  _qualifiedTags.clear();
  for (const auto& tagHandlerPair : _tagHandlers) {
    const TagHandler& handler = tagHandlerPair.second;
    const bool qualified = handler();
    if (qualified) {
      _qualifiedTags.emplace_back(tagHandlerPair.first);
    }
  }
  for (const auto& dynamicTagHandler : _dynamicTagHandlers) {
    _qualifiedTags.emplace_back(dynamicTagHandler());
  }
  {
    // report qualified tags
    std::string tagList = Util::StringJoin(_qualifiedTags, ' ');
    PRINT_NAMED_INFO("audience_tags.qualified", "%s", tagList.c_str());
  }
  return _qualifiedTags;
}

}
}
