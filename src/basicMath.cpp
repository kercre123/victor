#include "anki/common/point.h"

#include "anki/math/basicMath.h"
#include "anki/math/matrix.h"

#if defined(ANKICORETECH_USE_OPENCV)
#include "opencv2/opencv.hpp"
#endif

namespace Anki {
  
  Result Rodrigues(const Vec3f &Rvec_in, Matrix<float> &Rmat_out)
  {
#if defined(ANKICORETECH_USE_OPENCV)
    cv::Vec3f cvRvec(Rvec_in.get_CvPoint3_());
    cv::Mat_<float> cvRmat;
    cv::Rodrigues(cvRvec, cvRmat);
    assert(cvRmat.rows == 3 && cvRmat.cols == 3);
    Rmat_out = cvRmat;
#else
    assert(false);
    // TODO: implement our own version
#endif
  }
  
  Result Rodrigues(const Matrix<float> &Rmat_in, Vec3f &Rvec_out)
  {
    
  }
  
  
} // namespace Anki