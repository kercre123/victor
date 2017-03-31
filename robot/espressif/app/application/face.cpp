#include "face.h"
#include "messages.h"
#include "anki/cozmo/robot/faceDisplayDecode.h"
#include "anki/cozmo/robot/esp.h"
#include "anki/cozmo/robot/logging.h"
extern "C" {
  #include "driver/i2spi.h"
  #include "driver/uart.h"
}
#include <stdarg.h>

namespace Anki {
namespace Cozmo {
namespace Face {

  #define MAX_RECTS 4
  #define WORKING_RECTS (MAX_RECTS+1)

  // Overallocate so there is enough space to reuse buffer for crypto durring OTA
  static const int FRAME_ALLOC_COLS = (COLS + 12);

  typedef struct {
    u8 left;
    u8 right;
    u8 top;
    u8 bottom;
  } ScreenRect;

  typedef struct {
    bool transmitRect;
    u8 x;
    u8 y;
  } RectScanStatus;

  // 96 characters from ASCII 32 to 127, each 5x8 pixels in 5 bytes oriented vertically
  // `$` and `&` replaced by `donut` `tshirt` glyphs which combine to key icon.
  const int CHAR_WIDTH = 5, CHAR_HEIGHT = 8, CHAR_START = 32, CHAR_END = 127;
  static const u32 FONT[] ICACHE_RODATA_ATTR STORE_ATTR = {
     0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14,
     0x3E,0x7F,0x63,0x7F,0x3E, 0x23,0x13,0x08,0x64,0x62, 0x0C,0x3C,0x3C,0x3C,0x0C, 0x00,0x08,0x07,0x03,0x00,
     
     0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00, 0x2A,0x1C,0x7F,0x1C,0x2A, 0x08,0x08,0x3E,0x08,0x08,
     0x00,0x80,0x70,0x30,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x00,0x60,0x60,0x00, 0x20,0x10,0x08,0x04,0x02,
     0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x72,0x49,0x49,0x49,0x46, 0x21,0x41,0x49,0x4D,0x33,
     0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x31, 0x41,0x21,0x11,0x09,0x07,
     0x36,0x49,0x49,0x49,0x36, 0x46,0x49,0x49,0x29,0x1E, 0x00,0x00,0x14,0x00,0x00, 0x00,0x40,0x34,0x00,0x00,
     0x00,0x08,0x14,0x22,0x41, 0x14,0x14,0x14,0x14,0x14, 0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x59,0x09,0x06,
     0x3E,0x41,0x5D,0x59,0x4E, 0x7C,0x12,0x11,0x12,0x7C, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
     0x7F,0x41,0x41,0x41,0x3E, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x41,0x51,0x73,
     0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41,
     0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x1C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
     0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x26,0x49,0x49,0x49,0x32,
     0x03,0x01,0x7F,0x01,0x03, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F,
     0x63,0x14,0x08,0x14,0x63, 0x03,0x04,0x78,0x04,0x03, 0x61,0x59,0x49,0x4D,0x43, 0x00,0x7F,0x41,0x41,0x41,
     0x02,0x04,0x08,0x10,0x20, 0x00,0x41,0x41,0x41,0x7F, 0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
     0x00,0x03,0x07,0x08,0x00, 0x20,0x54,0x54,0x78,0x40, 0x7F,0x28,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x28,
     0x38,0x44,0x44,0x28,0x7F, 0x38,0x54,0x54,0x54,0x18, 0x00,0x08,0x7E,0x09,0x02, 0x18,0xA4,0xA4,0x9C,0x78,
     0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x40,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00,
     0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x78,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38,
     0xFC,0x18,0x24,0x24,0x18, 0x18,0x24,0x24,0x18,0xFC, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x24,
     0x04,0x04,0x3F,0x44,0x24, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C,
     0x44,0x28,0x10,0x28,0x44, 0x4C,0x90,0x90,0x90,0x7C, 0x44,0x64,0x54,0x4C,0x44, 0x00,0x08,0x36,0x41,0x00,
     0x00,0x00,0x77,0x00,0x00, 0x00,0x41,0x36,0x08,0x00, 0x02,0x01,0x02,0x04,0x02, 0x3C,0x26,0x23,0x26,0x3C
  };

  static const u32 BIG_DIGITS[16][16] ICACHE_RODATA_ATTR STORE_ATTR = {
    { 
      0x00000000, 0x00000000, 0x007ffe00, 0x01ffff80, 
      0x038001c0, 0x070000e0, 0x06000060, 0x06000060, 
      0x06000060, 0x06000060, 0x070000e0, 0x038001c0, 
      0x01ffff80, 0x007ffe00, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x06000000, 0x06000000, 
      0x06000040, 0x06000060, 0x06000060, 0x07ffffe0, 
      0x07ffffe0, 0x06000000, 0x06000000, 0x06000000, 
      0x06000000, 0x06000000, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x06000600, 0x07000780, 
      0x078001c0, 0x06c000e0, 0x06600060, 0x06300060, 
      0x06180060, 0x060c0060, 0x060600e0, 0x060381c0, 
      0x0601ff80, 0x06007e00, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x00600600, 0x00e00700, 
      0x038001c0, 0x070000e0, 0x06000060, 0x06018060, 
      0x06018060, 0x06018060, 0x070180e0, 0x0383c1c0, 
      0x01fe7f80, 0x007c3e00, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x003ff800, 0x003ffc00, 
      0x00300000, 0x00300000, 0x00300000, 0x00300000, 
      0x00300000, 0x07ffffe0, 0x07ffffe0, 0x00300000, 
      0x00300000, 0x00300000, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x00600000, 0x01e0ffe0,
      0x0380ffe0, 0x0700c060, 0x0600c060, 0x0600c060, 
      0x0600c060, 0x0600c060, 0x0701c060, 0x03838060, 
      0x01ff0060, 0x007c0060, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x007fc000, 0x01fff000, 
      0x0383f800, 0x0701cc00, 0x0600c600, 0x0600c300, 
      0x0600c180, 0x0600c0c0, 0x0701c060, 0x03838020,
      0x01ff0000, 0x007c0000, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x00000060, 0x00000060, 
      0x06000060, 0x07800060, 0x01e00060, 0x00780060, 
      0x001e0060, 0x00078060, 0x0001e060, 0x00007860, 
      0x00001fe0, 0x000007e0, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x007c3e00, 0x01fe7f80, 
      0x0383c1c0, 0x070180e0, 0x06018060, 0x06018060, 
      0x06018060, 0x06018060, 0x070180e0, 0x0383c1c0, 
      0x01fe7f80, 0x007c3e00, 0x00000000, 0x00000000, 
    },
    { 
      0x00000000, 0x00000000, 0x00003e00, 0x0000ff80, 
      0x0401c1c0, 0x060380e0, 0x03030060, 0x01830060, 
      0x00c30060, 0x00630060, 0x003380e0, 0x001fc1c0, 
      0x000fff80, 0x0003fe00, 0x00000000, 0x00000000
    },
    { 
      0x02000000, 0x03c00000, 0x03f80000, 0x01ff0000, 
      0x003fe000, 0x001ffc00, 0x0018ff80, 0x00181f80, 
      0x00181f80, 0x0018ff80, 0x001ffc00, 0x003fe000, 
      0x01ff0000, 0x03f80000, 0x03c00000, 0x02000000 
    },
    { 
      0x00000000, 0x00000000, 0x03ffff80, 0x03ffff80, 
      0x03ffff80, 0x03018180, 0x03018180, 0x03018180, 
      0x03018180, 0x0301c380, 0x0183ff80, 0x01ff7f00, 
      0x00fe3e00, 0x007c0000, 0x00000000, 0x00000000 
    },
    { 
      0x000fe000, 0x007ff800, 0x00fffe00, 0x01f01f00, 
      0x01c00700, 0x03800380, 0x03000180, 0x03000180, 
      0x03000180, 0x03000180, 0x03000180, 0x01800300, 
      0x01c00700, 0x01e00f00, 0x00000000, 0x00000000 
    },
    { 
      0x03ffff80, 0x03ffff80, 0x03ffff80, 0x03000180, 
      0x03000180, 0x03000180, 0x03000180, 0x03000180, 
      0x03800380, 0x01800300, 0x01c00700, 0x00f01e00, 
      0x007ffc00, 0x003ff800, 0x000fe000, 0x00000000 
    },
    { 
      0x00000000, 0x03ffff80, 0x03ffff80, 0x03ffff80, 
      0x0300c180, 0x0300c180, 0x0300c180, 0x0300c180, 
      0x0300c180, 0x0300c180, 0x0300c180, 0x0300c180, 
      0x03000180, 0x00000000, 0x00000000, 0x00000000 
    },
    { 
      0x00000000, 0x03ffff80, 0x03ffff80, 0x03ffff80, 
      0x00018180, 0x00018180, 0x00018180, 0x00018180, 
      0x00018180, 0x00018180, 0x00018180, 0x00018180, 
      0x00018180, 0x00000000, 0x00000000, 0x00000000
    }
  };
  
  #define MED_CHAR_WIDTH (10)
  
  static const u32 MED_DIGITS[10][MED_CHAR_WIDTH] ICACHE_RODATA_ATTR STORE_ATTR = {
    {0x0000, 0x03fc, 0x07fe, 0x0e87, 0x0c43, 0x0c23, 0x0e17, 0x07fe, 0x03fc, 0x0000}, // 0
    {0x0000, 0x0000, 0x0008, 0x001c, 0x000e, 0x0fff, 0x0fff, 0x0000, 0x0000, 0x0000}, // 1
    {0x0000, 0x0f06, 0x0f87, 0x0dc3, 0x0cc3, 0x0ce3, 0x0c63, 0x0c7f, 0x0c3e, 0x0000}, // 2
    {0x0000, 0x0606, 0x0e07, 0x0c03, 0x0c63, 0x0c63, 0x0c63, 0x0fff, 0x079e, 0x0000}, // 3
    {0x0000, 0x01c0, 0x01f0, 0x01fc, 0x01bc, 0x0180, 0x0fff, 0x0fff, 0x0180, 0x0000}, // 4
    {0x0000, 0x021f, 0x063f, 0x0e33, 0x0c33, 0x0c33, 0x0e73, 0x07e3, 0x03c3, 0x0000}, // 5
    {0x0000, 0x07c0, 0x0ff0, 0x0c78, 0x0c7c, 0x0c6e, 0x0ce7, 0x0fc3, 0x0781, 0x0000}, // 6
    {0x0000, 0x0003, 0x0c03, 0x0f03, 0x0fc3, 0x03f3, 0x00ff, 0x003f, 0x000f, 0x0000}, // 7
    {0x0000, 0x079e, 0x0fff, 0x0c63, 0x0c63, 0x0c63, 0x0c63, 0x0fff, 0x079e, 0x0000}, // 8
    {0x0000, 0x081e, 0x0c3f, 0x0e73, 0x0763, 0x03e3, 0x01e3, 0x00ff, 0x003e, 0x0000}, // 9
  };
  
  static const u32 MED_CAPS[26][MED_CHAR_WIDTH] ICACHE_RODATA_ATTR STORE_ATTR = {
    {0x0000, 0x0f80, 0x0ff0, 0x03fe, 0x019f, 0x019f, 0x03fe, 0x0ff0, 0x0f80, 0x0000}, // A
    {0x0000, 0x0fff, 0x0fff, 0x0c63, 0x0c63, 0x0c63, 0x0c67, 0x0ffe, 0x079c, 0x0000}, // B
    {0x0000, 0x03fc, 0x07fe, 0x0e07, 0x0c03, 0x0c03, 0x0c03, 0x0e07, 0x0606, 0x0000}, // C
    {0x0000, 0x0fff, 0x0fff, 0x0c03, 0x0c03, 0x0c07, 0x0e0e, 0x07fc, 0x03f8, 0x0000}, // D
    {0x0000, 0x0fff, 0x0fff, 0x0c63, 0x0c63, 0x0c63, 0x0c63, 0x0c63, 0x0c03, 0x0000}, // E
    {0x0000, 0x0fff, 0x0fff, 0x0063, 0x0063, 0x0063, 0x0063, 0x0003, 0x0003, 0x0000}, // F
    {0x0000, 0x03fc, 0x07fe, 0x0e07, 0x0c03, 0x0cc3, 0x0cc3, 0x0fc7, 0x07c6, 0x0000}, // G
    {0x0000, 0x0fff, 0x0fff, 0x0060, 0x0060, 0x0060, 0x0060, 0x0fff, 0x0fff, 0x0000}, // H
    {0x0000, 0x0c03, 0x0c03, 0x0c03, 0x0fff, 0x0fff, 0x0c03, 0x0c03, 0x0c03, 0x0000}, // I
    {0x0000, 0x0200, 0x0600, 0x0e00, 0x0c00, 0x0c00, 0x0e00, 0x07ff, 0x03ff, 0x0000}, // J
    {0x0000, 0x0fff, 0x0fff, 0x00f0, 0x01f8, 0x039c, 0x070e, 0x0e07, 0x0c03, 0x0000}, // K
    {0x0000, 0x0fff, 0x0fff, 0x0c00, 0x0c00, 0x0c00, 0x0c00, 0x0c00, 0x0c00, 0x0000}, // L
    {0x0000, 0x0fff, 0x0ffe, 0x0038, 0x00f0, 0x00f0, 0x0038, 0x0ffe, 0x0fff, 0x0000}, // M
    {0x0000, 0x0fff, 0x0ffe, 0x003c, 0x0070, 0x00e0, 0x03c0, 0x07ff, 0x0fff, 0x0000}, // N
    {0x0000, 0x03fc, 0x07fe, 0x0f0f, 0x0e07, 0x0e07, 0x0f0f, 0x07fe, 0x03fc, 0x0000}, // O
    {0x0000, 0x0fff, 0x0fff, 0x0063, 0x0063, 0x0063, 0x0067, 0x007e, 0x003c, 0x0000}, // P
    {0x0000, 0x03fc, 0x07fe, 0x0e07, 0x0d83, 0x0f83, 0x0707, 0x0ffe, 0x0dfc, 0x0000}, // Q
    {0x0000, 0x0fff, 0x0fff, 0x00e3, 0x01e3, 0x03e3, 0x0767, 0x0e7e, 0x0c3c, 0x0000}, // R
    {0x0000, 0x021c, 0x063e, 0x0e37, 0x0c33, 0x0c33, 0x0e77, 0x07e6, 0x03c4, 0x0000}, // S
    {0x0000, 0x0003, 0x0003, 0x0003, 0x0fff, 0x0fff, 0x0003, 0x0003, 0x0003, 0x0000}, // T
    {0x0000, 0x07ff, 0x0fff, 0x0e00, 0x0c00, 0x0c00, 0x0e00, 0x0fff, 0x07ff, 0x0000}, // U
    {0x0000, 0x003f, 0x00ff, 0x07e0, 0x0f00, 0x0f00, 0x07e0, 0x00ff, 0x003f, 0x0000}, // V
    {0x0000, 0x0fff, 0x07ff, 0x01c0, 0x00f0, 0x00f0, 0x01c0, 0x07ff, 0x0fff, 0x0000}, // W
    {0x0000, 0x0f0f, 0x0f9f, 0x01f8, 0x00f0, 0x00f0, 0x01f8, 0x0f9f, 0x0f0f, 0x0000}, // X
    {0x0000, 0x0007, 0x001f, 0x007c, 0x0ff0, 0x0ff0, 0x007c, 0x001f, 0x0007, 0x0000}, // Y
    {0x0000, 0x0f03, 0x0f83, 0x0dc3, 0x0ce3, 0x0c73, 0x0c3b, 0x0c1f, 0x0c0f, 0x0000}, // X
  };

  #define TINY_DIGIT_WIDTH 4

  static const u32 TINY_DIGITS[16] ICACHE_RODATA_ATTR STORE_ATTR = {
    0x1e29251e, // 0
    0x003f0200, // 1
    0x26292932, // 2
    0x1a252112, // 3
    0x3f040407, // 4
    0x19252527, // 5
    0x1825251e, // 6
    0x030d3101, // 7
    0x1a25251a, // 8
    0x0e152502, // 9
    0x3e09093e, // A
    0x1a25253f, // B
    0x1221211e, // C
    0x1e21213f, // D
    0x2125253f, // E
    0x0105053f, // F
  };

#define PSK_CHAR_WIDTH (8)
#define PSK_BYTES_PER_DIGIT (12)
#define PSK_DASH_OFFSET (10*PSK_BYTES_PER_DIGIT) //dash comes after 0..9
static const uint8_t PSK_DIGITS[] ICACHE_RODATA_ATTR STORE_ATTR = {
0x00,0xc0,0x1f,0xfe,0x73,0x70,0x03,0x76,0x70,0xfe,0xc3,0x1f,  // 0
0x00,0x00,0x00,0x18,0xc0,0x00,0x06,0xf0,0x7f,0xff,0x07,0x00,  // 1
0x00,0xc0,0x60,0x0e,0x37,0x78,0xc3,0x36,0x66,0x3f,0xe6,0x61,  // 2
0x00,0x60,0x18,0x87,0x33,0x70,0x33,0x36,0x63,0xff,0xe3,0x1c,  // 3
0x00,0x00,0x1c,0xe0,0x81,0x1b,0x8e,0xf1,0x7f,0xff,0x07,0x18,  // 4
0x00,0x80,0x1b,0xbf,0xf3,0x71,0x1b,0xb6,0x61,0xf3,0x33,0x1e,  // 5
0x00,0xc0,0x1f,0xfe,0x73,0x63,0x33,0x36,0x63,0xf7,0x63,0x1e,  // 6
0x00,0x30,0x00,0x03,0x37,0x7e,0xfb,0xf0,0x01,0x07,0x30,0x00,  // 7
0x00,0xc0,0x1c,0xfe,0x33,0x63,0x33,0x36,0x63,0xfe,0xc3,0x1c,  // 8
0x00,0xc0,0x33,0x7e,0x37,0x66,0x63,0x36,0x76,0xfe,0xc3,0x1f,  // 9
0x00,0x00,0x06,0x60,0x00,0x06,  //-
};

  u64 m_frame[FRAME_ALLOC_COLS];

  ScreenRect STORE_ATTR m_rects[WORKING_RECTS]; // Extra rect for working
  ScreenRect *m_activeRect;
  RectScanStatus m_scanStatus;
  int m_remainingRects;
  bool m_rectLock;
  uint32 m_lastUpdateTime;

  typedef enum {
    graphics = 0x00,
    text     = 0x01,
    debug    = 0x02,
    inverted = 0x04,
    suspended= 0x08, 
  } FaceMode;

  static s8 m_mode;
  
  static void ResetScan() {
    if (m_remainingRects > 0) {
      m_scanStatus.x = m_activeRect->left;
      m_scanStatus.y = m_activeRect->top;
      m_scanStatus.transmitRect = true;
    }
  }

  void ResetScreen() {
    if (m_mode & suspended) return;
    
    m_remainingRects = 1;
    m_activeRect = &m_rects[0];
    
    m_activeRect->left = 0;
    m_activeRect->top = 0;
    m_activeRect->right = COLS - 1;
    m_activeRect->bottom = PAGES - 1;

    memset(m_frame, 0, sizeof(m_frame));

    ResetScan();
    m_lastUpdateTime = system_get_time();
  }

  Result Init()
  {
    // Setup a single frame transfer
    ResetScreen();

    m_mode = graphics;
    return RESULT_OK;
  }
  
  #define MAX_PENALTY (-(COLS*PAGES+10))
  #define MIN(a,b) ((a < b) ? (a) : (b))
  #define MAX(a,b) ((a > b) ? (a) : (b))
  #define PIXEL(screen,x,y) (((u8*)&screen[x])[y])

  static int RectPenalty(ScreenRect& rect) {
    int width = rect.right - rect.left + 1;
    int height = rect.bottom - rect.top + 1;

    return width * height + 10;
  }

  static void MergeRects(ScreenRect& c, ScreenRect& a, ScreenRect& b) {
    c.left = MIN(a.left, b.left);
    c.top = MIN(a.top, b.top);
    c.right = MAX(a.right, b.right);
    c.bottom = MAX(a.bottom, b.bottom);
  }

  static bool ConsolidateRects(bool force = false) {
    int best_delta = MAX_PENALTY, best_a = 0, best_b = 0;
    ScreenRect best_merged;

    int a_top = m_remainingRects - 1,
        b_top = m_remainingRects;

    // Locate the best consolidation candidate
    for (int a = 0; a < a_top; a++) {
      for (int b = a+1; b < b_top; b++) {
        ScreenRect merged;
        MergeRects(merged, m_rects[a], m_rects[b]);

        // Just look at those savings
        int score =
            RectPenalty(m_rects[a])
          + RectPenalty(m_rects[b])
          - RectPenalty(merged);

        if (score > best_delta) {
          best_delta = score;
          best_merged = merged;
          best_a = a;
          best_b = b;
        }
      }
    }

    // Consolidate the rectangle down
    if (force || best_delta >= 0) {
      m_rects[best_a] = best_merged;
      m_rects[best_b] = m_rects[--m_remainingRects];
      return true;
    }

    return false;
  }

  static void CreateRects(u64* frame, u8 sx = 0, u8 sy = 0, u8 ex = COLS, u8 ey = PAGES) {
    if (m_mode & suspended) return;
    // We cannot create new rects while we are currently transmitting
    if (m_remainingRects > 0) return ;

    m_rectLock = true;

    m_activeRect = &m_rects[0];
    m_remainingRects = 0;

    for (int y = sy; y < ey; y++) {
      for (int x = sx; x < ex; x++) {
        // Find dirty pixels
        if (PIXEL(m_frame, x, y) == PIXEL(frame, x, y)) {
          continue ;
        }

        // Create a new rectangle, expanding horizontally
        ScreenRect *working = &m_rects[m_remainingRects++];

        working->top = working->bottom = y;
        working->left = x;

        // Find bounding rectangle for horizontal strip
        // ... while coping the dirty data in
        do {
          PIXEL(m_frame, x, y) = PIXEL(frame, x, y);
          working->right = x;
        } while(++x < COLS && PIXEL(m_frame, x, y) != PIXEL(frame, x, y));

        // Keep our rectangle count down
        if (m_remainingRects > MAX_RECTS) {
          ConsolidateRects(true);
        }
      }
    }

    // Reduce total rect count
    while (ConsolidateRects()) ;

    ResetScan();
    m_lastUpdateTime = system_get_time();

    m_rectLock = false;
  }

  void FillScreenData(void)
  {
    if (m_mode & suspended) return;
    
    s8 wordsToFill = i2spiGetScreenBufferAvailable();
    //if (wordsToFill) os_printf("FSD %d\r\n", wordsToFill);
    
    while (wordsToFill > 0)
    {
      wordsToFill--;

      if (m_rectLock || m_remainingRects <= 0) {
        return;
      }

      // We need to update the bounding box
      if (m_scanStatus.transmitRect) {
        m_scanStatus.transmitRect = false;
        i2spiPushScreenData((const uint32_t*)m_activeRect, true);
      }
      else
      {
        // Transmit the screen bytes
        uint32_t data = 0;
        uint8_t* dest = (uint8_t*)&data;
        for (int i = 0; i < MAX_SCREEN_BYTES_PER_DROP; i++)
        {
          dest[i] = PIXEL(m_frame, m_scanStatus.x, m_scanStatus.y);

          // Advance cursor through the rectangle
          if (++m_scanStatus.x > m_activeRect->right)
          {
            m_scanStatus.x = m_activeRect->left;

            if (++m_scanStatus.y > m_activeRect->bottom)
            {
              // Overflow to next rectangle
              m_activeRect++;
              m_remainingRects--;
              ResetScan();
              break;
            }
          }
        }
        i2spiPushScreenData(&data, false);
      }
    }
  }

  void PrintFormatted(char *buffer)
  {
    // Build the result into the framebuffer
    u64 frame[COLS];
    memset(frame, 0, sizeof(frame));

    Draw::PrintSmall(frame, buffer, os_strlen(buffer), 0, 0);
    
    if (m_mode & inverted)
    {
      int y;
      for (y=0; y<COLS; ++y) frame[y] = ~frame[y];
    }

    Face::CreateRects((u64*) frame);
  }

  u32 DecToBCD(const u32 val)
  {
    u32 ret = 0;
    u32 num = val;
    for (int i=0; i < 8; ++i)
    {
      ret |= (num % 10) << (4*i);
      num /= 10;
    }
    return ret;
  }

  namespace Draw {
    void Clear(u64* frame) {
      os_memset(frame, 0, sizeof(u64) * COLS);
    }

    void Mask(u64* frame, const u64 mask) {
      for (int i = 0; i < COLS; i++) {
        *(frame++) &= mask;
      }
    }

    void Invert(u64* frame)
    {
      for (int i = 0; i < COLS; i++) {
        *frame = ~(*frame);
        frame++;
      } 
    }

    void Copy(u64* frame, const u64* image) {
      os_memcpy(frame, image, sizeof(u64) * COLS);
    }

    bool Copy(u64* frame, const u32* image, const int imgCols, const int x, const int y)
    {
      if ((imgCols + x) > COLS)
      {
        AnkiWarn( 197, "Face.Draw.Copy.TooWide", 520, "%d + %d > %d", 3, imgCols, x, COLS);
        return false;
      }
      
      for (int c=0; c<imgCols; ++c)
      {
        u64 line = image[c];
        frame[c+x] |= line << y;
      }
      
      return true;
    }

    bool Number(u64* frame, int digits, u32 value, int x, int y)
    {
      const int number_width = digits * 16;

      if (x + number_width > COLS)
      {
        AnkiWarn( 213, "Face.Draw.Number.TooWide", 520, "%d + %d > %d", 3, x, number_width, COLS);
        return false;
      }

      u64* pixels = &frame[x];
      memset(frame, 0, sizeof(frame));

      for (int c = 0; c < digits; c++) {
        int ch = (value >> ((digits - 1 - c) * 4)) & 0xF;
        for (int x = 0; x < 16; x++) {
          u64 line = BIG_DIGITS[ch][x];
          *(pixels++) |= line << y;
        }
      }
      
      return true;
    }
    
    bool NumberTiny(u64* frame, int digits, u32 value, int x, int y)
    {
      const int number_width = (digits * (TINY_DIGIT_WIDTH + 1)) - 1;
      
      if (x + number_width > COLS)
      {
        AnkiWarn( 376, "Face.Draw.NumberTiny.TooWide", 520, "%d + %d > %d", 3, x, number_width, COLS);
        return false;
      }
      
      u64* pixels = &frame[x];
      for (int c = 0; c < digits; c++) {
        const u8 ch = (value >> ((digits - 1 - c) * 4)) & 0xF;
        const u32 digitPixels = TINY_DIGITS[ch]; // Aligned fetch from flash
        const u8* dpxarr = (u8*)&digitPixels; // Recast as array
        for (u8 cx = 0; cx < TINY_DIGIT_WIDTH; cx++) {
          const u64 line = dpxarr[cx];
          *(pixels++) |= line << y;
        }
        pixels++; // Add white space between characters
      }
      
      return true;
    }
    
    bool Print(u64* frame, const char* text, const int characters, const int x, const int y)
    {
      const int text_len   = (characters == 0) ? os_strlen(text) : characters;
      const int text_width = text_len * MED_CHAR_WIDTH;
      
      if (x + text_width > COLS)
      {
        AnkiWarn( 214, "Face.Draw.Print.TooWide", 520, "%d + %d > %d", 3, x, text_width, COLS);
        return false;
      }
      
      int frameCol = x;
      
      for (int character = 0; character < text_len; ++character)
      {
        // Skip characters we can't represent
        if (( text[character] < '0')  ||
            ((text[character] > '9') && (text[character] < 'A')) ||
            ( text[character] > 'Z'))
        {
          frameCol += MED_CHAR_WIDTH;
          continue;
        }
        
        for (int col = 0; col < MED_CHAR_WIDTH; ++col)
        {
          u64 line;
          if (text[character] <= '9') line = MED_DIGITS[text[character] - '0'][col];
          else line = MED_CAPS[text[character] - 'A'][col];
          frame[frameCol] |= line << y;
          frameCol++;
        }
      }
      
      return true;
    }

    bool PrintSmall(u64* frame, const char* text, const int characters, int x, int y)
    {
      const char* cptr = text;
      int characterCount = 0;
      
      // Go character by character until we hit the end
      while (*cptr && (characterCount++ < characters))
      {
        // Wrap to next row, and bail out past bottom row
        int c = *cptr++;
        if (c == '\n' || (x + CHAR_WIDTH + 1) >= COLS)
        {
          x = 0;
          y += CHAR_HEIGHT;
        }
        if (y >= ROWS)
          break;

        // Skip unrecognized chars
        if (c < CHAR_START || c > CHAR_END)
          continue;

        // Copy the character from the font buffer to the display buffer
        const u32* fptr = FONT + (c - CHAR_START) * CHAR_WIDTH;
        u8* gptr = (u8*)(frame) + (y/CHAR_HEIGHT) + (x + 1) * (ROWS / CHAR_HEIGHT);

        for (int i = 0; i < CHAR_WIDTH; i++)
        {
          *gptr = *fptr++;
          gptr += ROWS / CHAR_HEIGHT;
        }
        x += CHAR_WIDTH + 1;
        if (c == '$'||c=='&') { x--;} //width adjust for key glyphs
      }
      
      return true;
    }

    bool PrintPsk(u64* frame, const char* text, int text_len, const int x, const int y)
    {
      text_len = MIN(text_len, DIGITDASH_WIFI_PSK_LEN);
      const int text_width = (text_len-1) * PSK_CHAR_WIDTH; //-1 b/c 2 half-width dashes
      if (x + text_width > COLS)
      {
        AnkiWarn( 214, "Face.Draw.Print.TooWide", 520, "%d + %d > %d", 3, x, text_width, COLS);
        return false;
      }
      
      int frameCol = x;
      uint8_t bits[PSK_BYTES_PER_DIGIT];  //we will copy font char into here to allow unaligned reads.
      
      for (int character = 0; character < text_len; ++character)
      {
        int digit = text[character];
        if (digit == '\0')
        {
          break;   //stop early
        }
        else if (digit == '-') 
        {
          os_memcpy(bits, &PSK_DIGITS[PSK_DASH_OFFSET], PSK_BYTES_PER_DIGIT);
        }
        else if ( (digit >= '0') || (digit <= '9') )
        {
          os_memcpy(bits, &PSK_DIGITS[(digit-'0')*PSK_BYTES_PER_DIGIT], PSK_BYTES_PER_DIGIT);
        }
        else
        {
          os_memset(bits, 0x55, PSK_BYTES_PER_DIGIT); //Show horiz bars icon for bad chars.
        }
        
        for (int byte = 0; byte < PSK_BYTES_PER_DIGIT; byte+=3)
        {
          if (digit=='-' && byte >= PSK_BYTES_PER_DIGIT/2)
          {
            break; //dashes are half-width
          }
          else  //unpack: 3 bytes -> 2 12-bit cols.
          {
            u64 line = bits[byte+0] | ( ((uint16_t)bits[byte+1]&0x0F) << 8 );
            frame[frameCol++] |= line << y;
            line = ( ((uint16_t)bits[byte+1] >> 4) & 0x0F ) | ((uint16_t)bits[byte+2] << 4);
            frame[frameCol++] |= line << y;
          }
        }
      }
      return true;
    }

    void Flip(u64* frame) {
      Face::CreateRects((u64*) frame);
    }
  }

  void Clear()
  {
    u64 frame[COLS];
    memset(frame, 0, sizeof(frame));
    CreateRects((u64*) frame);
  }

#define MAX_SCREEN_IDLE_TIME_US  (30*1000*1000)  //30 seconds

  static bool IsFrozen(void)
  {
    const u32 idleTime = system_get_time() - m_lastUpdateTime;
    return (idleTime > MAX_SCREEN_IDLE_TIME_US);
  }

  void Update(void)
  {
    if (IsFrozen())
    {
      Clear();
    }
    
    FillScreenData();
  }

  // Display text on the screen until turned off
  extern "C" void FacePrintf(const char *format, ...)
  {
    if ((m_mode & debug) == 0)
      {// Build the printf into a local buffer and zero-terminate it
      char buffer[256];
      va_list argptr;
      va_start(argptr, format);
      vsnprintf(buffer, sizeof(buffer)-1, format, argptr);
      va_end(argptr);
      buffer[sizeof(buffer)-1] = '\0';
      
      m_mode = text;

      PrintFormatted(buffer);
    }
  }
  
  extern "C" void FaceDebugPrintf(const char *format, ...)
  {
    // Build the printf into a local buffer and zero-terminate it
    char buffer[256];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf(buffer, sizeof(buffer)-1, format, argptr);
    va_end(argptr);
    buffer[sizeof(buffer)-1] = '\0';
    
    m_mode |= debug | inverted;

    PrintFormatted(buffer);
  }
  
  // Return display to normal function
  extern "C" void FaceUnPrintf(void)
  {
    m_mode = graphics;
    m_rectLock = true;
    ResetScreen();
    m_rectLock = false;
  }

  int GetRemainingRects()
  {
    return m_remainingRects;
  }

  s32 SuspendAndGetBuffer(void** buffer)
  {
    m_mode = suspended;
    // Must not clear buffer because we want to allow callers to use repeatedly so they don't have to maintain pointer
    *buffer = m_frame;
    return sizeof(m_frame);
  }
  
  void ResumeAndRestoreBuffer()
  {
    m_mode = graphics;
    ResetScreen();
  }


} // Face

namespace HAL {
  void FaceAnimate(u8* image, const u16 length)
  {
    using namespace Anki::Cozmo::Face;

    if (Face::m_mode != Face::graphics || Face::m_remainingRects > 0) return; // Ignore when in text mode

    else if (length == MAX_FACE_FRAME_SIZE) // If it's this size, it's raw
    {
      Face::CreateRects((u64*) image);
    }
    else
    {
      u64 frame[COLS];
      ISR_STACK_LEFT('F');
      FaceDisplayDecode(image, ROWS, COLS, frame);
      Face::CreateRects(frame);
    }
  }
}

} // Cozmo
} // Anki
