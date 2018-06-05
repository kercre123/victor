/**
 * File: imageCache.h
 *
 * Author:  Andrew Stein
 * Created: 06/05/2017
 *
 * Description: Caches resized/gray/color version of an image so that different parts of the vision system
 *              can re-use already-requested sizes/colorings of an image without needing to know about each other.
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_Basestation_ImageCache_H__
#define __Anki_Cozmo_Basestation_ImageCache_H__

#include "coretech/vision/engine/image.h"

#include <map>

namespace Anki {
namespace Vision {
  
class ImageCache
{
public:
  
  ImageCache();
  
  // Invalidate all cached data and start with new image data at scaleFactor=1.0.
  // May not release associated image memory.
  void Reset(const Image&    imgGray);
  void Reset(const ImageRGB& imgColor);
  
  // Invalidate the cache and release all the memory associated with it.
  void ReleaseMemory();
  
  // Get information about the original image from which all cached information was computed.
  // OrigNumRows and OrigNumCols are the size of the original image passed into Reset().
  // HasColor is true iff Reset was called with an ImageRGB (even if cached "colorized gray" data exists)
  s32  GetOrigNumRows() const { return _origNumRows; }
  s32  GetOrigNumCols() const { return _origNumCols; }
  bool HasColor()       const { return _hasColor;    }
  TimeStamp_t GetTimeStamp() const { return _timeStamp; }
  
  // For requesting different sizes and resize interpolation methods
  enum class Size : u8
  {
    // Original Image
    Full,
    
    // Nearest Neighbor
    Half_NN,
    Quarter_NN,
    Double_NN,
    
    // Linear Interpolation
    Half_Linear,
    Quarter_Linear,
    Double_Linear,

    // Average-Area Downsampling
    Half_AverageArea,
    Quarter_AverageArea,

    // TODO: add other sizes/methods (e.g. cubic interpolation)
  };

  
  // Get the image dimensions of the image returned for a given Size, based on
  // the original number of rows/cols.
  s32 GetNumRows(const Size atSize) const;
  s32 GetNumCols(const Size atSize) const;
  
  // Look up a Size enum, given a scale and resize method
  static Size GetSize(s32 scale, Vision::ResizeMethod method);
  
  // Interprets a scale string and a method string into a Size enum
  // Valid scales are: "full", "half", "quarter"
  // Valid methods are: "nearest", "linear", "area"
  static Size StringToSize(const std::string& scaleStr, const std::string& methodStr);

  // For unit tests to verify expected behavior
  enum class GetType : u8
  {
    NewEntry,
    ResizeIntoExisting,
    ComputeFromExisting,
    FullyCached
  };
  
  // Get color or gray image data at a given scale.
  // If Reset with a color image, the gray version will be computed by averaging (and cached).
  // If Reset with a gray image, the "RGB" version will be computed by replicating channels (and cached).
  // Thus you will always get a valid, non-empty image for both of these calls.
  // If you want to do different things depending on whether there is actually color data available (not just
  // replicated gray channels), use HasColor() above to decide which of these to call.
  // Note that the result of HasColor is not changed by calling GetRGB.
  // Note these are non-const because they could compute a resized version on demand.
  const Image&    GetGray(Size size=Size::Full, GetType* getType = nullptr);
  const ImageRGB& GetRGB(Size size=Size::Full,  GetType* getType = nullptr);
  
private:

  s32  _origNumRows = 0;
  s32  _origNumCols = 0;
  bool _hasColor = false;
  TimeStamp_t _timeStamp = 0;
  
  class ResizedEntry
  {
    Image    _gray;
    ImageRGB _rgb;
    bool     _hasValidGray;
    bool     _hasValidRGB;
    
  public:
    template<class ImageType>
    ResizedEntry(const ImageType& origImg, Size size) { Update(origImg, size); }
    
    void Invalidate() { _hasValidGray = false; _hasValidRGB = false; }
    
    template<class ImageType>
    ImageType& Get(bool computeFromOpposite = false);
    
    template<class ImageType>
    bool IsValid() const { return (_hasValidRGB || _hasValidGray); }
    
    void Update(const Image&    origImg, Size size);
    void Update(const ImageRGB& origImg, Size size);
  };
  
  std::map<Size, ResizedEntry> _resizedVersions;
  
  template<class ImageType>
  void ResetHelper(const ImageType& img);
  
  template<class ImageType>
  const ImageType& GetImageHelper(Size size, GetType& getType);
  
}; // class ImageCache
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_ImageCache_H__ */
