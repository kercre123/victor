/**
 * File: observedFaceObject.h
 *
 * Author: Andrew Stein
 * Date:   6/30/2015
 *
 * Description: A subclass of the generic ObservableObject for holding faces.
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_VISION_OBSERVED_FACE_OBJECT_H
#define ANKI_VISION_OBSERVED_FACE_OBJECT_H

#include "anki/vision/basestation/observableObject.h"

namespace Anki {
namespace Vision {
  
  class ObservedFaceObject : public ObservableObject
  {
  public:
    
    class Type : public ObjectType
    {
      Type(const std::string& name) : ObjectType(name) { }
    public:
      // Define new markerless object types here, as static const Type:
      // (Note: don't forget to instantiate each in the .cpp file)
      
      // TODO: Use "Types" for known vs. unknown, age, gender, or what?
      static const Type UNKNOWN_FACE;
    };
    
    ObservedFaceObject(Type faceType = Type::UNKNOWN_FACE);
  
    virtual ~ObservedFaceObject();
    
    //
    // Inherited Virtual Methods
    //
    
    virtual ObjectType GetType() const override;
    
    virtual ObservedFaceObject* CloneType() const override;
    
    virtual void Visualize(const ColorRGBA& color) override;
    virtual void EraseVisualization() override;
    
    virtual const Point3f& GetSize() const override { return _size; }
    
  protected:
    
    virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
    
    const Type _type;
    
    Point3f _size;
    
    std::vector<Point3f> _canonicalCorners;
    
  }; // class ObservedFaceObject

  
  //
  // Inlined implementations
  //
  
  inline ObjectType ObservedFaceObject::GetType() const {
    return _type;
  }
  
  inline ObservedFaceObject* ObservedFaceObject::CloneType() const
  {
    // Call the copy constructor
    return new ObservedFaceObject(this->_type);
  }
  
} // namespace Vision
} // namespace Anki

#endif // ANKI_VISION_OBSERVED_FACE_OBJECT_H
