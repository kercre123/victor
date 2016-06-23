/**
 * File: customObject.h
 *
 * Author: Alec Solder
 * Date:   06/20/16
 *
 * Description: Implements CustomObject which is an object type that is created from external sources, such as via the SDK
 *              they are unrelated to vision/sensors and the robot will trust them wholeheartedly
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

namespace Anki {
  
  namespace Cozmo {
    
    class CustomObject : public ObservableObject 
    {
    public:
      
      using Type = ObjectType;
      
      // Constructor, based on Type
      CustomObject(Type type, const f32 x_mm, const f32 y_mm, const f32 z_mm);
      
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

      
    protected:
      
      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      Point3f _size;
      
      f32 _width_mm = 20;
      f32 _depth_mm = 20;
      f32 _height_mm = 20;
      
      std::vector<Point3f> _canonicalCorners;
      
      mutable VizManager::Handle_t _vizHandle;
      
    }; // class CustomObject
    
    
    inline CustomObject* CustomObject::CloneType() const
    {
      // Call the copy constructor
      return new CustomObject(this->_type, _width_mm, _depth_mm, _height_mm);
    }
    
  } // namespace Cozmo
  
} // namespace Anki

#endif // ANKI_COZMO_CUSTOMOBJECT_H
