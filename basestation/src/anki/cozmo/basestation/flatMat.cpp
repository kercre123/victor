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
#include "anki/common/basestation/math/quad_impl.h"

namespace Anki {
  
  namespace Cozmo {
    
    const FlatMat::Type FlatMat::Type::LETTERS_4x4("LETTERS_4x4");
    const FlatMat::Type FlatMat::Type::ANKI_LOGO_8BIT("ANKI_LOGO_8BIT");
    const FlatMat::Type FlatMat::Type::LAVA_PLAYTEST("LAVA_PLAYTEST");
    const FlatMat::Type FlatMat::Type::GEARS_4x4("GEARS_4x4");
    
    static const Point3f& GetSizeByType(FlatMat::Type type)
    {
      static const std::map<FlatMat::Type, Point3f> Sizes = {
        {FlatMat::Type::LETTERS_4x4, {1000.f, 1000.f, 2.5f}},
        {FlatMat::Type::GEARS_4x4,   {1000.f, 1000.f, 2.5f}},
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
      if(Type::LETTERS_4x4 == _type) {
#         include "anki/cozmo/basestation/Mat_Letters_30mm_4x4.def"
      } else if(Type::LAVA_PLAYTEST == _type) {
#         include "anki/cozmo/basestation/Mat_LavaPlayTest.def"
      } else if(Type::ANKI_LOGO_8BIT == _type) {
//#         include "anki/cozmo/basestation/Mat_AnkiLogoPlus8Bits_8x8.def"
      } else if(Type::GEARS_4x4 == _type) {
#         include "anki/cozmo/basestation/Mat_Gears_30mm_4x4.def"
      } else {
          PRINT_NAMED_ERROR("FlatMat.UnrecognizedType", "Unknown FlatMat type specified at construction.\n");
          assert(false);
      }
    } // FlatMat(type) Constructor
    
    
    void FlatMat::GetCanonicalUnsafeRegions(const f32 padding_mm,
                                            std::vector<Quad3f>& regions) const
    {
      if(Type::LAVA_PLAYTEST == _type) {
        // Unsafe regions on the Lava play test mat are the lava regions
        regions = {{
          Quad3f({-500.f - padding_mm,  45.f + padding_mm, 0.f},
                 {-500.f - padding_mm,-105.f - padding_mm, 0.f},
                 { 500.f + padding_mm,  45.f + padding_mm, 0.f},
                 { 500.f + padding_mm,-105.f - padding_mm, 0.f}),
          
          Quad3f({-500.f - padding_mm, 15.f-260.f + padding_mm, 0.f},
                 {-500.f - padding_mm,-15.f-260.f - padding_mm, 0.f},
                 { 500.f + padding_mm, 15.f-260.f + padding_mm, 0.f},
                 { 500.f + padding_mm,-15.f-260.f - padding_mm, 0.f}),
          
          Quad3f({-55.f-280.f - padding_mm, 55.f+470.f + padding_mm, 0.f},
                 {-55.f-280.f - padding_mm,-50.f+470.f - padding_mm, 0.f},
                 { 55.f-280.f + padding_mm, 50.f+470.f + padding_mm, 0.f},
                 { 55.f-280.f + padding_mm,-50.f+470.f - padding_mm, 0.f}),
          
          Quad3f({-55.f-280.f - padding_mm, 125.f+170.f + padding_mm, 0.f},
                 {-55.f-280.f - padding_mm,-125.f+170.f - padding_mm, 0.f},
                 { 55.f-280.f + padding_mm, 125.f+170.f + padding_mm, 0.f},
                 { 55.f-280.f + padding_mm,-125.f+170.f - padding_mm, 0.f})
          
        }};
      }
      
    } // GetCanonicalUnsafeRegions()

    
    
  } // namespace Cozmo
} // namespace Anki