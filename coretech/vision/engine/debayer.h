/**
 * File: debayer.h
 *
 * Author: Patrick Doran
 * Date: 03/25/2019
 *
 * Description: Debayering functor definition. The architecture here is a common "Debayer" functor class that can have
 *              several different debayer operators registered to it via "Keys". These keys are just the set of options
 *              that a user may choose to create the different types of output. The interface is agnostic to image type
 *              because the interface takes an pointer to bytes as the "image". The interface only cares about the 
 *              bytes it's given, how they're laid out, and what the output options are.
 * 
 *              The goal with this approach is that we can easily separate out the different types of debayering that
 *              are available/implemented on a platform and then quickly be able to switch between them via the
 *              different options.
 * 
 * See Also: platform/debayer - it contains a regression tool that can create debayering output images as well as
 *           test to see if anything is changed. The stored images are the bytes that came from the existing debayering
 *           functions and this list can be extended if there are more options supported later.
 *
 * Copyright: Anki, Inc. 2019
 */

#ifndef __Anki_Coretech_Vision_Debayer_H__
#define __Anki_Coretech_Vision_Debayer_H__

#include "coretech/common/shared/types.h"

#include <memory>
#include <map>
#include <vector>

namespace Anki {
namespace Vision {

/**
 * @brief Debayer functor class. Automatically registers Debayer::Op classes that are found in the "debayer/" directory.
 */
class Debayer
{
public:
  //! Gamma correction factor
  static constexpr float GAMMA = 1.0f/2.2f;

  /**
   * @brief The method of debayering. This is akin to the level of quality of the image.
   */
  enum class Method : s32
  {
    PHOTO,          //! Make a photo quality image
    PERCEPTION      //! Make a perception quality image
  };

  /**
   * @brief The byte layout for the image
   */
  enum class Layout : s32
  {
    RAW10,
  };

  /**
   * @brief The amount to scale the bayer image. Only downscaling powers of two supported.
   */
  enum class Scale : s32
  {
    FULL,
    HALF,
    QUARTER,
    EIGHTH
  };

  /**
   * @brief The bayer pattern of the input data.
   */
  enum class Pattern : s32
  {
    RGGB,
    BGGR,
    GRBG,
    GBRG
  };

  /**
   * @brief Output format of the bytes
   */
  enum class OutputFormat : s32
  {
    RGB24,    //! 3 bytes in the order R, G, B
    Y8        //! Single byte representing luminance or grey
  };

  /**
   * @brief Input argument struct
   * @details The data pointer should point to contiguous memory of image data. The height and width of the image
   * should refer to intended height and width of the image. That is to say, if the image from the camera is 1280x720
   * and the layout format is RAW10, then the height and width of the image is 1280x720 and NOT 1600x720. The
   * information regarding actual number of bytes is communicated via height, width, and pattern.
   */
  struct InArgs
  {
    InArgs(u8* data, s32 height, s32 width, Layout layout, Pattern pattern) 
    : data(data), height(height), width(width), layout(layout), pattern(pattern)
    {}

    u8* data;
    s32 height;
    s32 width;
    Layout layout;
    Pattern pattern;
  };
  
  /**
   * @brief Output argument struct
   * @details The data pointer should point to contiguous memory of image data. The height and width of the image
   * should refer to intended height and width of the image. That is to say, if the desired image is 1280x720
   * and output format is RGB24, then the height and width of the image is still 1280x720 and NOT 3840x720. The
   * information regarding actual number of bytes is communicated via height, width, scale, and format.
   * @todo height, width are redundant because they can be computed using InArgs::height, InArgs::width, and 
   * OutArgs::scale
   */
  struct OutArgs
  {
    OutArgs(u8* data, s32 height, s32 width, Scale scale, OutputFormat format)
    : data(data), height(height), width(width), scale(scale), format(format)
    {}

    u8* data;
    s32 height;
    s32 width;
    Scale scale;
    OutputFormat format;
  };

  /**
   * @brief Interface definition for a debayering operation.
   */
  struct Op
  {
    virtual ~Op() = default;
    virtual Result operator()(const InArgs& inArgs, OutArgs& outArgs) const = 0;
  };

  /**
   * @brief Debayer constructor, registers default debayer operators.
   */
  Debayer();

  virtual ~Debayer() = default;

  /**
   * @brief Invoke debayering, return Anki::Result::RESULT_SUCCESS on success and Anki::Result::RESULT_FAIL otherwise.
   */
  Result Invoke(Method method, const InArgs& inArgs, OutArgs& outArgs) const;

  /**
   * @brief Convenience function for getting the sampling rate from the Debayer::Scale enum
   * @return true - If scale is a known value and sampleRate has been set
   * @return false - If scale is an unknown value and sampleRate has not been set
   */
  static inline bool SampleRateFromScale(Scale scale, u8& sampleRate)
  {
    switch (scale)
    {
    case Debayer::Scale::FULL: { sampleRate = 1; break; }
    case Debayer::Scale::HALF: { sampleRate = 2; break; }
    case Debayer::Scale::QUARTER: { sampleRate = 4; break; }
    case Debayer::Scale::EIGHTH: { sampleRate = 8; break; }
    default:
      return false;
    }
    return true;
  }

private:
  /**
   * @brief Convenience typedef for the tuples of options.
   */
  using Key = std::tuple<Layout,Pattern,Method,Scale,OutputFormat>;
  
  /**
   * @brief Convenience function for making tuples of options
   */
  static inline Key MakeKey(Layout layout, Pattern pattern, Method method, Scale scale, OutputFormat format) {
    return std::make_tuple(layout, pattern, method, scale, format);
  }

  /**
   * @brief Get the Op corresponding to the Key
   * 
   * @param key -  The key to look up
   * @return std::shared_ptr<Op> if it exists, nullptr otherwise 
   * 
   * @note This is private because we do not have a need to support dynamic Op registration. If this changes, consider
   * making this public.
   */
  std::shared_ptr<Op> GetOp(Key key);

  /**
   * @brief Convenience overload of GetOp
   */
  inline std::shared_ptr<Op> GetOp(Layout layout, Pattern pattern, Method method, Scale scale, OutputFormat format)
  {
    return GetOp(MakeKey(layout, pattern, method, scale, format));
  }
  
  /**
   * @brief Override the op for a specific key; replaces any existing key/op pair
   * 
   * @param method - Key for using the Op
   * @param functor - Op to use
   * 
   * @note This is private because we do not have a need to support dynamic Op registration. If this changes, consider
   * making this public.
   */
  void SetOp(Key key, std::shared_ptr<Op> op);

  /**
   * @brief Convenience overload of SetOp
   */
  inline void SetOp(Layout layout, Pattern pattern, Method method, Scale scale, OutputFormat format, std::shared_ptr<Op> op)
  {
    return SetOp(MakeKey(layout, pattern, method, scale, format), op);
  }

  /**
   * @brief Remove and return the Op corresponding to th key
   * 
   * @note This is private because we do not have a need to support dynamic Op registration. If this changes, consider
   * making this public.
   */
  void RemoveOp(Key key);

  /**
   * @brief Convenience overload of RemoveOp
   */
  inline void RemoveOp(Layout layout, Pattern pattern, Method method, Scale scale, OutputFormat format)
  {
    return RemoveOp(MakeKey(layout, pattern, method, scale, format));
  }

  /**
   * @brief Map of the different options (Debayer::Key) to an instance of a Debayer::Op that supports those options.
   */
  std::map<Key,std::shared_ptr<Op>> _ops;

};


} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Coretech_Vision_Debayer_H__ */