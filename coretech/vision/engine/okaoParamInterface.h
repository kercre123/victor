/**
 * File:   okaoParamInterface.h
 * 
 * Author: Humphrey Hu
 * Date:   6/6/2018
 * 
 * Description: Typedefs for the OKAO library parameters
 * 
 * Copyright: Anki, Inc. 2018
 **/



#include "coretech/common/shared/types.h"
#include "util/helpers/fullEnumToValueArrayChecker.h"
#include "util/logging/logging.h"

#include <array>

namespace Anki {
namespace Vision {
namespace Okao {

  enum class PoseAngle
  {
    Unspecified = 0,
    Front,
    HalfProfile,
    Profile,
    Rear,
    _Count
  };
  
  enum class RollAngle
  {
    Upper0 = 0,
    UpperRight30,
    UpperRight60,
    Right90,
    LowerRight120,
    LowerRight150,
    Down180,
    LowerLeft210,
    LowerLeft240,
    Left270,
    UpperLeft300,
    UpperLeft330,
    All,
    None,
    UpperPm45,
    UpperLeftRightPm15,
    UpperLeftRightPm45,
    _Count
  };

  enum class RotationAngle
  {
    None = 0,
    LeftRight1,
    LeftRight2,
    All,
    _Count
  };
  
  enum class SearchDensity
  {
    Highest = 0,
    High,
    Normal,
    Low,
    Lowest,
    _Count
  };

  enum class DetectionMode
  {
    Default = 0,
    Whole,
    Partition3,
    Gradual,
    Still,
    Movie,
    _Count
  };

  enum class TrackingAccuracy
  {
    Normal = 0,
    High,
    _Count
  };

  template <typename T>
  class ParamTraits
  {
    public:
      struct ParamInfo
      {
        T enumerated;
        s32 value;
        const char* name;
      };

      using EnumArray = std::array<ParamInfo, Util::EnumToUnderlying(T::_Count)>;
      static EnumArray GetEnums();
  };

  // Returns a string that can be used for console vars
  template <typename T>
  static std::string GetConsoleString()
  {
    std::string out;
    auto enums = ParamTraits<T>::GetEnums();
    for( int i = 0; i < Util::EnumToUnderlying(T::_Count); ++i )
    {
      out += std::string(enums[i].name) + ",";
    }
    out.pop_back();
    return out;
  }


  // Get index of enum
  template <typename T>
  static T GetEnum( s32 ind )
  {
    auto enums = ParamTraits<T>::GetEnums();
    if( ind < 0 || ind >= enums.size() )
    {
      PRINT_NAMED_ERROR("ParamTraits.GetOkao.InvalidInd", "");
    }
    return enums[ind].enumerated;
  }

  // Get index of enum
  template <typename T>
  static s32 GetIndex( const T& t )
  {
    auto enums = ParamTraits<T>::GetEnums();
    for( s32 i = 0; i < Util::EnumToUnderlying(T::_Count); ++i )
    {
      if( enums[i].enumerated == t )
      {
        return i;
      }
    }
    PRINT_NAMED_ERROR("ParamTraits.GetIndex.InvalidEnum", "");
    return -1;
  }

  // Get Okao value of enum
  template <typename T>
  static s32 GetOkao( const T& t )
  {
    auto enums = ParamTraits<T>::GetEnums();
    for( int i = 0; i < Util::EnumToUnderlying(T::_Count); ++i )
    {
      if( enums[i].enumerated == t )
      {
        return enums[i].value;
      }
    }
    PRINT_NAMED_ERROR("ParamTraits.GetOkao.InvalidEnum", "");
    return -1;
  }

  // Get Okao value of index
  template <typename T>
  static s32 GetOkao( s32 ind )
  {
    auto enums = ParamTraits<T>::GetEnums();
    if( ind < 0 || ind >= enums.size() )
    {
      PRINT_NAMED_ERROR("ParamTraits.GetOkao.InvalidInd", "");
    }
    return enums[ind].value;
  }

} // end namespace Okao
} // end namespace Vision
} // end namespace Anki