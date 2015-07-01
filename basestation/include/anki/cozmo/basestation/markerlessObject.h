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

#include "anki/vision/basestation/observableObject.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

namespace Anki {
  
  namespace Cozmo {
    
    class MarkerlessObject : public Vision::ObservableObject
    {
    public:
      
      class Type : public ObjectType
      {
        Type(const std::string& name) : ObjectType(name) { }
      public:
        // Define new markerless object types here, as static const Type:
        // (Note: don't forget to instantiate each in the .cpp file)
        static const Type PROX_OBSTACLE;
      };
      
      // Constructor, based on Type
      MarkerlessObject(Type type);
      
      virtual ~MarkerlessObject();
      
      //
      // Inherited Virtual Methods
      //
      
      virtual ObjectType GetType() const override;
      
      virtual MarkerlessObject* CloneType() const override;
      
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
      
      virtual void Visualize(const ColorRGBA& color) override;
      virtual void EraseVisualization() override;
      
      virtual Point3f GetSameDistanceTolerance() const override;      
      
      // Markerless object functions
      virtual const Point3f& GetSize() const override { return _size; }
      
    protected:
      
      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      const Type _type;
      
      Point3f _size;
      
      std::vector<Point3f> _canonicalCorners;
      
      VizManager::Handle_t _vizHandle;
      
    }; // class MarkerlessObject
    
    inline ObjectType MarkerlessObject::GetType() const {
      return _type;
    }
    
    inline MarkerlessObject* MarkerlessObject::CloneType() const
    {
      // Call the copy constructor
      return new MarkerlessObject(this->_type);
    }
    
  } // namespace Cozmo
  
} // namespace Anki

#endif // ANKI_COZMO_MARKERLESSOBJECT_H
