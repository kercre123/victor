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
        // Call the copy constructor
        return new MatPiece(static_cast<const MatPiece&>(*this));
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
