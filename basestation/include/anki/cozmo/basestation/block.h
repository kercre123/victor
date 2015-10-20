//
//  block.h
//  Products_Cozmo
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __Products_Cozmo__block__
#define __Products_Cozmo__block__

#include "anki/common/basestation/math/pose.h"
#include "anki/common/basestation/math/quad.h"

#include "anki/vision/basestation/observableObject.h"

#include "clad/robotInterface/messageEngineToRobot.h"

#include "anki/vision/MarkerCodeDefinitions.h"

#include "anki/cozmo/basestation/actionableObject.h"
#include "anki/cozmo/basestation/viz/vizManager.h"

#include "clad/types/activeObjectTypes.h"

namespace Anki {
  
  // Forward Declarations:
  class Camera;
  
  namespace Cozmo {
   
    // Forward Declarations:
    class Robot;

    using FaceType = u8;
    
    //
    // Block Class
    //
    //   Representation of a physical Block in the world.
    //
    class Block : public ActionableObject 
    {
    public:
      
#include "anki/cozmo/basestation/BlockDefinitions.h"
      
      // Enumerated block types
      using Type = Cozmo::ObjectType;
      
      // NOTE: if the ordering of these is modified, you must also update
      //       the static OppositeFaceLUT.
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
      
      // "Safe" conversion from FaceType to enum FaceName (at least in Debug mode)
      //static FaceName FaceType_to_FaceName(FaceType type);
      
      enum Corners {
        LEFT_FRONT_TOP =     0,
        RIGHT_FRONT_TOP =    1,
        LEFT_FRONT_BOTTOM =  2,
        RIGHT_FRONT_BOTTOM = 3,
        LEFT_BACK_TOP =      4,
        RIGHT_BACK_TOP =     5,
        LEFT_BACK_BOTTOM =   6,
        RIGHT_BACK_BOTTOM =  7,
        NUM_CORNERS       =  8
      };
      
      enum PreActionOrientation {
        NONE  = 0x0,
        UP    = 0x01,
        LEFT  = 0x02,
        DOWN  = 0x04,
        RIGHT = 0x08,
        ALL   = UP | LEFT | DOWN | RIGHT
      };
      
      virtual ~Block();
      
      // Accessors:
      virtual const Point3f& GetSize() const override;
      
      // TODO: Promote this to ObservableObject
      const std::string& GetName()  const {return _name;}
      
      void SetName(const std::string name);
      
      void AddFace(const FaceName whichFace,
                   const Vision::MarkerType& code,
                   const float markerSize_mm,
                   const u8 dockOrientations = PreActionOrientation::ALL,
                   const u8 rollOrientations = PreActionOrientation::ALL);
            
      // Return a reference to the marker on a particular face of the block.
      // Symmetry convention: if no marker was set for the requested face, the
      // one on the opposite face is returned.  If none is defined for the
      // opposite face either, the front marker is returned.  Not having
      // a marker defined for at least the front the block is an error, (which
      // should be caught in the constructor).
      Vision::KnownMarker const& GetMarker(FaceName onFace) const;
      
      /* Defined in ObservableObject class
      // Get the block's corners at its current pose
      void GetCorners(std::array<Point3f,8>& corners) const;
      */
      // Get the block's corners at a specified pose
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const override;
      
      /*
      // Get possible poses to start docking/tracking procedure. These will be
      // a point a given distance away from each vertical face that has the
      // specified code, in the direction orthogonal to that face.  The points
      // will be w.r.t. same parent as the block, with the Z coordinate at the
      // height of the center of the block. The poses will be paired with
      // references to the corresponding marker. Optionally, only poses/markers
      // with the specified code can be returned.
      virtual void GetPreDockPoses(const float distance_mm,
                                   std::vector<PoseMarkerPair_t>& poseMarkerPairs,
                                   const Vision::Marker::Code withCode = Vision::Marker::ANY_CODE) const override;
      
      // Return the default distance from which to start docking
      virtual f32  GetDefaultPreDockDistance() const override;
      */
      
      // Projects the box in its current 3D pose (or a given 3D pose) onto the
      // XY plane and returns the corresponding 2D quadrilateral. Pads the
      // quadrilateral (around its center) by the optional padding if desired.
      virtual Quad2f GetBoundingQuadXY(const Pose3d& atPose, const f32 padding_mm = 0.f) const override;
      
      // Projects the box in its current 3D pose (or a given 3D pose) onto the
      // XY plane and returns the corresponding quadrilateral. Adds optional
      // padding if desired.
      Quad3f GetBoundingQuadInPlane(const Point3f& planeNormal, const f32 padding_mm) const;
      Quad3f GetBoundingQuadInPlane(const Point3f& planeNormal, const Pose3d& atPose, const f32 padding_mm) const;
      
      // Visualize using VizManager.
      virtual void Visualize(const ColorRGBA& color) override;
      virtual void EraseVisualization() override;
      
    protected:
      
      Block(const ObjectType type);
      Block(const ObjectFamily family, const ObjectType type);
      
      // Make this protected so we have to use public AddFace() method
      using ActionableObject::AddMarker;
      
      //std::map<Vision::Marker::Code, std::vector<FaceName> > facesWithCode;
      
      // LUT of the marker on each face, NULL if none specified.

      std::array<const Vision::KnownMarker*, NUM_FACES> markersByFace_;
      
      // Static const lookup table for all block specs, by block ID, auto-
      // generated from the BlockDefinitions.h file using macros
      typedef struct {
        FaceName             whichFace;
        Vision::MarkerType   code;
        f32                  size;
        u8                   dockOrientations; // See PreActionOrientation
        u8                   rollOrientations; // See PreActionOrientation
      } BlockFaceDef_t;
      
      typedef struct {
        std::string          name;
        ColorRGBA            color;
        Point3f              size;
        bool                 isActive;
        std::vector<BlockFaceDef_t> faces;
      } BlockInfoTableEntry_t;
      
      virtual const std::vector<Point3f>& GetCanonicalCorners() const override;
      
      static const BlockInfoTableEntry_t& LookupBlockInfo(const ObjectType type);
            
      static const std::array<Point3f,NUM_FACES> CanonicalDockingPoints;
      
      constexpr static const f32 PreDockDistance = 100.f;

      Point3f     _size;
      std::string _name;
      
      VizManager::Handle_t _vizHandle;
      
      //std::vector<Point3f> blockCorners_;
      
    }; // class Block
    
    
    // prefix operator (++fname)
    Block::FaceName& operator++(Block::FaceName& fname);
    
    // postfix operator (fname++)
    Block::FaceName operator++(Block::FaceName& fname, int);
    
    
    
    // A cubical block with the same marker on all sides.
    class Block_Cube1x1 : public Block
    {
    public:
      
      Block_Cube1x1(Type type)
      : Block(type)
      {
        // The sizes specified by the block definitions should
        // agree with this being a cube (all dimensions the same)
        CORETECH_ASSERT(_size.x() == _size.y())
        CORETECH_ASSERT(_size.y() == _size.z())
      }
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const override;
      
      virtual Block_Cube1x1* CloneType() const override
      {
        return new Block_Cube1x1(this->_type);
      }
      

    };
    
    /*
#   define MAKE_BLOCK_CLASS(__BLOCK_NAME__) \
    class __BLOCK_NAME__ : public Block_Cube1x1 { \
      public: \
      virtual __BLOCK_NAME__* Clone() const override { \
        return new __BLOCK_NAME__(*this); \
      } \
      virtual ObjectType GetType() const override { \
        static const ObjectType type(QUOTE(__BLOCK_NAME__)); \
        return type; \
    };
    
    MAKE_BLOCK_CLASS(Block_Cube1x1_AngryFace)
    */
    
    // Long dimension is along the x axis (so one unique face has x axis
    // sticking out of it, the other unique face type has y and z axes sticking
    // out of it).  One marker on
    class Block_2x1 : public Block
    {
    public:
      
      Block_2x1(Type type)
      : Block(type)
      {
        
      }
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const override;
      
      virtual Block_2x1* CloneType() const override
      {
        return new Block_2x1(this->_type);
      }
      
    protected:
      // Protected constructor using generic ObjectType
      
      
    };
    
    
    
    class ActiveCube : public Block
    {
    public:
      static const s32 NUM_LEDS = 4;
      
      ActiveCube(Type type);
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const override;
      
      virtual ActiveCube* CloneType() const override {
        return new ActiveCube(this->_type);
      }
      
      // This overrides ObservableObject::SetPose to mark this object as localized
      // anytime its pose is set
      void SetPose(const Pose3d& newPose);
      
      // Set the same color and flashing frequency of one or more LEDs on the block
      // If turnOffUnspecifiedLEDs is true, any LEDs that were not indicated by
      // whichLEDs will be turned off. Otherwise, they will be left in their current
      // state.
      // NOTE: Alpha is ignored.
      void SetLEDs(const WhichCubeLEDs whichLEDs,
                   const ColorRGBA& onColor,        const ColorRGBA& offColor,
                   const u32 onPeriod_ms,           const u32 offPeriod_ms,
                   const u32 transitionOnPeriod_ms, const u32 transitionOffPeriod_ms,
                   const bool turnOffUnspecifiedLEDs);
      
      // Specify individual colors and flash frequencies for all the LEDS of the block
      // The index of the arrays matches the diagram above.
      // NOTE: Alpha is ignored
      void SetLEDs(const std::array<u32,NUM_LEDS>& onColors,
                   const std::array<u32,NUM_LEDS>& offColors,
                   const std::array<u32,NUM_LEDS>& onPeriods_ms,
                   const std::array<u32,NUM_LEDS>& offPeriods_ms,
                   const std::array<u32,NUM_LEDS>& transitionOnPeriods_ms,
                   const std::array<u32,NUM_LEDS>& transitionOffPeriods_ms);
      
      // Make whatever state has been set on the block relative to a given (x,y)
      //  location.
      // When byUpperLeftCorner=true, "relative" means that the pattern is rotated
      //  so that whatever is currently specified for LED 0 is applied to the LED
      //  currently closest to the given position
      // When byUpperLeftCorner=false, "relative" means that the pattern is rotated
      //  so that whatever is specified for the side with LEDs 0 and 4 is applied
      //  to the face currently closest to the given position
      void MakeStateRelativeToXY(const Point2f& xyPosition, MakeRelativeMode mode);
      
      // Similar to above, but returns rotated WhichCubeLEDs rather than changing
      // the block's current state.
      WhichCubeLEDs MakeWhichLEDsRelativeToXY(const WhichCubeLEDs whichLEDs,
                                               const Point2f& xyPosition,
                                               MakeRelativeMode mode) const;
      
      // Trigger a brief change in flash/color to allow identification of this block
      // (Possibly actually flash out the block's ID? TBD...)
      virtual void Identify() override;
      
      virtual bool IsActive() const override { return true; }
      
      virtual bool CanBeUsedForLocalization() const override;
      
      virtual s32 GetActiveID() const override { return _activeID; }
      
      static void RegisterAvailableID(s32 activeID);
      static void ClearAvailableIDs();
      
      // Take the given top LED pattern and create a pattern that indicates
      // the corresponding bottom LEDs as well
      static WhichCubeLEDs MakeTopAndBottomPattern(WhichCubeLEDs topPattern);
      
      // Get the LED specification for the top (and bottom) LEDs on the corner closest
      // to the specified (x,y) position, using the ActiveCube's current pose.
      WhichCubeLEDs GetCornerClosestToXY(const Point2f& xyPosition) const;
      
      // Get the LED specification for the four LEDs on the face closest
      // to the specified (x,y) position, using the ActiveCube's current pose.
      WhichCubeLEDs GetFaceClosestToXY(const Point2f& xyPosition) const;
      
      // Rotate the currently specified pattern of colors/flashing once slot in
      // the specified direction (assuming you are looking down at the top face)
      void RotatePatternAroundTopFace(bool clockwise);
      
      // Helper for figuring out which LEDs will be selected after rotating
      // a given pattern of LEDs one slot in the specified direction
      static WhichCubeLEDs RotateWhichLEDsAroundTopFace(WhichCubeLEDs whichLEDs, bool clockwise);
      
      // Get the orientation of the top marker around the Z axis. An angle of 0
      // means the top marker is in the canonical orienation, such that the corners
      // are as shown in activeBlockTypes.h
      Radians GetTopMarkerOrientation() const;
      
      // Populate a message specifying the current state of the block, for sending
      // out to actually set the physical block to match
      //void FillMessage(SetBlockLights& msg) const;

      // TODO: Make protected/private
    //protected:
      
      // TODO: Promote to Block object
      const Vision::KnownMarker& GetTopMarker(Pose3d& markerPoseWrtOrigin) const;
      
      s32 _activeID;
      
      struct LEDstate {
        ColorRGBA onColor;
        ColorRGBA offColor;
        u32       onPeriod_ms;
        u32       offPeriod_ms;
        u32       transitionOnPeriod_ms;
        u32       transitionOffPeriod_ms;
        
        LEDstate()
        : onColor(0), offColor(0), onPeriod_ms(0), offPeriod_ms(0)
        , transitionOnPeriod_ms(0), transitionOffPeriod_ms(0)
        {
          
        }
      };
     
      // Keep track of flash rate and color of each LED
      std::array<LEDstate,NUM_LEDS> _ledState;
      
      // Map of available active IDs that the robot knows are around, and an
      // indicator of whether or not we've seen each yet.
      static std::map<s32,bool>& GetAvailableIDs();
      
      // Temporary timer for faking duration of identification process
      // TODO: Remove once real identification is implemented
      static const s32 ID_TIME_MS = 300;
      s32 _identificationTimer = ID_TIME_MS;
      
    }; // class ActiveCube
    

#pragma mark --- Inline Accessors Implementations ---
    
       

    //
    // Block:
    //
    /*
    inline BlockID_t Block::GetID() const
    { return this->blockID_; }
    */
    
    inline Point3f const& Block::GetSize() const
    { return _size; }
    
    /*
    inline float Block::GetWidth() const
    { return _size.y(); }
    
    inline float Block::GetHeight() const
    { return _size.z(); }
    
    inline float Block::GetDepth() const
    { return _size.x(); }
    */
    
    /*
    inline float Block::GetMinDim() const
    {
      return std::min(GetWidth(), std::min(GetHeight(), GetDepth()));
    }
     */
    
    /*
    inline void Block::SetSize(const float width,
                               const float height,
                               const float depth)
    {
      _size = {width, height, depth};
    }
    */
    
    /*
    inline void Block::SetColor(const unsigned char red,
                                const unsigned char green,
                                const unsigned char blue)
    {
      _color = {red, green, blue};
    }
     */
    
    inline void Block::SetName(const std::string name)
    {
      _name = name;
    }
    
    /*
    inline void Block::SetPose(const Pose3d &newPose)
    { this->pose_ = newPose; }
    */
    
    /*
    inline const BlockMarker3d& Block::get_faceMarker(const FaceName face) const
    { return this->markers[face]; }
    */
    
    /*
    inline Block::FaceName Block::FaceType_to_FaceName(FaceType type)
    {
      CORETECH_ASSERT(type > 0 && type < NUM_FACES+1);
      return static_cast<FaceName>(type-1);
    }
     */
    
    
    inline WhichCubeLEDs ActiveCube::MakeTopAndBottomPattern(WhichCubeLEDs topPattern) {
      u8 pattern = static_cast<u8>(topPattern);
      return static_cast<WhichCubeLEDs>((pattern << 4) + (pattern & 0x0F));
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__block__
