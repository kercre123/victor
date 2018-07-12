/**
 * File: rejectionSamplerHelper_fwd.h
 *
 * Author: ross
 * Created: 2018 Jun 29
 *
 * Description: forward declaration for rejectionSamplerHelper.h
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Util_Random_RejectionSamplerHelperFwd_H__
#define __Util_Random_RejectionSamplerHelperFwd_H__

namespace Anki{
namespace Util{
  
template <typename T, bool ExpectConditionReordering = false>
class RejectionSamplerHelper;
  
template <typename T>
class RejectionSamplingCondition;
  
} // namespace
} // namespace

#endif // __Util_Random_RejectionSamplerHelperFwd_H__
