/**
 * File: MarkerlessObject.h
 *
 * Author: Kevin Yoon
 * Date:   (various)
 *
 * Description: Implements a MarkerlessObject which ironically inherits from ObservableObject so that blockworld
 *              functionality will work for these objects. MarkerlessObjects differ from other ObservableObjects
 *              in that they are instantiated by way of processing not vision markers, but some other sensing
 *              modality like proximity sensors.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_COZMO_MARKERLESSOBJECT_H
#define ANKI_COZMO_MARKERLESSOBJECT_H

#include "engine/cozmoObservableObject.h"
#include "engine/viz/vizManager.h"

#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"

namespace Anki {
  
  namespace Vector {
    
    class MarkerlessObject : public ObservableObject // Note: Vector:: not Vision::
    {
    public:
      
      // Constructor, based on Type
      MarkerlessObject(ObjectType type);
      
      virtual ~MarkerlessObject();
      
      //
      // Inherited Virtual Methods
      //
    
      
      virtual MarkerlessObject* CloneType() const override;
      
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
      
      // Projects the box in its current 3D pose (or a given 3D pose) onto the
      // XY plane and returns the corresponding 2D quadrilateral. Pads the
      // quadrilateral (around its center) by the optional padding if desired.
      virtual Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm = 0.f) const override;
      
      virtual void Visualize(const ColorRGBA& color) const override;
      virtual void EraseVisualization() const override;
      
      virtual Point3f GetSameDistanceTolerance() const override;      
      
      // Markerless object functions
      virtual const Point3f& GetSize() const override { return _size; }
      
      static const Point3f& GetSizeByType(ObjectType type);
      
      // TODO: Consider making this settable 
      virtual bool CanIntersectWithRobot() const override { return true; }
      
    protected:
      
      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      Point3f _size;
      
      std::vector<Point3f> _canonicalCorners;
      
      mutable VizManager::Handle_t _vizHandle;
      
    }; // class MarkerlessObject
    
    
    inline MarkerlessObject* MarkerlessObject::CloneType() const
    {
      // Call the copy constructor
      return new MarkerlessObject(this->_type);
    }
    
  } // namespace Vector
  
} // namespace Anki

#endif // ANKI_COZMO_MARKERLESSOBJECT_H
