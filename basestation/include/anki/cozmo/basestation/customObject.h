/**
 * File: customObject.h
 *
 * Author: Alec Solder
 * Date:   06/20/16
 *
 * Description: Implements CustomObject which is an object type that is created from external sources, such as via the SDK
 *              They can optionally be created with markers associated with them so they are observable in the world.
 *
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_CUSTOMOBJECT_H
#define ANKI_COZMO_CUSTOMOBJECT_H

#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"

#include "anki/vision/MarkerCodeDefinitions.h"


namespace Anki {
  
  namespace Cozmo {
    
    class CustomObject : public ObservableObject 
    {
    public:
      enum FaceName {
        FIRST_FACE  = 0,
        FRONT_FACE  = 0,
        LEFT_FACE   = 1,
        BACK_FACE   = 2,
        RIGHT_FACE  = 3,
        TOP_FACE    = 4,
        BOTTOM_FACE = 5,
        NUM_FACES
      };
      
      // Creates a custom object and depending on the type will either bind markers to it or not.
      // Custom_Fixed will have no markers, Custom_xxxx_Cube is a cube with the same marker on all sides
      // Custom_xxxx_Box will have unique markers on all sides and is any rectangular prism
      CustomObject(ObjectType type, const f32 xSize_mm, const f32 ySize_mm, const f32 zSize_mm,
                   const f32 markerWidth_mm = 25.f, const f32 markerHeight_mm = 25.f);
      
      virtual ~CustomObject();
      
      //
      // Inherited Virtual Methods
      //
      
      
      virtual CustomObject* CloneType() const override;
      
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
      
      virtual void Visualize(const ColorRGBA& color) const override;
      virtual void EraseVisualization() const override;
      
      virtual Point3f GetSameDistanceTolerance() const override;      
      
      // CustomObject object functions
      virtual const Point3f& GetSize() const override { return _size; }

      // TODO: Consider making this settable
      virtual bool CanIntersectWithRobot() const override { return true; }
      
    protected:

      constexpr static const f32 kSameDistToleranceFraction      = 0.8f;

      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      void DefineFacesByType(ObjectType type);
      void AddFace(const FaceName whichFace, const Vision::MarkerType code);

      Point3f _size;

      std::array<const Vision::KnownMarker*, NUM_FACES> _markersByFace;

      f32 _markerWidth_mm;
      f32 _markerHeight_mm;
      
      std::vector<Point3f> _canonicalCorners;
            
      mutable VizManager::Handle_t _vizHandle;
      
    }; // class CustomObject
    
    
    inline CustomObject* CustomObject::CloneType() const
    {
      // Call the copy constructor
      return new CustomObject(this->_type, _size.x(), _size.y(), _size.z(), _markerWidth_mm, _markerHeight_mm);
    }
    
  } // namespace Cozmo
  
} // namespace Anki

#endif // ANKI_COZMO_CUSTOMOBJECT_H
