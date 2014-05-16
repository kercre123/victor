#ifndef __Products_Cozmo__Mat__
#define __Products_Cozmo__Mat__

#include "anki/vision/basestation/observableObject.h"

namespace Anki {
  
  namespace Cozmo {

    class MatPiece : public Vision::ObservableObject //Base<MatPiece>
    {
    public:
      MatPiece(ObjectType_t type) : Vision::ObservableObject(type) {};
      
      //virtual float GetMinDim() const {return 0;}
      
      virtual ObservableObject* Clone() const
      {
        // Create an all-new mat piece of this type, to keep all the
        // bookkeeping and pointers (e.g. the pose tree) kosher.
        MatPiece* clone = new MatPiece(this->GetType());
        
        // Move the clone to this mat piece's pose
        clone->SetPose(this->GetPose());
        
        return clone;
      }
      
      virtual std::vector<RotationMatrix3d> const& GetRotationAmbiguities() const;
      
      // Required by Vision::ObservableObject
      // NOTE: this just returns an empty vector of corners for now.
      // TODO: Actually define mat piece corners?
      virtual void GetCorners(const Pose3d& atPose, std::vector<Point3f>& corners) const {
        corners.resize(0);
      }
      
    protected:
      static const std::vector<RotationMatrix3d> rotationAmbiguities_;
      
    };
    
    
  } // namespace Cozmo

} // namespace Anki

#endif // __Products_Cozmo__Mat__
