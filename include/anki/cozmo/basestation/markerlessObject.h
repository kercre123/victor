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

#ifndef __Products_Cozmo__MarkerlessObject__
#define __Products_Cozmo__MarkerlessObject__

#include "anki/vision/basestation/observableObject.h"
#include "vizManager.h"

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
      
      ~MarkerlessObject();
      
      //
      // Inherited Virtual Methods
      //
      
      virtual ObjectType GetType() const override { return _type; }
      
      virtual MarkerlessObject* CloneType() const;
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const;
      
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
    
      virtual void Visualize(const ColorRGBA& color) override;
      virtual void EraseVisualization() override;
      
      virtual Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm = 0.f) const override;
      
      static ObjectType GetTypeByName(const std::string& name);
      
      virtual Point3f GetSameDistanceTolerance() const override;
      virtual Radians GetSameAngleTolerance() const override;

    
      // Markerless object functions
      void GetSize(f32& x, f32& y, f32& z);
    
    protected:
      static const std::vector<RotationMatrix3d> _rotationAmbiguities;
      static const s32 NUM_CORNERS = 8;
      static const std::array<Point3f, MarkerlessObject::NUM_CORNERS> _canonicalCorners;
      
      const Type _type;
      
      Point3f _size;
      
      VizManager::Handle_t _vizHandle;
    };
    
    
    inline MarkerlessObject* MarkerlessObject::CloneType() const
    {
      // Call the copy constructor
      return new MarkerlessObject(this->_type);
    }
    
  } // namespace Cozmo

} // namespace Anki

#endif // __Products_Cozmo__MarkerlessObject__
