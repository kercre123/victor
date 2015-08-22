/**
 * File: platform.cpp
 *
 * Author: Andrew Stein
 * Date:   9/15/2014
 *
 * Description: Implements a Platform object.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "anki/common/types.h"

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/poseBase_impl.h"
#include "anki/common/basestation/math/quad_impl.h"

#include "anki/vision/MarkerCodeDefinitions.h"

#include "anki/cozmo/shared/cozmoConfig.h"

#include "platform.h"

namespace Anki {
  namespace Cozmo {
    
    static const Point3f& GetPlatformSize(Platform::Type type)
    {
      static const std::map<Platform::Type, Point3f> Sizes = {
        {ObjectType::Platform_LARGE, {252.f, 252.f, 44.f}},
      };
      
      auto iter = Sizes.find(type);
      if(iter == Sizes.end()) {
        PRINT_NAMED_ERROR("Platform.GetSize.UnknownPlatformType",
                          "No size defined for platform type %s (%d).\n",
                          ObjectTypeToString(type), type);
        
        static const Point3f DefaultSize(0.f, 0.f, 0.f);
        return DefaultSize;
      } else {
        return iter->second;
      }
    } // GetSize()
    
    Platform::Platform(Type type)
    : MatPiece(type, GetPlatformSize(type))
    {
      const f32& length = GetSize().x();
      const f32& width  = GetSize().y();
      //const f32& height = GetSize().z();
      
      //const f32 markerSize_sides = 30.f;
      const f32 markerSize_top   = 30.f;
      
      /* COMMENTING OUT B/C THESE MARKERS DON'T CURRENTLY EXIST IN NN LIBRARY
       
      // Front Face
      AddMarker(Vision::MARKER_PLATFORMNORTH, // Vision::MARKER_INVERTED_E,
                Pose3d(M_PI_2, Z_AXIS_3D(), {length*.5f, 0.f, -.5f*height}),
                markerSize_sides);
      
      // Back Face
      AddMarker(Vision::MARKER_PLATFORMSOUTH, //Vision::MARKER_INVERTED_RAMPBACK,
                Pose3d(-M_PI_2, Z_AXIS_3D(), {-length*.5f, 0.f, -.5f*height}),
                markerSize_sides);
      
      // Right Face
      AddMarker(Vision::MARKER_PLATFORMEAST, //Vision::MARKER_INVERTED_RAMPRIGHT,
                Pose3d(M_PI, Z_AXIS_3D(), {0, width*.5f, -.5f*height}),
                markerSize_sides);
      
      // Left Face
      AddMarker(Vision::MARKER_PLATFORMWEST, //Vision::MARKER_INVERTED_RAMPLEFT,
                Pose3d(0.f, Z_AXIS_3D(), {0, -width*.5f, -.5f*height}),
                markerSize_sides);
      */
      
      // Top Faces:
      AddMarker(Vision::MARKER_INVERTED_A, //TODO: Vision::MARKER_ANKILOGOWITHBITS001,
                Pose3d(-M_PI_2, X_AXIS_3D(), {-length*.25f, -width*.25f, 0.f}),
                markerSize_top);
      AddMarker(Vision::MARKER_INVERTED_B, //TODO: Vision::MARKER_ANKILOGOWITHBITS010,
                Pose3d(-M_PI_2, X_AXIS_3D(), {-length*.25f,  width*.25f, 0.f}),
                markerSize_top);
      AddMarker(Vision::MARKER_INVERTED_C, //TODO: Vision::MARKER_ANKILOGOWITHBITS020,
                Pose3d(-M_PI_2, X_AXIS_3D(), { length*.25f, -width*.25f, 0.f}),
                markerSize_top);
      AddMarker(Vision::MARKER_INVERTED_D, //TODO: Vision::MARKER_ANKILOGOWITHBITS030,
                Pose3d(-M_PI_2, X_AXIS_3D(), { length*.25f,  width*.25f, 0.f}),
                markerSize_top);
      
    } // Platform(type) Constructor
    
    
    void Platform::GetCanonicalUnsafeRegions(const f32 padding_mm,
                                           std::vector<Quad3f>& regions) const
    {
      // TODO: Define these geometry parameters elsewhere
      const f32 wallThickness = 3.f;
      const f32 grooveWidth = 3.75f;
      
      // Platforms have four unsafe regions around the edges, inset by the space
      // taken up by the inset for the lip (or "tongue-n-groove")
      const f32 xdim = 0.5f*GetSize().x()-wallThickness-grooveWidth;
      const f32 ydim = 0.5f*GetSize().y()-wallThickness-grooveWidth;
      regions = {{
        Quad3f({-xdim - padding_mm, ydim + padding_mm, 0.f},
               {-xdim - padding_mm, ydim - padding_mm, 0.f},
               { xdim + padding_mm, ydim + padding_mm, 0.f},
               { xdim + padding_mm, ydim - padding_mm, 0.f}),
      
        Quad3f({-xdim - padding_mm,-ydim + padding_mm, 0.f},
               {-xdim - padding_mm,-ydim - padding_mm, 0.f},
               { xdim + padding_mm,-ydim + padding_mm, 0.f},
               { xdim + padding_mm,-ydim - padding_mm, 0.f}),
        
        Quad3f({-xdim - padding_mm, ydim + padding_mm, 0.f},
               {-xdim - padding_mm,-ydim - padding_mm, 0.f},
               {-xdim + padding_mm, ydim + padding_mm, 0.f},
               {-xdim + padding_mm,-ydim - padding_mm, 0.f}),
        
        Quad3f({ xdim - padding_mm, ydim + padding_mm, 0.f},
               { xdim - padding_mm,-ydim - padding_mm, 0.f},
               { xdim + padding_mm, ydim + padding_mm, 0.f},
               { xdim + padding_mm,-ydim - padding_mm, 0.f})
      }};

    }
    
    
  } // namespace Cozmo
} // namespace Anki
