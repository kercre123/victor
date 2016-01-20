using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

public static class CozmoPalette {
  public static uint ColorToUInt(Color color) { 
    uint value = (uint)(((uint)(255f * color.r) << 24) | 
      ((uint)(255f * color.g) << 16) | 
      ((uint)(255f * color.b) << 8) | 
      ((uint)(255f * color.a) << 0));
    return value;
  }

  public static Color32 UIntToColor32(uint value) {
    Color color = new Color32(
      (byte)(value >> 24), 
      (byte)((value >> 16) & 0xFF), 
      (byte)((value >> 8) & 0xFF), 
      (byte)(value & 0xFF));
    return color;
  }

  public static Color UIntToColor(uint value) {
    return UIntToColor32(value).ToColor();
  }

  public static uint ToUInt(this Color color) {
    return ColorToUInt(color);
  }

  public static Color ToColor(this uint value) {
    return UIntToColor(value);
  }

  public static Color32 ToColor32(this uint value) {
    return UIntToColor32(value);
  }

  public static Color ToColor(this Color32 color32) {
    return new Color(color32.r / 255f, color32.g / 255f, color32.b / 255f, color32.a / 255f);
  }

  public static Color32 ToColor32(this Color color) {
    return new Color32(
      (byte)(color.r * 255), 
      (byte)(color.g * 255), 
      (byte)(color.b * 255), 
      (byte)(color.a * 255));
  }

}
