/**
 * File: flatMat.cpp
 *
 * Author: Andrew Stein
 * Date:   9/15/2014
 *
 * Description: Implements a basic flat mat piece, which is just a thin rectangular
 *              surface with markers for localization.
 *
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "flatMat.h"

#include "anki/vision/MarkerCodeDefinitions.h"

namespace Anki {
  
  namespace Cozmo {
    
    const FlatMat::Type FlatMat::Type::LETTERS_4x4("LETTERS_4x4");
    
    static const Point3f& GetSizeByType(FlatMat::Type type)
    {
      static const std::map<FlatMat::Type, Point3f> Sizes = {
        {FlatMat::Type::LETTERS_4x4, {1000.f, 1000.f, 2.5f}},
      };
      
      auto iter = Sizes.find(type);
      if(iter == Sizes.end()) {
        PRINT_NAMED_ERROR("FlatMat.GetSize.UnrecognizedType",
                          "Trying to instantiate a MatPiece with an unknown Type = %d.\n", int(type));
        static const Point3f DefaultSize(1000.f, 1000.f, 2.5f);
        return DefaultSize;
      } else {
        return iter->second;
      }
    }
    
    
    FlatMat::FlatMat(Type type)
    : MatPiece(GetSizeByType(type))
    , _type(type)
    {
      //#include "anki/cozmo/basestation/Mat_AnkiLogoPlus8Bits_8x8.def"
#     include "anki/cozmo/basestation/Mat_Letters_30mm_4x4.def"
    } // MatPiece(type) Constructor
    
    
  } // namespace Cozmo
} // namespace Anki