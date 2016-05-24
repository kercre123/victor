// Auto-generated code from testNumericCast for generating tables of expected casts for each type->type for a representative set of values
// See CodeGenTestNumericCastAll(), enabled via '#define ENABLE_NUMERIC_CAST_CODE_GEN 1'


void TestNumericCastFrom_int16_t()
{
  const int16_t fromValues[] = {
    -32768, -32767, -32766, -257, -256,
    -255, -254, -129, -128, -127,
    -126, -1, 0, 1, 126,
    127, 128, 129, 254, 255,
    256, 257, 32766, 32767 };
  
  // to: uint8_t
  {
    uint8_t expectedValues_uint8_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 1, 126,
      127, 128, 129, 254, 255,
      255, 255, 255, 255    };
    
    bool expectedValid_uint8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, true, true, true,
      true, true, true, true, true,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint8_t, expectedValid_uint8_t, Anki::Util::array_size(fromValues));
  }
  // to: uint16_t
  {
    uint16_t expectedValues_uint16_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 1, 126,
      127, 128, 129, 254, 255,
      256, 257, 32766, 32767    };
    
    bool expectedValid_uint16_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint16_t, expectedValid_uint16_t, Anki::Util::array_size(fromValues));
  }
  // to: uint32_t
  {
    uint32_t expectedValues_uint32_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 1, 126,
      127, 128, 129, 254, 255,
      256, 257, 32766, 32767    };
    
    bool expectedValid_uint32_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint32_t, expectedValid_uint32_t, Anki::Util::array_size(fromValues));
  }
  // to: uint64_t
  {
    uint64_t expectedValues_uint64_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 1, 126,
      127, 128, 129, 254, 255,
      256, 257, 32766, 32767    };
    
    bool expectedValid_uint64_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint64_t, expectedValid_uint64_t, Anki::Util::array_size(fromValues));
  }
  // to: int8_t
  {
    int8_t expectedValues_int8_t[] = {
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -127,
      -126, -1, 0, 1, 126,
      127, 127, 127, 127, 127,
      127, 127, 127, 127    };
    
    bool expectedValid_int8_t[] = {
      false, false, false, false, false,
      false, false, false, true, true,
      true, true, true, true, true,
      true, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int8_t, expectedValid_int8_t, Anki::Util::array_size(fromValues));
  }
  // to: int16_t
  {
    int16_t expectedValues_int16_t[] = {
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, 0, 1, 126,
      127, 128, 129, 254, 255,
      256, 257, 32766, 32767    };
    
    bool expectedValid_int16_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int16_t, expectedValid_int16_t, Anki::Util::array_size(fromValues));
  }
  // to: int32_t
  {
    int32_t expectedValues_int32_t[] = {
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, 0, 1, 126,
      127, 128, 129, 254, 255,
      256, 257, 32766, 32767    };
    
    bool expectedValid_int32_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int32_t, expectedValid_int32_t, Anki::Util::array_size(fromValues));
  }
  // to: int64_t
  {
    int64_t expectedValues_int64_t[] = {
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, 0, 1, 126,
      127, 128, 129, 254, 255,
      256, 257, 32766, 32767    };
    
    bool expectedValid_int64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int64_t, expectedValid_int64_t, Anki::Util::array_size(fromValues));
  }
  // to: float
  {
    float expectedValues_float[] = {
      -32768.000000f, -32767.000000f, -32766.000000f, -257.000000f, -256.000000f,
      -255.000000f, -254.000000f, -129.000000f, -128.000000f, -127.000000f,
      -126.000000f, -1.000000f, 0.000000f, 1.000000f, 126.000000f,
      127.000000f, 128.000000f, 129.000000f, 254.000000f, 255.000000f,
      256.000000f, 257.000000f, 32766.000000f, 32767.000000f    };
    
    bool expectedValid_float[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_float), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_float),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_float, expectedValid_float, Anki::Util::array_size(fromValues));
  }
  // to: double
  {
    double expectedValues_double[] = {
      -32768.000000, -32767.000000, -32766.000000, -257.000000, -256.000000,
      -255.000000, -254.000000, -129.000000, -128.000000, -127.000000,
      -126.000000, -1.000000, 0.000000, 1.000000, 126.000000,
      127.000000, 128.000000, 129.000000, 254.000000, 255.000000,
      256.000000, 257.000000, 32766.000000, 32767.000000    };
    
    bool expectedValid_double[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_double), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_double),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_double, expectedValid_double, Anki::Util::array_size(fromValues));
  }
}


void TestNumericCastFrom_int32_t()
{
  const int32_t fromValues[] = {
    -2147483648, -2147483647, -2147483646, -65537, -65536,
    -65535, -65534, -32769, -32768, -32767,
    -32766, -257, -256, -255, -254,
    -129, -128, -127, -126, -1,
    0, 1, 126, 127, 128,
    129, 254, 255, 256, 257,
    32766, 32767, 32768, 32769, 65534,
    65535, 65536, 65537, 2147483646, 2147483647 };
  
  // to: uint8_t
  {
    uint8_t expectedValues_uint8_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 126, 127, 128,
      129, 254, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255    };
    
    bool expectedValid_uint8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, false, false,
      false, false, false, false, false,
      false, false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint8_t, expectedValid_uint8_t, Anki::Util::array_size(fromValues));
  }
  // to: uint16_t
  {
    uint16_t expectedValues_uint16_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65535, 65535, 65535, 65535    };
    
    bool expectedValid_uint16_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint16_t, expectedValid_uint16_t, Anki::Util::array_size(fromValues));
  }
  // to: uint32_t
  {
    uint32_t expectedValues_uint32_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647    };
    
    bool expectedValid_uint32_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint32_t, expectedValid_uint32_t, Anki::Util::array_size(fromValues));
  }
  // to: uint64_t
  {
    uint64_t expectedValues_uint64_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647    };
    
    bool expectedValid_uint64_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint64_t, expectedValid_uint64_t, Anki::Util::array_size(fromValues));
  }
  // to: int8_t
  {
    int8_t expectedValues_int8_t[] = {
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -127, -126, -1,
      0, 1, 126, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127    };
    
    bool expectedValid_int8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int8_t, expectedValid_int8_t, Anki::Util::array_size(fromValues));
  }
  // to: int16_t
  {
    int16_t expectedValues_int16_t[] = {
      -32768, -32768, -32768, -32768, -32768,
      -32768, -32768, -32768, -32768, -32767,
      -32766, -257, -256, -255, -254,
      -129, -128, -127, -126, -1,
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767    };
    
    bool expectedValid_int16_t[] = {
      false, false, false, false, false,
      false, false, false, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, false, false, false,
      false, false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int16_t, expectedValid_int16_t, Anki::Util::array_size(fromValues));
  }
  // to: int32_t
  {
    int32_t expectedValues_int32_t[] = {
      -2147483648, -2147483647, -2147483646, -65537, -65536,
      -65535, -65534, -32769, -32768, -32767,
      -32766, -257, -256, -255, -254,
      -129, -128, -127, -126, -1,
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647    };
    
    bool expectedValid_int32_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int32_t, expectedValid_int32_t, Anki::Util::array_size(fromValues));
  }
  // to: int64_t
  {
    int64_t expectedValues_int64_t[] = {
      -2147483648, -2147483647, -2147483646, -65537, -65536,
      -65535, -65534, -32769, -32768, -32767,
      -32766, -257, -256, -255, -254,
      -129, -128, -127, -126, -1,
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647    };
    
    bool expectedValid_int64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int64_t, expectedValid_int64_t, Anki::Util::array_size(fromValues));
  }
  // to: float
  {
    float expectedValues_float[] = {
      -2147483648.000000f, -2147483648.000000f, -2147483648.000000f, -65537.000000f, -65536.000000f,
      -65535.000000f, -65534.000000f, -32769.000000f, -32768.000000f, -32767.000000f,
      -32766.000000f, -257.000000f, -256.000000f, -255.000000f, -254.000000f,
      -129.000000f, -128.000000f, -127.000000f, -126.000000f, -1.000000f,
      0.000000f, 1.000000f, 126.000000f, 127.000000f, 128.000000f,
      129.000000f, 254.000000f, 255.000000f, 256.000000f, 257.000000f,
      32766.000000f, 32767.000000f, 32768.000000f, 32769.000000f, 65534.000000f,
      65535.000000f, 65536.000000f, 65537.000000f, 2147483648.000000f, 2147483648.000000f    };
    
    bool expectedValid_float[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_float), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_float),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_float, expectedValid_float, Anki::Util::array_size(fromValues));
  }
  // to: double
  {
    double expectedValues_double[] = {
      -2147483648.000000, -2147483647.000000, -2147483646.000000, -65537.000000, -65536.000000,
      -65535.000000, -65534.000000, -32769.000000, -32768.000000, -32767.000000,
      -32766.000000, -257.000000, -256.000000, -255.000000, -254.000000,
      -129.000000, -128.000000, -127.000000, -126.000000, -1.000000,
      0.000000, 1.000000, 126.000000, 127.000000, 128.000000,
      129.000000, 254.000000, 255.000000, 256.000000, 257.000000,
      32766.000000, 32767.000000, 32768.000000, 32769.000000, 65534.000000,
      65535.000000, 65536.000000, 65537.000000, 2147483646.000000, 2147483647.000000    };
    
    bool expectedValid_double[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_double), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_double),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_double, expectedValid_double, Anki::Util::array_size(fromValues));
  }
}


void TestNumericCastFrom_int64_t()
{
  const int64_t fromValues[] = {
    kInt64Lowest, -9223372036854775807, -9223372036854775806, -2147483649, -2147483648,
    -2147483647, -2147483646, -65537, -65536, -65535,
    -65534, -32769, -32768, -32767, -32766,
    -257, -256, -255, -254, -129,
    -128, -127, -126, -1, 0,
    1, 126, 127, 128, 129,
    254, 255, 256, 257, 32766,
    32767, 32768, 32769, 65534, 65535,
    65536, 65537, 2147483646, 2147483647, 2147483648,
    2147483649, 4294967294, 4294967295, 4294967296, 4294967297,
    9223372036854775806, 9223372036854775807 };
  
  // to: uint8_t
  {
    uint8_t expectedValues_uint8_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 126, 127, 128, 129,
      254, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255    };
    
    bool expectedValid_uint8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint8_t, expectedValid_uint8_t, Anki::Util::array_size(fromValues));
  }
  // to: uint16_t
  {
    uint16_t expectedValues_uint16_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 126, 127, 128, 129,
      254, 255, 256, 257, 32766,
      32767, 32768, 32769, 65534, 65535,
      65535, 65535, 65535, 65535, 65535,
      65535, 65535, 65535, 65535, 65535,
      65535, 65535    };
    
    bool expectedValid_uint16_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint16_t, expectedValid_uint16_t, Anki::Util::array_size(fromValues));
  }
  // to: uint32_t
  {
    uint32_t expectedValues_uint32_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 126, 127, 128, 129,
      254, 255, 256, 257, 32766,
      32767, 32768, 32769, 65534, 65535,
      65536, 65537, 2147483646, 2147483647, 2147483648,
      2147483649, 4294967294, 4294967295, 4294967295, 4294967295,
      4294967295, 4294967295    };
    
    bool expectedValid_uint32_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint32_t, expectedValid_uint32_t, Anki::Util::array_size(fromValues));
  }
  // to: uint64_t
  {
    uint64_t expectedValues_uint64_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      1, 126, 127, 128, 129,
      254, 255, 256, 257, 32766,
      32767, 32768, 32769, 65534, 65535,
      65536, 65537, 2147483646, 2147483647, 2147483648,
      2147483649, 4294967294, 4294967295, 4294967296, 4294967297,
      9223372036854775806, 9223372036854775807    };
    
    bool expectedValid_uint64_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint64_t, expectedValid_uint64_t, Anki::Util::array_size(fromValues));
  }
  // to: int8_t
  {
    int8_t expectedValues_int8_t[] = {
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -127, -126, -1, 0,
      1, 126, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127    };
    
    bool expectedValid_int8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int8_t, expectedValid_int8_t, Anki::Util::array_size(fromValues));
  }
  // to: int16_t
  {
    int16_t expectedValues_int16_t[] = {
      -32768, -32768, -32768, -32768, -32768,
      -32768, -32768, -32768, -32768, -32768,
      -32768, -32768, -32768, -32767, -32766,
      -257, -256, -255, -254, -129,
      -128, -127, -126, -1, 0,
      1, 126, 127, 128, 129,
      254, 255, 256, 257, 32766,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767    };
    
    bool expectedValid_int16_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int16_t, expectedValid_int16_t, Anki::Util::array_size(fromValues));
  }
  // to: int32_t
  {
    int32_t expectedValues_int32_t[] = {
      -2147483648, -2147483648, -2147483648, -2147483648, -2147483648,
      -2147483647, -2147483646, -65537, -65536, -65535,
      -65534, -32769, -32768, -32767, -32766,
      -257, -256, -255, -254, -129,
      -128, -127, -126, -1, 0,
      1, 126, 127, 128, 129,
      254, 255, 256, 257, 32766,
      32767, 32768, 32769, 65534, 65535,
      65536, 65537, 2147483646, 2147483647, 2147483647,
      2147483647, 2147483647, 2147483647, 2147483647, 2147483647,
      2147483647, 2147483647    };
    
    bool expectedValid_int32_t[] = {
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int32_t, expectedValid_int32_t, Anki::Util::array_size(fromValues));
  }
  // to: int64_t
  {
    int64_t expectedValues_int64_t[] = {
      kInt64Lowest, -9223372036854775807, -9223372036854775806, -2147483649, -2147483648,
      -2147483647, -2147483646, -65537, -65536, -65535,
      -65534, -32769, -32768, -32767, -32766,
      -257, -256, -255, -254, -129,
      -128, -127, -126, -1, 0,
      1, 126, 127, 128, 129,
      254, 255, 256, 257, 32766,
      32767, 32768, 32769, 65534, 65535,
      65536, 65537, 2147483646, 2147483647, 2147483648,
      2147483649, 4294967294, 4294967295, 4294967296, 4294967297,
      9223372036854775806, 9223372036854775807    };
    
    bool expectedValid_int64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int64_t, expectedValid_int64_t, Anki::Util::array_size(fromValues));
  }
  // to: float
  {
    float expectedValues_float[] = {
      -9223372036854775808.000000f, -9223372036854775808.000000f, -9223372036854775808.000000f, -2147483648.000000f, -2147483648.000000f,
      -2147483648.000000f, -2147483648.000000f, -65537.000000f, -65536.000000f, -65535.000000f,
      -65534.000000f, -32769.000000f, -32768.000000f, -32767.000000f, -32766.000000f,
      -257.000000f, -256.000000f, -255.000000f, -254.000000f, -129.000000f,
      -128.000000f, -127.000000f, -126.000000f, -1.000000f, 0.000000f,
      1.000000f, 126.000000f, 127.000000f, 128.000000f, 129.000000f,
      254.000000f, 255.000000f, 256.000000f, 257.000000f, 32766.000000f,
      32767.000000f, 32768.000000f, 32769.000000f, 65534.000000f, 65535.000000f,
      65536.000000f, 65537.000000f, 2147483648.000000f, 2147483648.000000f, 2147483648.000000f,
      2147483648.000000f, 4294967296.000000f, 4294967296.000000f, 4294967296.000000f, 4294967296.000000f,
      9223372036854775808.000000f, 9223372036854775808.000000f    };
    
    bool expectedValid_float[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_float), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_float),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_float, expectedValid_float, Anki::Util::array_size(fromValues));
  }
  // to: double
  {
    double expectedValues_double[] = {
      -9223372036854775808.000000, -9223372036854775808.000000, -9223372036854775808.000000, -2147483649.000000, -2147483648.000000,
      -2147483647.000000, -2147483646.000000, -65537.000000, -65536.000000, -65535.000000,
      -65534.000000, -32769.000000, -32768.000000, -32767.000000, -32766.000000,
      -257.000000, -256.000000, -255.000000, -254.000000, -129.000000,
      -128.000000, -127.000000, -126.000000, -1.000000, 0.000000,
      1.000000, 126.000000, 127.000000, 128.000000, 129.000000,
      254.000000, 255.000000, 256.000000, 257.000000, 32766.000000,
      32767.000000, 32768.000000, 32769.000000, 65534.000000, 65535.000000,
      65536.000000, 65537.000000, 2147483646.000000, 2147483647.000000, 2147483648.000000,
      2147483649.000000, 4294967294.000000, 4294967295.000000, 4294967296.000000, 4294967297.000000,
      9223372036854775808.000000, 9223372036854775808.000000    };
    
    bool expectedValid_double[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_double), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_double),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_double, expectedValid_double, Anki::Util::array_size(fromValues));
  }
}


void TestNumericCastFrom_uint16_t()
{
  const uint16_t fromValues[] = {
    0, 1, 126, 127, 128,
    129, 254, 255, 256, 257,
    32766, 32767, 32768, 32769, 65534,
    65535 };
  
  // to: uint8_t
  {
    uint8_t expectedValues_uint8_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 255, 255,
      255, 255, 255, 255, 255,
      255    };
    
    bool expectedValid_uint8_t[] = {
      true, true, true, true, true,
      true, true, true, false, false,
      false, false, false, false, false,
      false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint8_t, expectedValid_uint8_t, Anki::Util::array_size(fromValues));
  }
  // to: uint16_t
  {
    uint16_t expectedValues_uint16_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535    };
    
    bool expectedValid_uint16_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint16_t, expectedValid_uint16_t, Anki::Util::array_size(fromValues));
  }
  // to: uint32_t
  {
    uint32_t expectedValues_uint32_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535    };
    
    bool expectedValid_uint32_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint32_t, expectedValid_uint32_t, Anki::Util::array_size(fromValues));
  }
  // to: uint64_t
  {
    uint64_t expectedValues_uint64_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535    };
    
    bool expectedValid_uint64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint64_t, expectedValid_uint64_t, Anki::Util::array_size(fromValues));
  }
  // to: int8_t
  {
    int8_t expectedValues_int8_t[] = {
      0, 1, 126, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127    };
    
    bool expectedValid_int8_t[] = {
      true, true, true, true, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int8_t, expectedValid_int8_t, Anki::Util::array_size(fromValues));
  }
  // to: int16_t
  {
    int16_t expectedValues_int16_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32767, 32767, 32767,
      32767    };
    
    bool expectedValid_int16_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, false, false, false,
      false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int16_t, expectedValid_int16_t, Anki::Util::array_size(fromValues));
  }
  // to: int32_t
  {
    int32_t expectedValues_int32_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535    };
    
    bool expectedValid_int32_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int32_t, expectedValid_int32_t, Anki::Util::array_size(fromValues));
  }
  // to: int64_t
  {
    int64_t expectedValues_int64_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535    };
    
    bool expectedValid_int64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int64_t, expectedValid_int64_t, Anki::Util::array_size(fromValues));
  }
  // to: float
  {
    float expectedValues_float[] = {
      0.000000f, 1.000000f, 126.000000f, 127.000000f, 128.000000f,
      129.000000f, 254.000000f, 255.000000f, 256.000000f, 257.000000f,
      32766.000000f, 32767.000000f, 32768.000000f, 32769.000000f, 65534.000000f,
      65535.000000f    };
    
    bool expectedValid_float[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_float), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_float),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_float, expectedValid_float, Anki::Util::array_size(fromValues));
  }
  // to: double
  {
    double expectedValues_double[] = {
      0.000000, 1.000000, 126.000000, 127.000000, 128.000000,
      129.000000, 254.000000, 255.000000, 256.000000, 257.000000,
      32766.000000, 32767.000000, 32768.000000, 32769.000000, 65534.000000,
      65535.000000    };
    
    bool expectedValid_double[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_double), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_double),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_double, expectedValid_double, Anki::Util::array_size(fromValues));
  }
}


void TestNumericCastFrom_uint32_t()
{
  const uint32_t fromValues[] = {
    0, 1, 126, 127, 128,
    129, 254, 255, 256, 257,
    32766, 32767, 32768, 32769, 65534,
    65535, 65536, 65537, 2147483646, 2147483647,
    2147483648, 2147483649, 4294967294, 4294967295 };
  
  // to: uint8_t
  {
    uint8_t expectedValues_uint8_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255    };
    
    bool expectedValid_uint8_t[] = {
      true, true, true, true, true,
      true, true, true, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint8_t, expectedValid_uint8_t, Anki::Util::array_size(fromValues));
  }
  // to: uint16_t
  {
    uint16_t expectedValues_uint16_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65535, 65535, 65535, 65535,
      65535, 65535, 65535, 65535    };
    
    bool expectedValid_uint16_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint16_t, expectedValid_uint16_t, Anki::Util::array_size(fromValues));
  }
  // to: uint32_t
  {
    uint32_t expectedValues_uint32_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647,
      2147483648, 2147483649, 4294967294, 4294967295    };
    
    bool expectedValid_uint32_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint32_t, expectedValid_uint32_t, Anki::Util::array_size(fromValues));
  }
  // to: uint64_t
  {
    uint64_t expectedValues_uint64_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647,
      2147483648, 2147483649, 4294967294, 4294967295    };
    
    bool expectedValid_uint64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint64_t, expectedValid_uint64_t, Anki::Util::array_size(fromValues));
  }
  // to: int8_t
  {
    int8_t expectedValues_int8_t[] = {
      0, 1, 126, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127    };
    
    bool expectedValid_int8_t[] = {
      true, true, true, true, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int8_t, expectedValid_int8_t, Anki::Util::array_size(fromValues));
  }
  // to: int16_t
  {
    int16_t expectedValues_int16_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767    };
    
    bool expectedValid_int16_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int16_t, expectedValid_int16_t, Anki::Util::array_size(fromValues));
  }
  // to: int32_t
  {
    int32_t expectedValues_int32_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647,
      2147483647, 2147483647, 2147483647, 2147483647    };
    
    bool expectedValid_int32_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int32_t, expectedValid_int32_t, Anki::Util::array_size(fromValues));
  }
  // to: int64_t
  {
    int64_t expectedValues_int64_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647,
      2147483648, 2147483649, 4294967294, 4294967295    };
    
    bool expectedValid_int64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int64_t, expectedValid_int64_t, Anki::Util::array_size(fromValues));
  }
  // to: float
  {
    float expectedValues_float[] = {
      0.000000f, 1.000000f, 126.000000f, 127.000000f, 128.000000f,
      129.000000f, 254.000000f, 255.000000f, 256.000000f, 257.000000f,
      32766.000000f, 32767.000000f, 32768.000000f, 32769.000000f, 65534.000000f,
      65535.000000f, 65536.000000f, 65537.000000f, 2147483648.000000f, 2147483648.000000f,
      2147483648.000000f, 2147483648.000000f, 4294967296.000000f, 4294967296.000000f    };
    
    bool expectedValid_float[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_float), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_float),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_float, expectedValid_float, Anki::Util::array_size(fromValues));
  }
  // to: double
  {
    double expectedValues_double[] = {
      0.000000, 1.000000, 126.000000, 127.000000, 128.000000,
      129.000000, 254.000000, 255.000000, 256.000000, 257.000000,
      32766.000000, 32767.000000, 32768.000000, 32769.000000, 65534.000000,
      65535.000000, 65536.000000, 65537.000000, 2147483646.000000, 2147483647.000000,
      2147483648.000000, 2147483649.000000, 4294967294.000000, 4294967295.000000    };
    
    bool expectedValid_double[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_double), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_double),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_double, expectedValid_double, Anki::Util::array_size(fromValues));
  }
}


void TestNumericCastFrom_uint64_t()
{
  const uint64_t fromValues[] = {
    0, 1, 126, 127, 128,
    129, 254, 255, 256, 257,
    32766, 32767, 32768, 32769, 65534,
    65535, 65536, 65537, 2147483646, 2147483647,
    2147483648, 2147483649, 4294967294, 4294967295, 4294967296,
    4294967297, 9223372036854775806, 9223372036854775807, 9223372036854775808ull, 9223372036854775809ull,
    18446744073709551614ull, 18446744073709551615ull };
  
  // to: uint8_t
  {
    uint8_t expectedValues_uint8_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255    };
    
    bool expectedValid_uint8_t[] = {
      true, true, true, true, true,
      true, true, true, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint8_t, expectedValid_uint8_t, Anki::Util::array_size(fromValues));
  }
  // to: uint16_t
  {
    uint16_t expectedValues_uint16_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65535, 65535, 65535, 65535,
      65535, 65535, 65535, 65535, 65535,
      65535, 65535, 65535, 65535, 65535,
      65535, 65535    };
    
    bool expectedValid_uint16_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint16_t, expectedValid_uint16_t, Anki::Util::array_size(fromValues));
  }
  // to: uint32_t
  {
    uint32_t expectedValues_uint32_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647,
      2147483648, 2147483649, 4294967294, 4294967295, 4294967295,
      4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
      4294967295, 4294967295    };
    
    bool expectedValid_uint32_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint32_t, expectedValid_uint32_t, Anki::Util::array_size(fromValues));
  }
  // to: uint64_t
  {
    uint64_t expectedValues_uint64_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647,
      2147483648, 2147483649, 4294967294, 4294967295, 4294967296,
      4294967297, 9223372036854775806, 9223372036854775807, 9223372036854775808ull, 9223372036854775809ull,
      18446744073709551614ull, 18446744073709551615ull    };
    
    bool expectedValid_uint64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint64_t, expectedValid_uint64_t, Anki::Util::array_size(fromValues));
  }
  // to: int8_t
  {
    int8_t expectedValues_int8_t[] = {
      0, 1, 126, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127    };
    
    bool expectedValid_int8_t[] = {
      true, true, true, true, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int8_t, expectedValid_int8_t, Anki::Util::array_size(fromValues));
  }
  // to: int16_t
  {
    int16_t expectedValues_int16_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767    };
    
    bool expectedValid_int16_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int16_t, expectedValid_int16_t, Anki::Util::array_size(fromValues));
  }
  // to: int32_t
  {
    int32_t expectedValues_int32_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647,
      2147483647, 2147483647, 2147483647, 2147483647, 2147483647,
      2147483647, 2147483647, 2147483647, 2147483647, 2147483647,
      2147483647, 2147483647    };
    
    bool expectedValid_int32_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int32_t, expectedValid_int32_t, Anki::Util::array_size(fromValues));
  }
  // to: int64_t
  {
    int64_t expectedValues_int64_t[] = {
      0, 1, 126, 127, 128,
      129, 254, 255, 256, 257,
      32766, 32767, 32768, 32769, 65534,
      65535, 65536, 65537, 2147483646, 2147483647,
      2147483648, 2147483649, 4294967294, 4294967295, 4294967296,
      4294967297, 9223372036854775806, 9223372036854775807, 9223372036854775807, 9223372036854775807,
      9223372036854775807, 9223372036854775807    };
    
    bool expectedValid_int64_t[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, false, false,
      false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int64_t, expectedValid_int64_t, Anki::Util::array_size(fromValues));
  }
  // to: float
  {
    float expectedValues_float[] = {
      0.000000f, 1.000000f, 126.000000f, 127.000000f, 128.000000f,
      129.000000f, 254.000000f, 255.000000f, 256.000000f, 257.000000f,
      32766.000000f, 32767.000000f, 32768.000000f, 32769.000000f, 65534.000000f,
      65535.000000f, 65536.000000f, 65537.000000f, 2147483648.000000f, 2147483648.000000f,
      2147483648.000000f, 2147483648.000000f, 4294967296.000000f, 4294967296.000000f, 4294967296.000000f,
      4294967296.000000f, 9223372036854775808.000000f, 9223372036854775808.000000f, 9223372036854775808.000000f, 9223372036854775808.000000f,
      1.844674e+19, 1.844674e+19    };
    
    bool expectedValid_float[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_float), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_float),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_float, expectedValid_float, Anki::Util::array_size(fromValues));
  }
  // to: double
  {
    double expectedValues_double[] = {
      0.000000, 1.000000, 126.000000, 127.000000, 128.000000,
      129.000000, 254.000000, 255.000000, 256.000000, 257.000000,
      32766.000000, 32767.000000, 32768.000000, 32769.000000, 65534.000000,
      65535.000000, 65536.000000, 65537.000000, 2147483646.000000, 2147483647.000000,
      2147483648.000000, 2147483649.000000, 4294967294.000000, 4294967295.000000, 4294967296.000000,
      4294967297.000000, 9223372036854775808.000000, 9223372036854775808.000000, 9223372036854775808.000000, 9223372036854775808.000000,
      1.844674e+19, 1.844674e+19    };
    
    bool expectedValid_double[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_double), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_double),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_double, expectedValid_double, Anki::Util::array_size(fromValues));
  }
}


void TestNumericCastFrom_float()
{
  const float fromValues[] = {
    kFloatLowest, -1.000000e+38, -1.000000e+29, -1.000000e+20, -99999997952.000000f,
    -65537.000000f, -65536.000000f, -65535.000000f, -65534.000000f, -32769.000000f,
    -32768.000000f, -32767.000000f, -32766.000000f, -257.000000f, -256.000000f,
    -255.000000f, -254.000000f, -129.000000f, -128.000000f, -127.000000f,
    -126.000000f, -1.500000f, -1.000000f, -0.500000f, 0.000000f,
    0.500000f, 1.000000f, 1.500000f, 126.000000f, 127.000000f,
    128.000000f, 129.000000f, 254.000000f, 255.000000f, 256.000000f,
    257.000000f, 32766.000000f, 32767.000000f, 32768.000000f, 32769.000000f,
    65534.000000f, 65535.000000f, 65536.000000f, 65537.000000f, 99999997952.000000f,
    1.000000e+20, 1.000000e+29, 1.000000e+38, kFloatMax };
  
  // to: uint8_t
  {
    uint8_t expectedValues_uint8_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255    };
    
    bool expectedValid_uint8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint8_t, expectedValid_uint8_t, Anki::Util::array_size(fromValues));
  }
  // to: uint16_t
  {
    uint16_t expectedValues_uint16_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65535, 65535, 65535,
      65535, 65535, 65535, 65535    };
    
    bool expectedValid_uint16_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint16_t, expectedValid_uint16_t, Anki::Util::array_size(fromValues));
  }
  // to: uint32_t
  {
    uint32_t expectedValues_uint32_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65536, 65537, 4294967295,
      4294967295, 4294967295, 4294967295, 4294967295    };
    
    bool expectedValid_uint32_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint32_t, expectedValid_uint32_t, Anki::Util::array_size(fromValues));
  }
  // to: uint64_t
  {
    uint64_t expectedValues_uint64_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65536, 65537, 99999997952,
      18446744073709551615ull, 18446744073709551615ull, 18446744073709551615ull, 18446744073709551615ull    };
    
    bool expectedValid_uint64_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint64_t, expectedValid_uint64_t, Anki::Util::array_size(fromValues));
  }
  // to: int8_t
  {
    int8_t expectedValues_int8_t[] = {
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -127,
      -126, -1, -1, 0, 0,
      0, 1, 1, 126, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127    };
    
    bool expectedValid_int8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int8_t, expectedValid_int8_t, Anki::Util::array_size(fromValues));
  }
  // to: int16_t
  {
    int16_t expectedValues_int16_t[] = {
      -32768, -32768, -32768, -32768, -32768,
      -32768, -32768, -32768, -32768, -32768,
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, -1, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767    };
    
    bool expectedValid_int16_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int16_t, expectedValid_int16_t, Anki::Util::array_size(fromValues));
  }
  // to: int32_t
  {
    int32_t expectedValues_int32_t[] = {
      -2147483648, -2147483648, -2147483648, -2147483648, -2147483648,
      -65537, -65536, -65535, -65534, -32769,
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, -1, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65536, 65537, 2147483647,
      2147483647, 2147483647, 2147483647, 2147483647    };
    
    bool expectedValid_int32_t[] = {
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int32_t, expectedValid_int32_t, Anki::Util::array_size(fromValues));
  }
  // to: int64_t
  {
    int64_t expectedValues_int64_t[] = {
      kInt64Lowest, kInt64Lowest, kInt64Lowest, kInt64Lowest, -99999997952,
      -65537, -65536, -65535, -65534, -32769,
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, -1, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65536, 65537, 99999997952,
      9223372036854775807, 9223372036854775807, 9223372036854775807, 9223372036854775807    };
    
    bool expectedValid_int64_t[] = {
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int64_t, expectedValid_int64_t, Anki::Util::array_size(fromValues));
  }
  // to: float
  {
    float expectedValues_float[] = {
      kFloatLowest, -1.000000e+38, -1.000000e+29, -1.000000e+20, -99999997952.000000f,
      -65537.000000f, -65536.000000f, -65535.000000f, -65534.000000f, -32769.000000f,
      -32768.000000f, -32767.000000f, -32766.000000f, -257.000000f, -256.000000f,
      -255.000000f, -254.000000f, -129.000000f, -128.000000f, -127.000000f,
      -126.000000f, -1.500000f, -1.000000f, -0.500000f, 0.000000f,
      0.500000f, 1.000000f, 1.500000f, 126.000000f, 127.000000f,
      128.000000f, 129.000000f, 254.000000f, 255.000000f, 256.000000f,
      257.000000f, 32766.000000f, 32767.000000f, 32768.000000f, 32769.000000f,
      65534.000000f, 65535.000000f, 65536.000000f, 65537.000000f, 99999997952.000000f,
      1.000000e+20, 1.000000e+29, 1.000000e+38, kFloatMax    };
    
    bool expectedValid_float[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_float), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_float),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_float, expectedValid_float, Anki::Util::array_size(fromValues));
  }
  // to: double
  {
    double expectedValues_double[] = {
      -3.402823e+38, -1.000000e+38, -1.000000e+29, -1.000000e+20, -99999997952.000000,
      -65537.000000, -65536.000000, -65535.000000, -65534.000000, -32769.000000,
      -32768.000000, -32767.000000, -32766.000000, -257.000000, -256.000000,
      -255.000000, -254.000000, -129.000000, -128.000000, -127.000000,
      -126.000000, -1.500000, -1.000000, -0.500000, 0.000000,
      0.500000, 1.000000, 1.500000, 126.000000, 127.000000,
      128.000000, 129.000000, 254.000000, 255.000000, 256.000000,
      257.000000, 32766.000000, 32767.000000, 32768.000000, 32769.000000,
      65534.000000, 65535.000000, 65536.000000, 65537.000000, 99999997952.000000,
      1.000000e+20, 1.000000e+29, 1.000000e+38, 3.402823e+38    };
    
    bool expectedValid_double[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_double), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_double),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_double, expectedValid_double, Anki::Util::array_size(fromValues));
  }
}


void TestNumericCastFrom_double()
{
  const double fromValues[] = {
    kDoubleLowest, -1.000000e+300, -1.000000e+200, -1.000000e+100, -1.000000e+40,
    -1.000000e+39, -1.000000e+38, -1.000000e+29, -1.000000e+20, -100000000000.000000,
    -65537.000000, -65536.000000, -65535.000000, -65534.000000, -32769.000000,
    -32768.000000, -32767.000000, -32766.000000, -257.000000, -256.000000,
    -255.000000, -254.000000, -129.000000, -128.000000, -127.000000,
    -126.000000, -1.500000, -1.000000, -0.500000, 0.000000,
    0.500000, 1.000000, 1.500000, 126.000000, 127.000000,
    128.000000, 129.000000, 254.000000, 255.000000, 256.000000,
    257.000000, 32766.000000, 32767.000000, 32768.000000, 32769.000000,
    65534.000000, 65535.000000, 65536.000000, 65537.000000, 100000000000.000000,
    1.000000e+20, 1.000000e+29, 1.000000e+38, 1.000000e+39, 1.000000e+40,
    1.000000e+100, 1.000000e+200, 1.000000e+300, kDoubleMax };
  
  // to: uint8_t
  {
    uint8_t expectedValues_uint8_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255, 255,
      255, 255, 255, 255    };
    
    bool expectedValid_uint8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint8_t, expectedValid_uint8_t, Anki::Util::array_size(fromValues));
  }
  // to: uint16_t
  {
    uint16_t expectedValues_uint16_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65535, 65535, 65535,
      65535, 65535, 65535, 65535, 65535,
      65535, 65535, 65535, 65535    };
    
    bool expectedValid_uint16_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint16_t, expectedValid_uint16_t, Anki::Util::array_size(fromValues));
  }
  // to: uint32_t
  {
    uint32_t expectedValues_uint32_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65536, 65537, 4294967295,
      4294967295, 4294967295, 4294967295, 4294967295, 4294967295,
      4294967295, 4294967295, 4294967295, 4294967295    };
    
    bool expectedValid_uint32_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint32_t, expectedValid_uint32_t, Anki::Util::array_size(fromValues));
  }
  // to: uint64_t
  {
    uint64_t expectedValues_uint64_t[] = {
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 0, 0, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65536, 65537, 100000000000,
      18446744073709551615ull, 18446744073709551615ull, 18446744073709551615ull, 18446744073709551615ull, 18446744073709551615ull,
      18446744073709551615ull, 18446744073709551615ull, 18446744073709551615ull, 18446744073709551615ull    };
    
    bool expectedValid_uint64_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_uint64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_uint64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_uint64_t, expectedValid_uint64_t, Anki::Util::array_size(fromValues));
  }
  // to: int8_t
  {
    int8_t expectedValues_int8_t[] = {
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -128,
      -128, -128, -128, -128, -127,
      -126, -1, -1, 0, 0,
      0, 1, 1, 126, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127, 127,
      127, 127, 127, 127    };
    
    bool expectedValid_int8_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int8_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int8_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int8_t, expectedValid_int8_t, Anki::Util::array_size(fromValues));
  }
  // to: int16_t
  {
    int16_t expectedValues_int16_t[] = {
      -32768, -32768, -32768, -32768, -32768,
      -32768, -32768, -32768, -32768, -32768,
      -32768, -32768, -32768, -32768, -32768,
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, -1, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767, 32767,
      32767, 32767, 32767, 32767    };
    
    bool expectedValid_int16_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, false, false,
      false, false, false, false, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int16_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int16_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int16_t, expectedValid_int16_t, Anki::Util::array_size(fromValues));
  }
  // to: int32_t
  {
    int32_t expectedValues_int32_t[] = {
      -2147483648, -2147483648, -2147483648, -2147483648, -2147483648,
      -2147483648, -2147483648, -2147483648, -2147483648, -2147483648,
      -65537, -65536, -65535, -65534, -32769,
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, -1, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65536, 65537, 2147483647,
      2147483647, 2147483647, 2147483647, 2147483647, 2147483647,
      2147483647, 2147483647, 2147483647, 2147483647    };
    
    bool expectedValid_int32_t[] = {
      false, false, false, false, false,
      false, false, false, false, false,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, false,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int32_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int32_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int32_t, expectedValid_int32_t, Anki::Util::array_size(fromValues));
  }
  // to: int64_t
  {
    int64_t expectedValues_int64_t[] = {
      kInt64Lowest, kInt64Lowest, kInt64Lowest, kInt64Lowest, kInt64Lowest,
      kInt64Lowest, kInt64Lowest, kInt64Lowest, kInt64Lowest, -100000000000,
      -65537, -65536, -65535, -65534, -32769,
      -32768, -32767, -32766, -257, -256,
      -255, -254, -129, -128, -127,
      -126, -1, -1, 0, 0,
      0, 1, 1, 126, 127,
      128, 129, 254, 255, 256,
      257, 32766, 32767, 32768, 32769,
      65534, 65535, 65536, 65537, 100000000000,
      9223372036854775807, 9223372036854775807, 9223372036854775807, 9223372036854775807, 9223372036854775807,
      9223372036854775807, 9223372036854775807, 9223372036854775807, 9223372036854775807    };
    
    bool expectedValid_int64_t[] = {
      false, false, false, false, false,
      false, false, false, false, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      false, false, false, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_int64_t), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_int64_t),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_int64_t, expectedValid_int64_t, Anki::Util::array_size(fromValues));
  }
  // to: float
  {
    float expectedValues_float[] = {
      kFloatLowest, kFloatLowest, kFloatLowest, kFloatLowest, kFloatLowest,
      kFloatLowest, -1.000000e+38, -1.000000e+29, -1.000000e+20, -99999997952.000000f,
      -65537.000000f, -65536.000000f, -65535.000000f, -65534.000000f, -32769.000000f,
      -32768.000000f, -32767.000000f, -32766.000000f, -257.000000f, -256.000000f,
      -255.000000f, -254.000000f, -129.000000f, -128.000000f, -127.000000f,
      -126.000000f, -1.500000f, -1.000000f, -0.500000f, 0.000000f,
      0.500000f, 1.000000f, 1.500000f, 126.000000f, 127.000000f,
      128.000000f, 129.000000f, 254.000000f, 255.000000f, 256.000000f,
      257.000000f, 32766.000000f, 32767.000000f, 32768.000000f, 32769.000000f,
      65534.000000f, 65535.000000f, 65536.000000f, 65537.000000f, 99999997952.000000f,
      1.000000e+20, 1.000000e+29, 1.000000e+38, kFloatMax, kFloatMax,
      kFloatMax, kFloatMax, kFloatMax, kFloatMax    };
    
    bool expectedValid_float[] = {
      false, false, false, false, false,
      false, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, false, false,
      false, false, false, false    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_float), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_float),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_float, expectedValid_float, Anki::Util::array_size(fromValues));
  }
  // to: double
  {
    double expectedValues_double[] = {
      kDoubleLowest, -1.000000e+300, -1.000000e+200, -1.000000e+100, -1.000000e+40,
      -1.000000e+39, -1.000000e+38, -1.000000e+29, -1.000000e+20, -100000000000.000000,
      -65537.000000, -65536.000000, -65535.000000, -65534.000000, -32769.000000,
      -32768.000000, -32767.000000, -32766.000000, -257.000000, -256.000000,
      -255.000000, -254.000000, -129.000000, -128.000000, -127.000000,
      -126.000000, -1.500000, -1.000000, -0.500000, 0.000000,
      0.500000, 1.000000, 1.500000, 126.000000, 127.000000,
      128.000000, 129.000000, 254.000000, 255.000000, 256.000000,
      257.000000, 32766.000000, 32767.000000, 32768.000000, 32769.000000,
      65534.000000, 65535.000000, 65536.000000, 65537.000000, 100000000000.000000,
      1.000000e+20, 1.000000e+29, 1.000000e+38, 1.000000e+39, 1.000000e+40,
      1.000000e+100, 1.000000e+200, 1.000000e+300, kDoubleMax    };
    
    bool expectedValid_double[] = {
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true, true,
      true, true, true, true    };
    
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValues_double), "Array Sizes Differ!");
    static_assert(Anki::Util::array_size(fromValues) == Anki::Util::array_size(expectedValid_double),  "Array Sizes Differ!");
    
    TestNumericCastValues(fromValues, expectedValues_double, expectedValid_double, Anki::Util::array_size(fromValues));
  }
}

