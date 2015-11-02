using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

public class CozmoPalette {
  public static uint ColorToUInt(Color color) { 
    uint value = (uint)(((uint)(255f * color.r) << 24) | ((uint)(255f * color.g) << 16) | ((uint)(255f * color.b) << 8) | ((uint)(255f * color.a) << 0));
    return value;
  }

}
