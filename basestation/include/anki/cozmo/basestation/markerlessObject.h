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

#include "anki/cozmo/basestation/cozmoObservableObject.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"

namespace Anki {
  
  namespace Cozmo {
    
    class MarkerlessObject : public ObservableObject // Note: Cozmo:: not Vision::
    {
    public:
      
      using Type = ObjectType;
      
      // Constructor, based on Type
      MarkerlessObject(Type type);
      
      virtual ~MarkerlessObject();
      
      //
      // Inherited Virtual Methods
      //
    
      
      virtual MarkerlessObject* CloneType() const override;
      
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
      
      virtual void Visualize(const ColorRGBA& color) override;
      virtual void EraseVisualization() override;
      
      virtual Point3f GetSameDistanceTolerance() const override;      
      
      // Markerless object functions
      virtual const Point3f& GetSize() const override { return _size; }
      
    protected:
      
      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      Point3f _size;
      
      std::vector<Point3f> _canonicalCorners;
      
      VizManager::Handle_t _vizHandle;
      
    }; // class MarkerlessObject
    
    
    inline MarkerlessObject* MarkerlessObject::CloneType() const
    {
      // Call the copy constructor
      return new MarkerlessObject(this->_type);
    }
    
  } // namespace Cozmo
  
} // namespace Anki

#endif // ANKI_COZMO_MARKERLESSOBJECT_H
