#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
#define CHECK_FOR_OUT_OF_BOUNDS
#define NUM_DIRECTIONS 8

    const Point_s16 offsets[NUM_DIRECTIONS] = {Point_s16(0,-1), Point_s16(1,-1), Point_s16(1,0), Point_s16(1,1), Point_s16(0,1), Point_s16(-1,1), Point_s16(-1,0), Point_s16(-1,-1)};
    //const s32 dxs[NUM_DIRECTIONS] = {0, 1, 1, 1, 0, -1, -1, -1};
    //const s32 dys[NUM_DIRECTIONS] = {-1, -1, 0, 1, 1, 1, 0, -1};

    //function newDirection = findNewDirection(img, curPoint, curDirection, value)
    static BoundaryDirection FindNewDirection(const Array_u8 &binaryImg, const Point_s16 &curPoint, const BoundaryDirection curDirection, const u8 value)
    {
      BoundaryDirection newDirection = BOUNDARY_UNKNOWN;

      s32 directionList1Limits[2];
      s32 directionList2Limits[2];
      if(curDirection == 0) {
        directionList1Limits[0] = NUM_DIRECTIONS-1;
        directionList1Limits[1] = NUM_DIRECTIONS;
        directionList2Limits[0] = 0;
        directionList2Limits[1] = NUM_DIRECTIONS-2;
      } else {
        directionList1Limits[0] = curDirection-1;
        directionList1Limits[1] = NUM_DIRECTIONS;
        directionList2Limits[0] = 0;
        directionList2Limits[1] = curDirection-1;
      }

      for(s32 newDirection=directionList1Limits[0]; newDirection<directionList1Limits[1]; newDirection++) {
        //const u8 imgValue = *(binaryImg.Pointer(curPoint.y+dys[newDirection], curPoint.x+dxs[newDirection]));
        const u8 imgValue = *(binaryImg.Pointer(curPoint + offsets[newDirection]));
        if(imgValue == value) {
          return static_cast<BoundaryDirection>(newDirection);
        }
      }

      for(s32 newDirection=directionList2Limits[0]; newDirection<directionList2Limits[1]; newDirection++) {
        //const u8 imgValue = *(binaryImg.Pointer(curPoint.y+dys[newDirection], curPoint.x+dxs[newDirection]));
        const u8 imgValue = *(binaryImg.Pointer(curPoint + offsets[newDirection]));
        if(imgValue == value) {
          return static_cast<BoundaryDirection>(newDirection);
        }
      }

      return BOUNDARY_UNKNOWN;
    }

    Result TraceBoundary(const Array_u8 &binaryImg, const Point_s16 &startPoint, BoundaryDirection initialDirection, FixedLengthList_Point_s16 &boundary)
    {
#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
      DASConditionalErrorAndReturnValue(binaryImg.IsValid(),
        RESULT_FAIL, "TraceBoundary", "binaryImg is not valid");

      DASConditionalErrorAndReturnValue(boundary.IsValid(),
        RESULT_FAIL, "TraceBoundary", "boundary is not valid");

      // Is the start point inside the inner pixel of the image?
      DASConditionalErrorAndReturnValue(
        startPoint.x > 0 &&
        startPoint.y > 0 &&
        static_cast<s32>(startPoint.x) < (binaryImg.get_size(1)-1) &&
        static_cast<s32>(startPoint.y) < (binaryImg.get_size(0)-1),
        RESULT_FAIL, "TraceBoundary", "startPoint is out of bounds");

      DASConditionalErrorAndReturnValue(boundary.get_maximumSize() == MAX_BOUNDARY_LENGTH,
        RESULT_FAIL, "TraceBoundary", "boundary must be have a maximum size of MAX_BOUNDARY_LENGTH");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH

      boundary.Clear();

      if(initialDirection == BOUNDARY_UNKNOWN)
        initialDirection = BOUNDARY_N;

      //if ~checkForOutOfBounds
      //    % The boundaries are zero, which means we don't have to check for out of bounds
      //    binaryImg([1,end],:) = 0;
      //    binaryImg(:, [1,end]) = 0;
      //end

      //curDirection = initialDirection;
      BoundaryDirection curDirection = initialDirection;

      //curPoint = [startPoint(1), startPoint(2)];
      Point_s16 curPoint(startPoint);

#if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH
      DASConditionalErrorAndReturnValue(*(binaryImg.Pointer(curPoint)) != 0,
        RESULT_FAIL, "TraceBoundary", "startPoint must be on a non-zero pixel of binaryImg");
#endif // #if ANKI_DEBUG_LEVEL == ANKI_DEBUG_HIGH

      // printf("0) %d %d\n", curPoint.y+1, curPoint.x+1);

      boundary.PushBack(startPoint);

      //while true
      for(s32 i=1; i<MAX_BOUNDARY_LENGTH; i++) {
        //% Search counter-clockwise until we find a 0
        const BoundaryDirection zeroDirection = FindNewDirection(binaryImg, curPoint, curDirection, 0);

        if(zeroDirection == BOUNDARY_UNKNOWN)
          break;

        //% Search counter-clockwise until we find a 1
        const s32 zeroDirectionPlusOneS32 = static_cast<s32>(zeroDirection) + 1;
        const BoundaryDirection zeroDirectionPlusOne = (zeroDirectionPlusOneS32==NUM_DIRECTIONS) ? static_cast<BoundaryDirection>(0) : static_cast<BoundaryDirection>(zeroDirectionPlusOneS32);

        const BoundaryDirection oneDirection = FindNewDirection(binaryImg, curPoint, zeroDirectionPlusOne, 1);

        //newPoint = [curPoint(1)+dy(oneDirection+1), curPoint(2)+dx(oneDirection+1)];
        //const Point2<u16> newPoint(curPoint.x+dxs[oneDirection], curPoint.y+dys[oneDirection]);
        const Point_s16 newPoint(curPoint + offsets[oneDirection]);

        // printf("%d) %d %d\n", i, newPoint.y+1, newPoint.x+1);

#ifdef CHECK_FOR_OUT_OF_BOUNDS
        /*if(newPoint.x < 0 &&
        newPoint.y < 0 &&
        newPoint.x >= binaryImg.get_size(1) &&
        newPoint.y >= binaryImg.get_size(0)) {
        return RESULT_OK;
        }*/
        if(newPoint.x <= 0 &&
          newPoint.y <= 0 &&
          static_cast<s32>(newPoint.x) >= (binaryImg.get_size(1)-1) &&
          static_cast<s32>(newPoint.y) >= (binaryImg.get_size(0)-1)) {
            return RESULT_OK;
        }
#endif //#ifdef CHECK_FOR_OUT_OF_BOUNDS

        //% If the new point is the same as the last point (in other words, if the search is stuck on a point), we're done
        if(newPoint == *boundary.Pointer(i-1)) {
          return RESULT_OK;
        }

        boundary.PushBack(newPoint);

        //% If the new point is the same as the first point, quit.
        if(newPoint == *boundary.Pointer(0)) {
          return RESULT_OK;
        }

        if(i > 1 &&
          (abs(newPoint.x - boundary.Pointer(0)->x) + abs(newPoint.y - boundary.Pointer(0)->y)) <= 1) {
            return RESULT_OK;
        }

        curPoint = newPoint;
      } // for(s32 i=0; i<MAX_BOUNDARY_LENGTH; i++) {
      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki