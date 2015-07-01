/**
 * File: humanHead.h
 *
 * Author: Andrew Stein
 * Date:   6/30/2015
 *
 * Description: A subclass of the generic ObservableObject for holding a human 
 *   head with a face on the front treated as a "Marker".
 *
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_VISION_HUMAN_HEAD_H
#define ANKI_VISION_HUMAN_HEAD_H

#include "anki/vision/basestation/observableObject.h"

namespace Anki {
namespace Vision {
  
  class HumanHead : public ObservableObject
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
    
    HumanHead(Type faceType = Type::UNKNOWN_FACE);
  
    virtual ~HumanHead();
    
    //
    // Inherited Virtual Methods
    //
    
    virtual ObjectType GetType() const override;
    
    virtual HumanHead* CloneType() const override;
    
    virtual void Visualize(const ColorRGBA& color) override;
    virtual void EraseVisualization() override;
    
    virtual const Point3f& GetSize() const override { return _size; }
    
  protected:
    
    virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
    
    const Type _type;
    
    Point3f _size;
    
    std::vector<Point3f> _canonicalCorners;
    
  }; // class HumanHead

  
  //
  // Inlined implementations
  //
  
  inline ObjectType HumanHead::GetType() const {
    return _type;
  }
  
  inline HumanHead* HumanHead::CloneType() const
  {
    // Call the copy constructor
    return new HumanHead(this->_type);
  }
  
} // namespace Vision
} // namespace Anki

#endif // ANKI_VISION_HUMAN_HEAD_H
