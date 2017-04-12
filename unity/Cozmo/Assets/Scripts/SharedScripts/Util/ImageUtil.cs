using System;
using System.IO;
using UnityEngine;
using System.Collections;
using Cozmo.Util;

public static class ImageUtil {


  public static readonly byte[] OneByteBuffer = new byte[1];
  public static readonly byte[] ThreeByteBuffer = new byte[3];



  #region JPEG

  // Pre-baked JPEG header for grayscale, Q50
  static readonly byte[] _Header50 = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, // 0x19 = QTable
    0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23,
    0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40,
    0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D,

    //0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0, // 0x5E = Height x Width
    0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x01, 0x28, // 0x5E = Height x Width

    //0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
    0x01, 0x90, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,

    0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
    0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01,
    0x00, 0x00, 0x3F, 0x00
  };

  // Pre-baked JPEG header for grayscale, Q80
  static readonly byte[] _Header80 = {
    0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x06, 0x04, 0x05, 0x06, 0x05, 0x04, 0x06,
    0x06, 0x05, 0x06, 0x07, 0x07, 0x06, 0x08, 0x0A, 0x10, 0x0A, 0x0A, 0x09, 0x09, 0x0A, 0x14, 0x0E,
    0x0F, 0x0C, 0x10, 0x17, 0x14, 0x18, 0x18, 0x17, 0x14, 0x16, 0x16, 0x1A, 0x1D, 0x25, 0x1F, 0x1A,
    0x1B, 0x23, 0x1C, 0x16, 0x16, 0x20, 0x2C, 0x20, 0x23, 0x26, 0x27, 0x29, 0x2A, 0x29, 0x19, 0x1F,
    0x2D, 0x30, 0x2D, 0x28, 0x30, 0x25, 0x28, 0x29, 0x28, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0,
    0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
    0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
    0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
    0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
    0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
    0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
    0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
    0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
    0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
    0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
    0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01,
    0x00, 0x00, 0x3F, 0x00
  };

  // Turn a fully assembled MINIPEG_GRAY image into a JPEG with header and footer
  // This is a port of C# code from Nathan.
  public static void MinimizedGreyToJpeg(MemoryStream bufferIn, MemoryStream bufferOut, int height, int width)
  {
    // Fetch quality to decide which header to use
    //int quality = bufferIn[0];
    int quality = 50;

    byte[] header = null;
    switch(quality)
    {
    case 50:
      header = _Header50;
      break;
    case 80:
      header = _Header80;
      break;
    default:
      DAS.Error("MinimizedGrayToJpeg", "No header for quality of " + quality);
      return;
    }

    bufferOut.Seek(0, SeekOrigin.Begin);
    bufferOut.SetLength(0);

    bufferOut.Write(header, 0, header.Length);

    // Adjust header size information
    bufferOut.Seek(0x5e, SeekOrigin.Begin);
    bufferOut.WriteByte((byte)(height >> 8));
    bufferOut.WriteByte((byte)(height & 0xff));
    bufferOut.WriteByte((byte)(width >> 8));
    bufferOut.WriteByte((byte)(width & 0xff));

    while(true) {
      bufferIn.Seek(-1, SeekOrigin.End);
      bufferIn.Read(OneByteBuffer,0,1);
      if(OneByteBuffer[0] == 0xff) {
        bufferIn.SetLength(bufferIn.Length - 1); // Remove trailing 0xFF padding
      }
      else {
        break;
      }
    }

    // Add byte stuffing - one 0 after each 0xff
    bufferIn.Seek(1, SeekOrigin.Begin);
    bufferOut.Seek(header.Length, SeekOrigin.Begin);

    for (int i = 1; i < bufferIn.Length; i++)
    {
      bufferIn.Read(OneByteBuffer, 0, 1);

      bufferOut.Write(OneByteBuffer, 0, 1);
      if (OneByteBuffer[0] == 0xff) {
        bufferOut.WriteByte(0);
      }
    }

    bufferOut.WriteByte(0xFF);
    bufferOut.WriteByte(0xD9);
  } // miniGrayToJpeg()


  #endregion //JPEG


  #region Filter
  public static void Convolve(Color[] pixels, Color[] newPixels, int width, int height, float[,] kernel, out int newWidth, out int newHeight) {

    int kw = kernel.GetLength(0);
    int kh = kernel.GetLength(1);

    int borderX = (kw / 2);
    int borderY = (kh / 2);

    newWidth = width - borderX * 2;
    newHeight = height - borderY * 2;

    for (int i = 0; i < newWidth; i++) {
      for (int j = 0; j < newHeight; j++) {
        int newIndex = i + newWidth * j;

        Color c = Color.clear;

        for (int m = 0; m < kw; m++) {
          for (int n = 0; n < kh; n++) {
            int oldIndex = (i + m) + width * (j + n);

            c += kernel[m, n] * pixels[oldIndex];
          }
        }

        newPixels[newIndex] = c;
      }
    }
  }

  // Uses only the R value of the color and sets x in R of result and y in G of result
  // and atan2(x,y) in B if the option bool gradient is set
  public static void ConvolveXandY(Color[] pixels, Color[] newPixels, int width, int height, float[,] kernel, out int newWidth, out int newHeight, bool calculateMagnitude) {

    int kw = kernel.GetLength(0);
    int kh = kernel.GetLength(1);

    int border = Mathf.Max((kw / 2), (kh / 2));


    newWidth = width - border * 2;
    newHeight = height - border * 2;

    for (int i = 0; i < newWidth; i++) {
      for (int j = 0; j < newHeight; j++) {
        int newIndex = i + newWidth * j;

        Color c = Color.black;

        for (int m = 0; m < kw; m++) {
          for (int n = 0; n < kh; n++) {
            int oldIndexX = (i + m) + width * (j + n);
            int oldIndexY = (i + n) + width * (j + m);

            c.r += kernel[m, n] * pixels[oldIndexX].ToGrayscale();
            c.g += kernel[m, n] * pixels[oldIndexY].ToGrayscale();
          }
        }

        if (calculateMagnitude) {
          c.b = Mathf.Sqrt(c.r * c.r + c.g * c.g);
        }

        newPixels[newIndex] = c;
      }
    }
  }

  public static void Flatten(Color[] pixels, Color[] newPixels, int width, int height, int levels) {
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {

        int index = i + width * j;
        var c = pixels[index];

        c.r = Mathf.Round(c.r * levels) / levels;
        c.g = Mathf.Round(c.g * levels) / levels;
        c.b = Mathf.Round(c.b * levels) / levels;

        newPixels[index] = c;
      }
    }
  }

  public static void GrayScaleToColorUsingGradient(Color[] pixels, Color[] newPixels, int width, int height, Gradient gradient) {
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height; j++) {

        int index = i + width * j;
        var c = pixels[index];

        newPixels[index] = gradient.Evaluate(c.ToGrayscale());
      }
    }
  }

  public static void SubtractOneMinus(Color[] pixelsA, Color[] pixelsB, Color[] newPixels, int widthA, int heightA, int widthB, int heightB, out int newWidth, out int newHeight) {

    newWidth = Mathf.Min(widthA, widthB);
    newHeight = Mathf.Min(heightA, heightB);

    int offsetXA = (widthA - newWidth) / 2;
    int offsetYA = (heightA - newHeight) / 2;
    int offsetXB = (widthB - newWidth) / 2;
    int offsetYB = (heightB - newHeight) / 2;

    for (int i = 0; i < newWidth; i++) {
      for (int j = 0; j < newHeight; j++) {

        int index = i + newWidth * j;
        int indexA = i + offsetXA + widthA * (j + offsetYA);
        int indexB = i + offsetXB + widthB * (j + offsetYB);

        newPixels[index] = pixelsA[indexA] - (Color.white - pixelsB[indexB]);
      }
    }
  }



  public static void PaintDots(Color[] pixelsA, Color[] pixelsB, int width, int height, int radius, bool first = false) {
    PaintLinesWithGradient(pixelsA, pixelsB, null, width, height, 0, 0, radius, first);
  }

  public static void SetAlpha(Color[] pixels, float alpha) {
    for (int i = 0; i < pixels.Length; i++) {
      pixels[i].a = alpha;
    }
  }

  // scale image using bilinear interpolation
  public static Color[] ScaleImage(Color[] pixels, int width, int height, out int newWidth, out int newHeight, float scale) {
    newHeight = (int)(height * scale);
    newWidth = (int)(width * scale);

    Color[] result = new Color[newWidth * newHeight];

    for (int i = 0; i < newWidth; i++) {
      for (int j = 0; j < newHeight; j++) {

        float x = i / scale;
        float y = j / scale;
        int xMin = Mathf.FloorToInt(x);
        int xMax = Mathf.CeilToInt(x);
        int yMin = Mathf.FloorToInt(y);
        int yMax = Mathf.CeilToInt(y);

        if (xMax == width) {
          xMax = xMin;
        }
        if (yMax == height) {
          yMax = yMin;
        }

        Color bl = pixels[xMin + width * yMin];
        Color br = pixels[xMax + width * yMin];
        Color tl = pixels[xMin + width * yMax];
        Color tr = pixels[xMax + width * yMax];

        result[i + j * newWidth] = Color.Lerp(Color.Lerp(bl, br, x - xMin), Color.Lerp(tl, tr, x - xMin), y - yMin);
      }
    }
    return result;
  }

  public static void PaintLinesWithGradient(Color[] pixelsA, Color[] pixelsB, Color[] gradient, int width, int height, int gradientWidth, int gradientHeight, int radius, bool first = false) {

    int lineLength = radius * 4;
    int length = (width * height);
    int gradientLength = (gradientWidth * gradientHeight);

    int r2 = (radius * radius);

    int borderX = (width - gradientWidth) / 2;
    int borderY = (height - gradientHeight) / 2;

    const float _kGridMultiple = 0.5f;

    int gridSize = Mathf.Max(1, (int)(radius * _kGridMultiple));
    int gridHalf = gridSize / 2;

    // if grid size is 1, grid half will be 0, which means no pixels will be checked.
    int gridHalfRight = Mathf.Max(1, gridHalf);

    float magThreshold = gridSize * gridSize * 0.1f;

    System.Random rand = new System.Random();

    for(int i = gridHalf; i < width; i+= gridSize) {
      for(int j = gridHalf; j < height; j+= gridSize) {

        float sumMags = 0f;
        float maxMag = 0f;
        int xIndex = 0, yIndex = 0;
        for (int x = -gridHalf; x < gridHalfRight; x++) {
          for (int y = -gridHalf; y < gridHalfRight; y++) {
            int ix = i + x + (j + y) * width;

            if (ix >= 0 && ix < length) {
              var ca = pixelsA[ix];
              var cb = pixelsB[ix];

              var mag = Mathf.Sqrt((ca.r - cb.r) * (ca.r - cb.r) + (ca.g - cb.g) * (ca.g - cb.g) + (ca.b - cb.b) * (ca.b - cb.b));

              if (mag > maxMag) {
                maxMag = mag;
                xIndex = i + x;
                yIndex = j + y;
              }
              sumMags += mag;
            }
          }
        }

        if (!first && sumMags < magThreshold) {
          continue;
        }

        int index = xIndex + yIndex * width;
        int gradientIndex = xIndex - borderX + (yIndex - borderY) * gradientWidth;

        var c = pixelsA[index];
        bool point = true;
        Vector2 n = Vector2.zero;
        if (gradientIndex >= 0 && gradientIndex < gradientLength) {
          var g = gradient[gradientIndex];
          var gv = new Vector2(g.r, g.g);
          if (!gv.Approximately(Vector2.zero)) {
            n = gv.normalized;
            point = false;
          }
        }

        float alpha = (float)rand.NextDouble();


        for (int k = 0; k < (point ? 1 : lineLength); k++) {

          var offset = n * k;

          for (int x = -radius; x <= radius; x++) {
            for (int y = -radius; y <= radius; y++) {

              float cX = (xIndex + offset.x);
              float cY = (yIndex + offset.y);

              int pX = (int)cX + x;
              int pY = (int)cY + y;

              int endIndex = pX + (pY * width);

              if (endIndex >= 0 && endIndex < length) {

                if (alpha >= pixelsB[endIndex].a) {                  
                  if (r2 >= ((pX - cX) * (pX - cX) + (pY - cY) * (pY - cY))) {

                    pixelsB[endIndex] = c;
                    pixelsB[endIndex].a = alpha;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  public static void NonMaximalSuppression(Color[] pixels, Color[] newPixels, int width, int height, out int newWidth, out int newHeight) {
    
    const float _kMagnitudeThreshold = 0.15f;
    const float _kMagnitudeScale = 20f;

    newWidth = width - 2;
    newHeight = height - 2;

    for (int x = 0; x < newWidth; x++) {
      for (int y = 0; y < newHeight; y++) {
        int newIndex = x + y * newWidth;

        int index = (x + 1) + (y + 1) * width;
        int indexN = index - width;
        int indexS = index + width;
        int indexW = index - 1;
        int indexE = index + 1;
        int indexNW = indexN - 1;
        int indexNE = indexN + 1;
        int indexSW = indexS - 1;
        int indexSE = indexS + 1;

        float xGrad = pixels[index].r;
        float yGrad = pixels[index].g;
        float gradMag = pixels[index].b;

        //perform non-maximal supression
        float nMag = pixels[indexN].b;
        float sMag = pixels[indexS].b;
        float wMag = pixels[indexW].b;
        float eMag = pixels[indexE].b;
        float neMag = pixels[indexNE].b;
        float seMag = pixels[indexSE].b;
        float swMag = pixels[indexSW].b;
        float nwMag = pixels[indexNW].b;
        float tmp;

        // http://www.tomgibara.com/computer-vision/CannyEdgeDetector.java
        /*
         * An explanation of what's happening here, for those who want
         * to understand the source: This performs the "non-maximal
         * supression" phase of the Canny edge detection in which we
         * need to compare the gradient magnitude to that in the
         * direction of the gradient; only if the value is a local
         * maximum do we consider the point as an edge candidate.
         * 
         * We need to break the comparison into a number of different
         * cases depending on the gradient direction so that the
         * appropriate values can be used. To avoid computing the
         * gradient direction, we use two simple comparisons: first we
         * check that the partial derivatives have the same sign (1)
         * and then we check which is larger (2). As a consequence, we
         * have reduced the problem to one of four identical cases that
         * each test the central gradient magnitude against the values at
         * two points with 'identical support'; what this means is that
         * the geometry required to accurately interpolate the magnitude
         * of gradient function at those points has an identical
         * geometry (upto right-angled-rotation/reflection).
         * 
         * When comparing the central gradient to the two interpolated
         * values, we avoid performing any divisions by multiplying both
         * sides of each inequality by the greater of the two partial
         * derivatives. The common comparand is stored in a temporary
         * variable (3) and reused in the mirror case (4).
         * 
         */

        float mag = 1;
        if (xGrad * yGrad <= 0f /*(1)*/
          ? Mathf.Abs(xGrad) >= Mathf.Abs(yGrad) /*(2)*/
          ? (tmp = Mathf.Abs(xGrad * gradMag)) >= Mathf.Abs(yGrad * neMag - (xGrad + yGrad) * eMag) /*(3)*/
          && tmp > Mathf.Abs(yGrad * swMag - (xGrad + yGrad) * wMag) /*(4)*/
          : (tmp = Mathf.Abs(yGrad * gradMag)) >= Mathf.Abs(xGrad * neMag - (yGrad + xGrad) * nMag) /*(3)*/
          && tmp > Mathf.Abs(xGrad * swMag - (yGrad + xGrad) * sMag) /*(4)*/
          : Mathf.Abs(xGrad) >= Mathf.Abs(yGrad) /*(2)*/
          ? (tmp = Mathf.Abs(xGrad * gradMag)) >= Mathf.Abs(yGrad * seMag + (xGrad - yGrad) * eMag) /*(3)*/
          && tmp > Mathf.Abs(yGrad * nwMag + (xGrad - yGrad) * wMag) /*(4)*/
          : (tmp = Mathf.Abs(yGrad * gradMag)) >= Mathf.Abs(xGrad * seMag + (yGrad - xGrad) * sMag) /*(3)*/
          && tmp > Mathf.Abs(xGrad * nwMag + (yGrad - xGrad) * nMag) /*(4)*/
        ) {
          mag = 1 - Mathf.Max(0, _kMagnitudeScale * (gradMag - _kMagnitudeThreshold));
          //NOTE: The orientation of the edge is not employed by this
          //implementation. It is a simple matter to compute it at
          //this point as: Math.atan2(yGrad, xGrad);
        }
        newPixels[newIndex] = new Color(mag, mag, mag);
      }
    }
  }

  private static float[,] _sGaussKernel = {
    { 2f/159f,  4f/159f,  5f/159f,  4f/159f,  2f/159f },
    { 4f/159f,  9f/159f, 12f/159f,  9f/159f,  4f/159f },
    { 5f/159f, 12f/159f, 15f/159f, 12f/159f,  5f/159f },
    { 4f/159f,  9f/159f, 12f/159f,  9f/159f,  4f/159f },
    { 2f/159f,  4f/159f,  5f/159f,  4f/159f,  2f/159f }
  };

  private static float[,] _sSobelKernel = {
    { -1, 0, 1 },
    { -2, 0, 2 },
    { -1, 0, 1 } 
  };

  public static Texture2D CannyEdgeDetection(Texture2D texture) {

    Color[] bufferA = texture.GetPixels();
    Color[] bufferB = new Color[bufferA.Length];

    int width, height;

    CannyEdgeDetection(bufferA, bufferB, texture.width, texture.height, out width, out height);

    Texture2D result = new Texture2D(width, height, texture.format, false);

    result.SetPixels(0, 0, width, height, bufferB);
    result.Apply();
    return result;
  }

  public static void CannyEdgeDetection(Color[] bufferA, Color[] bufferB, int width, int height, out int newWidth, out int newHeight) {
    Convolve(bufferA, bufferB, width, height, _sGaussKernel, out newWidth, out newHeight);

    ConvolveXandY(bufferB, bufferA, newWidth, newHeight, _sSobelKernel, out newWidth, out newHeight, true);

    NonMaximalSuppression(bufferA, bufferB, newWidth, newHeight, out newWidth, out newHeight);
  }
    
  #endregion // Filter

  #region Sketch

  public static Texture2D Sketch(Texture2D texture, int sketchColors = 8, Gradient colorGradient = null) {

    Color[] buffer = texture.GetPixels();

    int width = texture.width, height = texture.height;

    SketchInternal(buffer, ref width, ref height, sketchColors, colorGradient);

    Texture2D result = new Texture2D(width, height, texture.format, false);
    result.SetPixels(0, 0, width, height, buffer);
    result.Apply();
    return result;
  }

  private static void SketchInternal(Color[] bufferA, ref int width, ref int height, int sketchColors, Gradient colorGradient) {
    Color[] bufferB = new Color[bufferA.Length];
    Color[] bufferC = new Color[bufferA.Length];


    Convolve(bufferA, bufferB, width, height, _sGaussKernel, out width, out height);

    Flatten(bufferB, bufferC, width, height, sketchColors);

    if (colorGradient != null) {
      GrayScaleToColorUsingGradient(bufferC, bufferC, width, height, colorGradient);
    }

    int fillWidth = width, fillHeight = height;

    ConvolveXandY(bufferB, bufferA, width, height, _sSobelKernel, out width, out height, true);
    NonMaximalSuppression(bufferA, bufferB, width, height, out width, out height);

    SubtractOneMinus(bufferC, bufferB, bufferA, fillWidth, fillHeight, width, height, out width, out height);
  }

  private class SketchAsyncParam {
    public Color[] Buffer;
    public int Width;
    public int Height;
    public volatile bool Done;
    public Gradient ColorGradient;
    public int Colors;
  }

  public static IEnumerator SketchAsync(Texture2D texture, Action<Texture2D> callback, int sketchColors = 8, Gradient colorGradient = null) {
    if (callback == null) {
      yield break;
    }

    SketchAsyncParam param = new SketchAsyncParam() {
      Buffer = texture.GetPixels(),
      Width = texture.width,
      Height = texture.height,
      ColorGradient = colorGradient,
      Colors = sketchColors,
    };

    System.Threading.ThreadPool.QueueUserWorkItem(SketchAsyncInternal, param);

    while (!param.Done) {
      yield return null;
    }

    Texture2D result = new Texture2D(param.Width, param.Height, texture.format, false);
    result.SetPixels(0, 0, param.Width, param.Height, param.Buffer);
    result.Apply();
    callback(result);
  }

  private static void SketchAsyncInternal(object obj) {
    SketchAsyncParam param = (SketchAsyncParam)obj;
    SketchInternal(param.Buffer, ref param.Width, ref param.Height, param.Colors, param.ColorGradient);
    param.Done = true;
  }

  #endregion //Sketch

  #region Paint

  public static Texture2D Paint(Texture2D texture, bool dots = true, Gradient colorGradient = null) {

    Color[] buffer = texture.GetPixels();

    int width = texture.width, height = texture.height;

    PaintInternal(ref buffer, ref width, ref height, dots, colorGradient);

    Texture2D result = new Texture2D(width, height, texture.format, false);
    result.SetPixels(0, 0, width, height, buffer);
    result.Apply();
    return result;
  }

  private static void PaintInternal(ref Color[] bufferA, ref int width, ref int height, bool dots, Gradient colorGradient) {

    bufferA = ScaleImage(bufferA, width, height, out width, out height, 2);

    Color[] bufferB = new Color[bufferA.Length];
    Color[] bufferC = new Color[bufferA.Length];

    Convolve(bufferA, bufferB, width, height, _sGaussKernel, out width, out height);

    bufferA.Fill(default(Color));
    if (dots) {

      PaintDots(bufferB, bufferA, width, height, 8, true);
      SetAlpha(bufferA, 0f);
      PaintDots(bufferB, bufferA, width, height, 4);
      SetAlpha(bufferA, 0f);
      PaintDots(bufferB, bufferA, width, height, 2);
      SetAlpha(bufferA, 1f);

    }
    else {
      int gradientWidth, gradientHeight; 

      ConvolveXandY(bufferB, bufferC, width, height, _sSobelKernel, out gradientWidth, out gradientHeight, false);

      PaintLinesWithGradient(bufferB, bufferA, bufferC, width, height, gradientWidth, gradientHeight, 8, true);
      SetAlpha(bufferA, 0f);
      PaintLinesWithGradient(bufferB, bufferA, bufferC, width, height, gradientWidth, gradientHeight, 4);
      SetAlpha(bufferA, 0f);
      PaintLinesWithGradient(bufferB, bufferA, bufferC, width, height, gradientWidth, gradientHeight, 2);
      SetAlpha(bufferA, 1f);
    }

    if (colorGradient != null) {
      GrayScaleToColorUsingGradient(bufferA, bufferA, width, height, colorGradient);
    }
  }

  private class PaintAsyncParam {
    public Color[] Buffer;
    public int Width;
    public int Height;
    public volatile bool Done;
    public Gradient ColorGradient;
    public bool Dots;
  }

  public static IEnumerator PaintAsync(Texture2D texture, Action<Texture2D> callback, bool dots = true, Gradient colorGradient = null) {
    if (callback == null) {
      yield break;
    }

    PaintAsyncParam param = new PaintAsyncParam() {
      Buffer = texture.GetPixels(),
      Width = texture.width,
      Height = texture.height,
      ColorGradient = colorGradient,
      Dots = dots
    };

    System.Threading.ThreadPool.QueueUserWorkItem(PaintAsyncInternal, param);

    while (!param.Done) {
      yield return null;
    }

    Texture2D result = new Texture2D(param.Width, param.Height, texture.format, false);
    result.SetPixels(0, 0, param.Width, param.Height, param.Buffer);
    result.Apply();
    callback(result);
  }

  private static void PaintAsyncInternal(object obj) {
    PaintAsyncParam param = (PaintAsyncParam)obj;
    PaintInternal(ref param.Buffer, ref param.Width, ref param.Height, param.Dots, param.ColorGradient);
    param.Done = true;
  }

  #endregion // Paint
}
