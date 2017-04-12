using System;
using Anki.Cozmo;
using UnityEngine;

public static class ImageResolutionTable {

  public struct ImageDims { 
    public readonly int Width;
    public readonly int Height;

    public ImageDims(int w, int h) {
      Width = w;
      Height = h;
    }

    public static implicit operator Vector2(ImageDims dims) {
      return new Vector2(dims.Width, dims.Height);
    }
  }

  private static ImageDims[] CameraResInfo =
  {
    new ImageDims(16,   16),
    new ImageDims(40,   30),
    new ImageDims(80,   60),
    new ImageDims(160,  120),
    new ImageDims(320,  240),
    new ImageDims(400,  296),
    new ImageDims(640,  480),
    new ImageDims(800,  600),
    new ImageDims(1024, 768),
    new ImageDims(1280, 960),
    new ImageDims(1600, 1200),
    new ImageDims(2048, 1536),
    new ImageDims(3200, 2400),
  };

  public static ImageDims GetDimensions(ImageResolution resolution) {
    return CameraResInfo[(int)resolution];
  }
}

