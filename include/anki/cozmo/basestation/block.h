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

#include "anki/vision/basestation/marker2d.h"
#include "anki/vision/basestation/observableObject.h"

#include "anki/cozmo/basestation/messages.h"

#include "anki/vision/MarkerCodeDefinitions.h"

namespace Anki {
  
  // Forward Declarations:
  class Camera;
  
  namespace Cozmo {
    
    // Forward Declarations:
    class Robot;

    typedef u8  FaceType;
    
    
    //
    // Block Class
    //
    //   Representation of a physical Block in the world.
    //
    class Block : public Vision::ObservableObject //Base<Block>
    {
    public:
      typedef Point3<unsigned char> Color;
      
#include "anki/cozmo/basestation/BlockDefinitions.h"
      
      // Enumerated block types
      enum Type {
        UNKNOWN_BLOCK_TYPE = 0,
#define BLOCK_DEFINITION_MODE BLOCK_ENUM_MODE
#include "anki/cozmo/basestation/BlockDefinitions.h"
        NUM_BLOCK_TYPES
      };
      
      enum FaceName {
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
        RIGHT_BACK_BOTTOM =  7
      };
      
      Block(const ObjectType_t type);
      
      ~Block();
      
      //static unsigned int get_numBlocks();
      
      // Accessors:
      Point3f const& GetSize() const;
      float      GetWidth()  const;
      float      GetHeight() const;
      float      GetDepth()  const;
      //virtual float GetMinDim() const;
      //using Vision::ObservableObjectBase<Block>::GetMinDim;


      void SetSize(const float width, const float height, const float depth);
      void SetColor(const float red, const float green, const float blue);
      void SetName(const std::string name);
      
      void AddFace(const FaceName whichFace,
                   const Vision::Marker::Code& code,
                   const float markerSize_mm);
      
      static Type GetBlockTypeByName(const std::string& name);
      
    protected:
      
      //static ObjectType_t NumTypes;
      
      // Make this protected so we have to use public AddFace() method
      using Vision::ObservableObject::AddMarker;
      
      // Static const lookup table for all block specs, by block ID, auto-
      // generated from the BlockDefinitions.h file using macros
      typedef struct {
        FaceName             whichFace;
        Vision::Marker::Code code;
        f32                  size;
      } BlockFaceDef_t;
      
      typedef struct {
        std::string          name;
        Block::Color         color;
        Point3f              size;
        std::vector<BlockFaceDef_t> faces;
      } BlockInfoTableEntry_t;
      
      static const BlockInfoTableEntry_t BlockInfoLUT_[NUM_BLOCK_TYPES];
      static const std::map<std::string, Type> BlockNameToTypeMap;
      
      
      Color       color_;
      Point3f     size_;
      std::string name_;
      
      std::vector<Point3f> blockCorners_;
      
    }; // class Block
    
    
    class Block_Cube1x1 : public Block
    {
    public:
      Block_Cube1x1(Type type);
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const;
      
      virtual Block* Clone() const
      {
        // Call the copy constructor
        return new Block_Cube1x1(*this);
      }
      
    protected:
      //static const ObjectType_t BlockType;
      static const std::vector<RotationMatrix3d> rotationAmbiguities_;
      
    };
    
    // Long dimension is along the x axis (so one unique face has x axis
    // sticking out of it, the other unique face type has y and z axes sticking
    // out of it)
    class Block_2x1 : public Block
    {
    public:
      Block_2x1(Type type);
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const;
      
      virtual Block* Clone() const
      {
        // Call the copy constructor
        return new Block_2x1(*this);
      }
      
    protected:
      //static const ObjectType_t BlockType;
      static const std::vector<RotationMatrix3d> rotationAmbiguities_;
      
    };
#pragma mark --- Inline Accessors Implementations ---
    
       

    //
    // Block:
    //
    /*
    inline BlockID_t Block::GetID() const
    { return this->blockID_; }
    */
    
    inline Point3f const& Block::GetSize() const
    { return this->size_; }
    
    inline float Block::GetWidth() const
    { return this->size_.y(); }
    
    inline float Block::GetHeight() const
    { return this->size_.z(); }
    
    inline float Block::GetDepth() const
    { return this->size_.x(); }
    
    /*
    inline float Block::GetMinDim() const
    {
      return std::min(GetWidth(), std::min(GetHeight(), GetDepth()));
    }
     */
    
    inline void Block::SetSize(const float width,
                               const float height,
                               const float depth)
    {
      this->size_ = {width, height, depth};
    }
    
    inline void Block::SetColor(const float red,
                                const float green,
                                const float blue)
    {
      this->color_ = {red, green, blue};
    }
    
    inline void Block::SetName(const std::string name)
    {
      this->name_ = name;
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
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__block__