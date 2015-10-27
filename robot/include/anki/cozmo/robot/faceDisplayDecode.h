/**
 * FaceDisplayDecode.h
 *
 * Description: For decoding encoded face images from Cozmo engine.
 *
 * NOTES: 
 *   Used in hal, sim_hal, and cozmoEngineUnitTests. If we start to get more source
 *   that's shared between the robot and engine we can maybe put this in a shared lib, but
 *   for now a shared header was easiest.
 *
 **/

#ifndef FACE_DISPLAY_DECODE_H_
#define FACE_DISPLAY_DECODE_H_


static const uint64_t BIT_EXPAND[] = {
  0x0000000000000001L,
  0x0000000000000005L,
  0x0000000000000015L,
  0x0000000000000055L,
  0x0000000000000155L,
  0x0000000000000555L,
  0x0000000000001555L,
  0x0000000000005555L,
  0x0000000000015555L,
  0x0000000000055555L,
  0x0000000000155555L,
  0x0000000000555555L,
  0x0000000001555555L,
  0x0000000005555555L,
  0x0000000015555555L,
  0x0000000055555555L,
  0x0000000155555555L,
  0x0000000555555555L,
  0x0000001555555555L,
  0x0000005555555555L,
  0x0000015555555555L,
  0x0000055555555555L,
  0x0000155555555555L,
  0x0000555555555555L,
  0x0001555555555555L,
  0x0005555555555555L,
  0x0015555555555555L,
  0x0055555555555555L,
  0x0155555555555555L,
  0x0555555555555555L,
  0x1555555555555555L,
  0x5555555555555555L,
};


// Update the face to the next frame of an animation
// @param inImg - a pointer to a variable length frame of face animation data
// @param outImg - a pointer to the decoded image optimized for display on the robot.
//                 One bit/pixel, column-major ordering.
// Frame is in 8-bit RLE format:
// 00xxxxxx     CLEAR row (blank)
// 01xxxxxx     COPY PREVIOUS ROW (repeat)
// 1xxxxxyy     RLE 2-bit block
void FaceDisplayDecode(const uint8_t * inImg, const uint8_t numRows, const uint8_t numCols, uint64_t* outImg)
{
  uint64_t *frame = outImg;
  
  for (int x = 0; x < numCols; ) {
    // Full row treatment
    if (~*inImg & 0x80) {
      uint8_t op = *(inImg++);
      int rle = (op & 0x3F) + 1;
      uint64_t copy = (op & 0x40) ? *(frame - 1) : 0;
      
      x += rle;
      
      while (rle-- > 0) {
        *(frame++) = copy;
      }
      
      continue;
    }
    
    // Individual cell repeat
    uint64_t col = 0;
    
    for (int y = 0; y < numRows && *inImg & 0x80; ) {
      uint8_t op = *(inImg++);
      int rle = (op >> 2) & 0x1F;
      int pattern = op & 3;
      
      if (pattern) {
        col |= (BIT_EXPAND[rle] * pattern) << y;
      }
      
      y += (rle + 1) * 2;
    }
    
    *(frame++) = col;
    x++ ;
  }
}


#endif // FACE_DISPLAY_DECODE