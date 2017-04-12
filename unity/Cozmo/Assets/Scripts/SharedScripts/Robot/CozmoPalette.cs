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

  public static ushort ColorToUShort(Color color) {
    ushort value = (ushort)(((ushort)(31f) << 15) |
      ((ushort)(31f * color.r) << 10) |
      ((ushort)(31f * color.g) << 5) |
      ((ushort)(31f * color.b)));
    return value;
  }

  public static Color32 UShortToColor32(ushort value) {
    float factor = byte.MaxValue / 31f; // Max value of 5 bits is 31f
    Color color = new Color32(
      (byte)(((value >> 10) & 0x1F) * factor),
      (byte)(((value >> 5) & 0x1F) * factor),
      (byte)(((value) & 0x1F) * factor),
      byte.MaxValue);
    return color;
  }

  public static Color UShortToColor(ushort value) {
    return UShortToColor32(value).ToColor();
  }

  public static ushort ToUShort(this Color color) {
    return ColorToUShort(color);
  }

  public static Color ToColor(this uint value) {
    return UIntToColor(value);
  }

  public static Color32 ToColor32(this uint value) {
    return UIntToColor32(value);
  }

  public static Color ToColor(this ushort value) {
    return UShortToColor(value);
  }

  public static Color32 ToColor32(this ushort value) {
    return UShortToColor32(value);
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

  public static float ToGrayscale(this Color c) {
    return 0.3f * c.r + 0.59f * c.g + 0.11f * c.b;
  }
}
