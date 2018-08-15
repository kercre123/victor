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

#ifndef ANKI_COZMO_HUMAN_HEAD_H
#define ANKI_COZMO_HUMAN_HEAD_H

#include "engine/cozmoObservableObject.h"
#include "engine/viz/vizManager.h"

#include "clad/types/objectTypes.h"
#include "clad/types/objectFamilies.h"

namespace Anki {
namespace Vector {
  
  class HumanHead : public ObservableObject // Note: Vector:: not Vision::
  {
  public:
    
    using Type = ObjectType;
    
    HumanHead(Type faceType);
  
    virtual ~HumanHead();
    
    //
    // Inherited Virtual Methods
    //
    
    virtual HumanHead* CloneType() const override;
    
    virtual void Visualize(const ColorRGBA& color) const override;
    virtual void EraseVisualization() const override;
    
    virtual const Point3f& GetSize() const override { return _size; }
    
  protected:
    
    virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
    
    Point3f _size;
    
    std::vector<Point3f> _canonicalCorners;
    
    mutable VizManager::Handle_t _vizHandle;
    
  }; // class HumanHead

  
  //
  // Inlined implementations
  //
  
  inline HumanHead* HumanHead::CloneType() const
  {
    // Call the copy constructor
    return new HumanHead(this->_type);
  }
  
  
} // namespace Vector
} // namespace Anki

#endif // ANKI_COZMO_HUMAN_HEAD_H
