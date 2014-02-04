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
    class Block : public Vision::ObservableObjectBase<Block>
    {
    public:
      typedef Point3<unsigned char> Color;
      
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
      static FaceName FaceType_to_FaceName(FaceType type);
      
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
      
      Block(const ObjectID_t blockID);
      Block(const Block& otherBlock);
      
      ~Block();
      
      static unsigned int get_numBlocks();
      
      // Accessors:
      float     GetWidth() const;
      float     GetHeight() const;
      float     GetDepth() const;
      float     GetMinDim() const;
      //using Vision::ObservableObjectBase<Block>::GetMinDim;
      
    protected:
      // A counter for how many blocks are instantiated
      // (A static counter may not be the best way to do this...)
      static unsigned int numBlocks;
      
      using Vision::ObservableObjectBase<Block>::ID_;
      
#include "anki/cozmo/basestation/BlockDefinitions.h"
      
      // Enumerated block IDs
      typedef enum {
        UNKNOWN_BLOCK_ID = 0,
#define BLOCK_DEFINITION_MODE BLOCK_ENUM_MODE
#include "anki/cozmo/basestation/BlockDefinitions.h"
        NUM_BLOCK_IDS
      } BlockEnumID_t;
      
      // Static const lookup table for all block specs, by block ID, auto-
      // generated from the BlockDefinitions.h file using macros
      typedef struct{
        std::string          name;
        Block::Color         color;
        Point3f              size;
        Vision::Marker::Code faceCode[6];
        f32                  faceSize[6];
      } BlockInfoTableEntry_t;
      
      static const BlockInfoTableEntry_t BlockInfoLUT_[NUM_BLOCK_IDS];
      
      Color     color_;
      Point3f   size_;
      
      std::vector<Point3f> blockCorners_;
      
      void addFace(const FaceName whichFace);
      
    }; // class Block
    
    class Block_Cube1x1 : public Block
    {
    public:
      Block_Cube1x1(const ObjectID_t ID);
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const;
      
    protected:
      static const std::vector<RotationMatrix3d> rotationAmbiguities_;
      
    };
    
    // Long dimension is along the x axis (so one unique face has x axis
    // sticking out of it, the other unique face type has y and z axes sticking
    // out of it)
    class Block_2x1 : public Block
    {
    public:
      Block_2x1(const ObjectID_t ID);
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const;
      
    protected:
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
    
    inline float Block::GetWidth() const
    { return this->size_.x(); }
    
    inline float Block::GetHeight() const
    { return this->size_.z(); }
    
    inline float Block::GetDepth() const
    { return this->size_.y(); }
    
    inline float Block::GetMinDim() const
    {
      return std::min(this->GetWidth(),
                      std::min(GetHeight(), GetDepth()));
    }
    
    /*
    inline void Block::SetPose(const Pose3d &newPose)
    { this->pose_ = newPose; }
    */
    
    /*
    inline const BlockMarker3d& Block::get_faceMarker(const FaceName face) const
    { return this->markers[face]; }
    */
    
    inline Block::FaceName Block::FaceType_to_FaceName(FaceType type)
    {
      CORETECH_ASSERT(type > 0 && type < NUM_FACES+1);
      return static_cast<FaceName>(type-1);
    }
    
  } // namespace Cozmo
} // namespace Anki

#endif // __Products_Cozmo__block__