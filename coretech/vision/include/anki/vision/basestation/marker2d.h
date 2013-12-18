//
//  marker2d.h
//  CoreTech_Vision
//
//  Created by Andrew Stein on 8/23/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#ifndef __CoreTech_Vision__marker2d__
#define __CoreTech_Vision__marker2d__

namespace Anki {

  // If this is all we have in this file, maybe we can put it elsewhere?
  typedef enum {
    MARKER_TOP,
    MARKER_LEFT,
    MARKER_RIGHT,
    MARKER_BOTTOM
  } MarkerUpDirection;
  
  /*
  // Abstract Marker2d class for encoding and decoding our 2D barcodes.
  // TODO: Fill this in like the Matlab class
  class Marker2d
  {
  public:

  protected:
    
    
  }; // class Marker2d
   */
  
} // namespace Anki

#endif // __CoreTech_Vision__marker2d__