/**
 * File: bridge.h
 *
 * Author: Andrew Stein
 * Date:   8/5/2014
 *
 * Description: Defines a Bridge object, which is a type of DockableObject,
 *              "dockable" in the sense that we use markers to line the robot 
 *              up with it, analogous to a ramp.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#if 0
#ifndef ANKI_COZMO_BASESTATION_BRIDGE_H
#define ANKI_COZMO_BASESTATION_BRIDGE_H

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

#include "anki/cozmo/basestation/messages.h"

#include "anki/vision/MarkerCodeDefinitions.h"

#include "anki/cozmo/basestation/mat.h"

#include "actionableObject.h"
#include "vizManager.h"

namespace Anki {
  
  namespace Cozmo {

    class Bridge : public ActionableObject
    {
    public:
      
      class Type : public ObjectType
      {
        Type(const std::string& name) : ObjectType(name) { }
      public:
        // Add new bridge types here (and instantiate them in the .cpp file)
        static const Type LONG;
        static const Type SHORT;
      };
      
      Bridge(Type type);
      Bridge(const Bridge& otherBridge);
      
      f32 GetWidth()  const { return _size.y(); }
      f32 GetLength() const { return _size.x(); }
      
      const Vision::KnownMarker* GetLeftEndMarker()  const { return _leftEndMarker;  }
      const Vision::KnownMarker* GetRightEndMarker() const { return _rightEndMarker; }
      const Vision::KnownMarker* GetMiddleMarker()   const { return _middleMarker;   }
      
      // Return start poses (at Bridge's current position) for going across
      // the bridge in either direction.
      void GetPreCrossingPoses(Pose3d& leftStart, Pose3d& rightStart) const;
      
      // Return final pose (at Bridge's current position) for a robot after it
      // has finished crossing the bridge.
      // TODO: Don't use wheelBase?
      Pose3d GetPostCrossingPose(const float wheelBase)  const;
      
      
      //
      // Inherited Virtual Methods
      //
      virtual ~Bridge();
      
      virtual ObjectType GetType() const override;
      
      virtual Bridge* Clone() const override;
      
      virtual void    GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
      virtual void    Visualize(const ColorRGBA& color) override;
      virtual void    EraseVisualization() override;
      virtual Quad2f  GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm = 0.f) const override;
      
      virtual Point3f GetSameDistanceTolerance()  const override;
      virtual Radians GetSameAngleTolerance()     const override;
      
    protected:
      static const int NUM_CORNERS = 8;
      static const std::array<Point3f, NUM_CORNERS> _canonicalCorners;
      
      Type    _type;
      Point3f _size;
      
      const Vision::KnownMarker* _leftEndMarker;
      const Vision::KnownMarker* _rightEndMarker;
      const Vision::KnownMarker* _middleMarker;
      
      Pose3d _preCrossingPoseLeft;
      Pose3d _preCrossingPoseRight;
      
      VizManager::Handle_t _vizHandle;
      
    }; // class Bridge
    
    
    class BridgeShort : public Bridge
    {
      virtual ObjectType GetType() const { return BridgeShort::Type; }
      
    protected:
      static const ObjectType Type;
    }; // class BridgeShort
    
    
    class BridgeLong : public Bridge
    {
      virtual ObjectType GetType() const { return BridgeLong::Type; }
      
    protected:
      static const ObjectType Type;
    }; // class BridgeShort
    
    

    
    
#if 0
#pragma mark --- Inline Implementations ---
#endif

    inline ObjectType Bridge::GetType() const {
      return _type;
    }

    inline void Bridge::GetPreCrossingPoses(Anki::Pose3d& leftStart, Anki::Pose3d& rightStart) const {
      leftStart  = _preCrossingPoseLeft;
      rightStart = _preCrossingPoseRight;
    }
    
    
    
  } // namespace Cozmo
} // namespace Anki


#endif // ANKI_COZMO_BASESTATION_BRIDGE_H

#endif
