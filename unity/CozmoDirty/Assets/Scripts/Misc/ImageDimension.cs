// ImageDimension.cs
//
// Description: Types and constants for anki image resolutions
//
//  Created by Damjan Stulic on 9/18/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Anki {
  namespace Cozmo {
    public class ImageDimension {
      private ImageDimension(int w, int h) {
        width = w;
        height = h;
      }

      public int width = 0;
      public int height = 0;

      static public ImageDimension GetDimension(ImageResolution resolution) {
        if (resolutionMap == null) {
          PopulateMap();
        }
        if (resolutionMap.ContainsKey(resolution)) {
          ImageDimension dim = resolutionMap[resolution];
          return dim;
        }
        return null;
      }

      static private Dictionary<ImageResolution, ImageDimension> resolutionMap;

      static private void PopulateMap() {
        resolutionMap = new Dictionary<ImageResolution, ImageDimension>();
        resolutionMap.Add(ImageResolution.QUXGA, new ImageDimension(3200, 2400));
        resolutionMap.Add(ImageResolution.QXGA, new ImageDimension(2048, 1536));
        resolutionMap.Add(ImageResolution.UXGA, new ImageDimension(1600, 1200));
        resolutionMap.Add(ImageResolution.SXGA, new ImageDimension(1280, 1960));
        resolutionMap.Add(ImageResolution.XGA, new ImageDimension(1024, 768));
        resolutionMap.Add(ImageResolution.SVGA, new ImageDimension(800, 600));
        resolutionMap.Add(ImageResolution.VGA, new ImageDimension(640, 480));
        resolutionMap.Add(ImageResolution.CVGA, new ImageDimension(400, 296));
        resolutionMap.Add(ImageResolution.QVGA, new ImageDimension(320, 240));
        resolutionMap.Add(ImageResolution.QQVGA, new ImageDimension(160, 120));
        resolutionMap.Add(ImageResolution.QQQVGA, new ImageDimension(80, 60));
        resolutionMap.Add(ImageResolution.QQQQVGA, new ImageDimension(40, 30));
        resolutionMap.Add(ImageResolution.VerificationSnapshot, new ImageDimension(16, 16));
      }
    }



  }
  // namespace Cozmo
}
// namespace Anki

