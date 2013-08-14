
#include "anki/vision.h"

using namespace Anki;

//% img is UQ8.0
//% imgFiltered is UQ8.0
//% Handles edges by replicating the border pixel
//function imgFiltered = binomialFilter(img)
Result BinomialFilter(const Matrix<u8> &img, Matrix<u8> &filteredImg, MemoryStack scratch)
{
  /*
  const u32 kernelSum = 16;

  DASConditionalError(img.get_size(0) == filteredImg.get_size(0) && img.get_size(1) == filteredImg.get_size(1),
                      "BinomialFilter", "size(img) != size(filteredImg) (%dx%d != %dx%d)", img.get_size(0), img.get_size(1), img.get_size(0), img.get_size(1));

  const u32 height = img.get_size(0);
  const u32 width = img.get_size(1);
  
  DASConditionalError(img.get_size(0) == filteredImg.get_size(0) && img.get_size(1) == filteredImg.get_size(1),
                      "BinomialFilter", "size(img) != size(filteredImg)");

  //kernelU32 = uint32([1, 4, 6, 4, 1]);
  //kernelSum = uint32(sum(kernelU32));
  //img = uint32(img);

  //imgFilteredTmp = zeros(size(img), 'uint32');
  Matrix<u32> imgFilteredTmp(height, width, scratch);
    
  //% 1. Horizontally filter
  for(u32 y=0; y<height; y++) {
  //for y = 1:size(img,1)
  //    imgFilteredTmp(y,1) = sum(img(y,1:3) .* kernelU32(3:end)) + img(y,1)*sum(kernelU32(1:2));
  //    imgFilteredTmp(y,2) = sum(img(y,1:4) .* kernelU32(2:end)) + img(y,1)*kernelU32(1);
  //    
  //    for x = 3:(size(img,2)-2)
  //        imgFilteredTmp(y,x) = sum(img(y,(x-2):(x+2)) .* kernelU32);
  //    end
  //    
  //    imgFilteredTmp(y,size(img,2)-1) = sum(img(y,(size(img,2)-3):size(img,2)) .* kernelU32(1:(end-1))) + img(y,size(img,2))*kernelU32(end);
  //    imgFilteredTmp(y,size(img,2))   = sum(img(y,(size(img,2)-2):size(img,2)) .* kernelU32(1:(end-2))) + img(y,size(img,2))*sum(kernelU32((end-1):end));
  //end
  }

  //% 2. Vertically filter
  //imgFiltered = zeros(size(img), 'uint8');
  //
  //% for y = 1:2 unrolled
  //for x = 1:size(img,2)
  //    filtered0 = sum(imgFilteredTmp(1:3,x) .* kernelU32(3:end)') + imgFilteredTmp(1,x)*sum(kernelU32(1:2));
  //    imgFiltered(1,x) = uint8( filtered0/(kernelSum^2) );

  //    filtered1 = sum(imgFilteredTmp(1:4,x) .* kernelU32(2:end)') + imgFilteredTmp(1,x)*kernelU32(1);
  //    imgFiltered(2,x) = uint8( filtered1/(kernelSum^2) );
  //end
  //
  //for y = 3:(size(img,1)-2)
  //    for x = 1:size(img,2)
  //        imgFiltered(y,x) = uint8( sum(imgFilteredTmp((y-2):(y+2),x) .* kernelU32') / (kernelSum^2) );
  //    end
  //end
  //
  //% for y = (size(img,1)-2):size(img,1) unrolled
  //for x = 1:size(img,2)
  //    filtered0 = sum(imgFilteredTmp((size(img,1)-3):size(img,1),x) .* kernelU32(1:(end-1))') + imgFilteredTmp(size(img,1),x)*kernelU32(end);
  //    imgFiltered(size(img,1)-1,x) = uint8( filtered0/(kernelSum^2) );
  //    
  //    filtered1 = sum(imgFilteredTmp((size(img,1)-2):size(img,1),x) .* kernelU32(1:(end-2))') + imgFilteredTmp(size(img,1),x)*sum(kernelU32((end-1):end));
  //    imgFiltered(size(img,1),x) = uint8( filtered1/(kernelSum^2) );
  //end
  */

  return Result::OK;
}
