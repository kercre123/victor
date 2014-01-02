/*!     \file osdTextApi.h
        \defgroup osdText OSD Text component
        \{
        \brief All the functionality for The Osd Text Module can be changed and accessed from here

        Author      : Marius Truica
        Date        : 05-sept-2012
        E-mail      : marius.truica@movidius.com
        Copyright   :  Movidius Srl 2012, Movidius Ltd 2012
*/

#ifndef OsdTextApi_H
#define OsdTextApi_H

/*! !!! Important for Integration !!! */
/*! !!! in project ldscript place 
"INCLUDE osdTextCode.ldscript" 
"INCLUDE osdTextData.ldscript" 
"INCLUDE osdTextGliphBuffer.ldscript" 
where you have free space in CMX (separate section)!!! */
/*! !!! in project makefile write: "DirLDScrCommon += -L ../../../common/components/osdText" after inclusion of generic.mk   !!! */

/****************************************************************************/
/*!    1: Included types first then APIs from other modules */
/******************************************************************************/
#include "mv_types.h"
#include "stdio.h"
#include "stdarg.h"
#include <DrvLcd.h>
#include <isaac_registers.h>
#include "swcFrameTypes.h"


/****************************************************************************/
/*!    2:  Source File Specific \#defines */
/****************************************************************************/
#define OST_TEXT_SEEK_SET 1//    OST_TEXT_SEEK_SET    Beginning of file
#define OST_TEXT_SEEK_CUR 2//    OST_TEXT_SEEK_CUR    Current position of the file pointer
#define OST_TEXT_SEEK_END 3//    OST_TEXT_SEEK_END    End of file


/****************************************************************************/
/*!    3:  Exported API functions  */
/****************************************************************************/

/// Set the text osd properties, in conformity with wanted scenario. After call this function you have to call LCD start again.
///  This parameters can't be change with no LCD restart. This function will disable Graphic Layer.
///
/// @param[in]  osdTextFrame     -- pointer to overlay base frame top left corner (must be aligned to 4 bytes).
/// @param[in]  numLines              -- number of maximum lines supported by the osd text.
/// @param[in]  numCharPerLine      -- number of maximum characters per line
/// @param[in]  lineSpacingPixels   -- offer the possibility to  add extra pixel spacing between text lines; default 0
/// @param[in]  xLayerPossition     -- indicate left upper corner of text Layer position, the X coordinate
/// @param[in]  yLayerPossition     -- indicate left upper corner of text Layer position, the Y coordinate
/// @param[in]  textColor              -- text color, 0xAABBGGRR (alpha,blue,green,read)
/// @param[in]  fondColor              -- fond color, not visible in current implementation 0xAABBGGRR (alpha,blue,green,read)
/// @param[in]  makeFondTransparent -- default 1, if you will set to 0, the text layer will be visible TODO: is not implemented
///
/// @return                         -- the size that lcd out buffer need to have (for static usage the formula is (numLines*numCharPerLine*16))
///

u32  osdTextSetup(  frameBuffer * osdTextFrame,
                    u32 numLines,
                    u32 numCharPerLine,
                    u32 lineSpacingPixels,
                    u32 xLayerPossition,
                    u32 yLayerPossition,
                    u32 textColor,
                    u32 fondColor,
                    u32 makeFondTransparent
                );

/// Clean all the text layer
///
///Not necessary to know nothing from lcd start - the Layer parameters was set before, by osdTextSetup
///
/// @return     u32                    -- return 1 if everything is OK, TODO: in this moment all the time, no protection
///
u32 osdTextCleanAll(void);

/// Clean a specific portion of the text screen.
///
/// @param[in]  lineStartIdx     -- first line that will be erased
/// @param[in]  colStartIdx     -- starting with this column characters
/// @param[in]  noOffLines         -- number of lines that will be deleted
/// @param[in]  noOffCol         -- number of columns that will be deleted
///
/// @return     u32                -- return 1 if everything is OK, TODO: in this moment all the time, no protection
///
u32 osdTextCleanArea(u32 lineStartIdx,u32 colStartIdx,u32 noOffLines,u32 noOffCol);

/// Write a string on a specific position.
///
/// @param[in]  textMessage     --  String with the message that will be show.
/// @param[in]  lineIdx         -- Line that will be wrote.
/// @param[in]  colStartIdx     -- Starting position on horizontal
///
/// @return     u32                -- return 1 if everything is OK, TODO: in this moment almost all the time, increase the parameters protection
///
u32 osdTextWriteText(u8 *textMessage,u32 lineIdx, u32 colStartIdx);

/// Writes to the standard output (stdout) a sequence of data formatted as the format argument specifies. After the format parameter, the function expects at least as many additional arguments as specified in format.
///
// @param[in]  lineStartIdx     -- String that contains the text to be written to stdout.It can optionally contain embedded format tags that are substituted by the values specified in subsequent argument(s) and formatted as requested.
/// @param format string
///
/// @return     u32                -- On success, the total number of characters written is returned.On failure, a negative number is returned.
///
int osdTextprintf (const char *format, ...);

// Sets the position indicator associated with stream to the beginning of the file.
void osdTextrewind( void );

/// Sets the position indicator associated with the stream to a new position defined by adding offset to a reference position specified by origin.
///
/// @param[in]  offset     -- Number of bytes to offset from origin.
/// @param[in]  origin     -- Position from where offset is added. It is specified by one of the following constants defined in current header:
///    OST_TEXT_SEEK_SET    Beginning of file
///    OST_TEXT_SEEK_CUR    Current position of the file pointer
///    OST_TEXT_SEEK_END    End of file
///
/// @return     u32                -- If successful, the function returns a zero value.//Otherwise, it returns nonzero value.
///
int osdTextfseek (long int offset, int origin );

/// Returns the current value of the position indicator of the stream.
long int osdTextftell (void);


/// clrscr C function
void osdTextclrscr(void);

/// GotoXY positions the cursor at (X,Y), X in horizontal, Y in vertical direction relative to the origin of the current window.The origin is located at (1,1), the upper-left corner of the window.
///
/// @param[in]  X     -- deltaX in horizontal.
/// @param[in]  Y     --  deltaY in vertical direction.
///
/// @return     void
///
void osdTextgotoxy(u32 X, u32 Y);

/// Returns pointer to component color table
/// @return  pointer to color table
///
unsigned int* osdTextGetColorTable();
/// @}
#endif //OsdTextApi_H
