using UnityEngine;
using Anki.Cozmo;
using System;
using TMPro;

namespace CodeLab {

  public class CodeLabCozmoFaceDisplay {

    // Cozmo's Face is 128x64 interlaced, so only 128x32 can be displayed at any time
    private const int kFaceScreenWidth = 128;
    private const int kFaceScreenHeight = 64;

    private const int kFaceTextureWidth = kFaceScreenWidth;   // must be exact integer divisor of kFaceScreenWidth
    private const int kFaceTextureHeight = kFaceScreenHeight; // must be exact integer divisor of kFaceScreenHeight

    private byte[] _DrawToFaceBytes = new byte[1024]; // CLAD message requires this format (one bit per pixel)
    private byte[] _DrawToFacePixels = new byte[kFaceTextureWidth * kFaceTextureHeight]; // One byte per pixel

    private TMP_FontAsset _FontAsset = null;
    private TMP_FontAsset _KanaFontAsset = null;
    private Texture2D _FontTexture = null;
    private Texture2D _KanaFontTexture = null;
    private bool _IsFontInitialized = false;
    private const float kDefaultFontSpaceWidth = 20.0f;
    private float _FontSpaceWidth = kDefaultFontSpaceWidth;

    public void ClearScreen(byte clearCol = 0) {
      int i = 0;
      for (int y = 0; y < kFaceTextureHeight; ++y) {
        for (int x = 0; x < kFaceTextureWidth; ++x) {
          _DrawToFacePixels[i++] = clearCol;
        }
      }
    }

    private bool InitFontAsset() {
      if (_IsFontInitialized) {
        // Already initialized, just return if initialization was succesful
        return (_FontAsset != null);
      }

      var fontName = "AvenirLTStd-Medium SDF.asset";
      var kanaFontName = "AvenirLTStd-Medium SDF Kana.asset";
      _FontAsset = CozmoThemeSystemUtils.sInstance.LoadFontTMP(fontName) as TMP_FontAsset;
      _KanaFontAsset = CozmoThemeSystemUtils.sInstance.LoadFontTMP(kanaFontName) as TMP_FontAsset;

      if (_FontAsset == null) {
        DAS.Error("CodeLab.InitFontAsset.NullFont", "Failed to load font: " + fontName);
        _IsFontInitialized = true;  // Tried to initialize, but failed (don't retry)
        return false;
      }

      if (_KanaFontAsset == null) {
        DAS.Warn("CodeLab.InitFontAsset.NullKanaFont", "Failed to load font: " + kanaFontName);
      }

      // Copy the font's texture to a CPU-readable version
      _FontTexture = InitFontTexture(_FontAsset);
      if (_KanaFontAsset != null) {
        _KanaFontTexture = InitFontTexture(_KanaFontAsset);
      }

      TMP_Glyph glyph = null;
      if (_FontAsset.characterDictionary.TryGetValue((int)' ', out glyph)) {
        _FontSpaceWidth = glyph.xAdvance;
      }
      else {
        DAS.Error("CodeLab.GetSpaceWidth.NoSpace", "No space in font: " + _FontAsset.fontInfo.Name);
        _FontSpaceWidth = kDefaultFontSpaceWidth;
      }

      _IsFontInitialized = true;

      return true;
    }

    private Texture2D InitFontTexture(TMP_FontAsset fontAsset) {
      var sourceTexture = fontAsset.atlas;

      RenderTexture tempRenderTexture = RenderTexture.GetTemporary(
          sourceTexture.width,
          sourceTexture.height,
          0,
          RenderTextureFormat.Default,
          RenderTextureReadWrite.Linear);

      // Blit the pixels on texture to the RenderTexture
      Graphics.Blit(sourceTexture, tempRenderTexture);

      // Backup the currently set RenderTexture, and switch to our temporary one
      RenderTexture cachedActiveRenderTexture = RenderTexture.active;
      RenderTexture.active = tempRenderTexture;

      // Create a new readable Texture2D to copy the pixels to it
      var fontTexture = new Texture2D(sourceTexture.width, sourceTexture.height);

      // Copy the pixels from the RenderTexture to the new Texture
      //fontTexture will then have the same pixels as fontAsset.atlas, _and_ be readable from CPU
      fontTexture.ReadPixels(new Rect(0, 0, tempRenderTexture.width, tempRenderTexture.height), 0, 0);
      fontTexture.Apply();

      // Reset the active RenderTexture and release our temp texture
      RenderTexture.active = cachedActiveRenderTexture;
      RenderTexture.ReleaseTemporary(tempRenderTexture);
      return fontTexture;
    }

    public void CalculateTextBounds(float scale, string text, out float outWidth, out float outHeight) {
      if (!InitFontAsset()) {
        outWidth = 0.0f;
        outHeight = 0.0f;
        return;
      }

      var characterDictionary = _FontAsset.characterDictionary;
      var kanaCharacterDictionary = _KanaFontAsset != null ? _KanaFontAsset.characterDictionary : null;

      float x = 0.0f;
      float maxY = 0.0f;

      for (int cI = 0; cI < text.Length; ++cI) {
        int charKey = (int)text[cI];
        TMP_Glyph glyph = null;

        if (characterDictionary.TryGetValue(charKey, out glyph) || (kanaCharacterDictionary != null && kanaCharacterDictionary.TryGetValue(charKey, out glyph))) {
          float charMaxY = (Mathf.Abs(glyph.height) - glyph.yOffset) * scale;
          maxY = Mathf.Max(maxY, charMaxY);
          x += glyph.xAdvance * scale;
        }
        else {
          // The character isn't in this font - insert a space
          x += _FontSpaceWidth * scale;
        }
      }

      outWidth = x;
      outHeight = (_FontAsset.fontInfo.Ascender * scale) + maxY;
    }

    public void DrawText(float x, float y, float scale, AlignmentX alignmentX, AlignmentY alignmentY, string text, byte color) {
      if ((alignmentX != AlignmentX.Left) || (alignmentY != AlignmentY.Top)) {
        // Calc size that the text would be, and adjust position appropriately
        float width;
        float height;
        CalculateTextBounds(scale, text, out width, out height);
        switch (alignmentX) {
        case AlignmentX.Left:
          // x remains unchanged
          break;
        case AlignmentX.Center:
          x -= (width * 0.5f);
          break;
        case AlignmentX.Right:
          x -= width;
          break;
        }

        switch (alignmentY) {
        case AlignmentY.Top:
          // y remains unchanged
          break;
        case AlignmentY.Center:
          y -= (height * 0.5f);
          break;
        case AlignmentY.Bottom:
          y -= height;
          break;
        }
      }

      DrawText(x, y, scale, text, color);
    }

    public void DrawText(float x, float y, float scale, string text, byte color) {
      if (!InitFontAsset()) {
        return;
      }

      Texture2D fontTexture = _FontTexture;
      Texture2D kanaFontTexture = _KanaFontTexture;
      Texture2D currentTexture = fontTexture;
      var characterDictionary = _FontAsset.characterDictionary;
      var kanaCharacterDictionary = _KanaFontAsset != null ? _KanaFontAsset.characterDictionary : null;
      var fontInfo = _FontAsset.fontInfo;

      for (int cI = 0; cI < text.Length; ++cI) {
        int charKey = (int)text[cI];
        TMP_Glyph glyph = null;

        bool isNormalCharacter = characterDictionary.TryGetValue(charKey, out glyph);

        if (isNormalCharacter || (kanaCharacterDictionary != null && kanaCharacterDictionary.TryGetValue(charKey, out glyph))) {
          if (isNormalCharacter) {
            currentTexture = fontTexture;
            fontInfo = _FontAsset.fontInfo;
          } else {
            currentTexture = kanaFontTexture;
            fontInfo = _KanaFontAsset.fontInfo;
          }
          float srcMinX = (glyph.x);
          float srcMaxX = (glyph.x + glyph.width);
          float srcMinY = (glyph.y);
          float srcMaxY = (glyph.y + glyph.height);

          float srcRangeX = Mathf.Abs(srcMaxX - srcMinX);
          float srcRangeY = Mathf.Abs(srcMaxY - srcMinY);

          // Calculate the destination rectangle for drawing this character

          // Apply glyph offsets (y offset is relative to the max ascender for the font so that chars line up)
          float destMinXFloat = x + (glyph.xOffset * scale);
          float destMinYFloat = y + ((fontInfo.Ascender - glyph.yOffset) * scale);

          int destMinX = Mathf.RoundToInt(destMinXFloat);
          int destMinY = Mathf.RoundToInt(destMinYFloat);
          int destMaxX = Mathf.RoundToInt(destMinXFloat + (srcRangeX * scale));
          int destMaxY = Mathf.RoundToInt(destMinYFloat + (srcRangeY * scale));
          float srcXPerDestX = srcRangeX / (float)(destMaxX - destMinX);
          float srcYPerDestY = srcRangeY / (float)(destMaxY - destMinY);

          float clippedSrcX = srcMinX;
          float clippedSrcY = srcMinY;

          // Clamp dest to screen, clipping the source
          if (destMinX < 0) {
            clippedSrcX -= (float)destMinX * srcXPerDestX;
            destMinX = 0;
          }
          if (destMinY < 0) {
            clippedSrcY -= (float)destMinY * srcYPerDestY;
            destMinY = 0;
          }
          if (destMaxX >= kFaceTextureWidth) {
            destMaxX = kFaceTextureWidth - 1;
          }
          if (destMaxY >= kFaceTextureHeight) {
            destMaxY = kFaceTextureHeight - 1;
          }

          // Rasterize the character
          float srcY = clippedSrcY;
          for (int destY = destMinY; destY <= destMaxY; ++destY) {
            // Calculate the texel indices and ratios for bilinear filtering in Y
            int srcYInt = (int)srcY;
            int srcYInt1 = currentTexture.height - srcYInt; // y coordinate is flipped
            int srcYInt2 = srcYInt1 - 1;
            float srcYRatio = srcY - (float)srcYInt;
            float srcX = clippedSrcX;
            for (int destX = destMinX; destX <= destMaxX; ++destX) {
              // Calculate the texel indices and ratios for bilinear filtering in X
              int srcXInt1 = (int)srcX;
              int srcXInt2 = srcXInt1 + 1;
              float srcXRatio = srcX - (float)srcXInt1;

              // Calculate the relative weights to use from each of the 4 texels
              bool srcXInt1InBounds = ((srcXInt1 >= 0) && (srcXInt1 < currentTexture.width));
              bool srcXInt2InBounds = ((srcXInt2 >= 0) && (srcXInt2 < currentTexture.width));
              bool srcYInt1InBounds = ((srcYInt1 >= 0) && (srcYInt1 < currentTexture.height));
              bool srcYInt2InBounds = ((srcYInt2 >= 0) && (srcYInt2 < currentTexture.height));

              // Calculate weights for each texel, if texel is clamped then give the weight to their neighboring texels.
              float xRatio1 = srcXInt2InBounds ? (1.0f - srcXRatio) : 1.0f;
              float xRatio2 = srcXInt1InBounds ? srcXRatio : 1.0f;
              float yRatio1 = srcYInt2InBounds ? (1.0f - srcYRatio) : 1.0f;
              float yRatio2 = srcYInt1InBounds ? srcYRatio : 1.0f;

              // Calculated weighted sum from the texels
              float fontAlpha = 0.0f;
              if (srcXInt1InBounds) {
                if (srcYInt1InBounds) {
                  Color fontColor = currentTexture.GetPixel(srcXInt1, srcYInt1);
                  fontAlpha += fontColor.a * xRatio1 * yRatio1;
                }
                if (srcYInt2InBounds) {
                  Color fontColor = currentTexture.GetPixel(srcXInt1, srcYInt2);
                  fontAlpha += fontColor.a * xRatio1 * yRatio2;
                }
              }
              if (srcXInt2InBounds) {
                if (srcYInt1InBounds) {
                  Color fontColor = currentTexture.GetPixel(srcXInt2, srcYInt1);
                  fontAlpha += fontColor.a * xRatio2 * yRatio1;
                }
                if (srcYInt2InBounds) {
                  Color fontColor = currentTexture.GetPixel(srcXInt2, srcYInt2);
                  fontAlpha += fontColor.a * xRatio2 * yRatio2;
                }
              }

              // Compare against threshold to see if this is solid.
              if (fontAlpha > 0.49f) {
                DrawPixel(destX, destY, color);
              }

              srcX += srcXPerDestX;
            }
            srcY += srcYPerDestY;
          }

          x += glyph.xAdvance * scale;
        }
        else {
          // The character isn't in this font - insert a space
          x += _FontSpaceWidth * scale;
        }
      }
    }

    public void DrawPixel(int x, int y, byte color) {
      if ((x >= 0) && (x < kFaceTextureWidth) && (y >= 0) && (y < kFaceTextureHeight)) {
        int destIndex = x + (y * kFaceTextureWidth);
        _DrawToFacePixels[destIndex] = color;

        if (color == 0) {
          // Because of the DoubleUpRowPixels later, any drawing that subtracts pixels
          // should clear both rows to avoid the doubling undoing this and leaving it solid.
          // Even numbered rows match with the next row, odd rows match with the previous row
          int rowOffset = ((y % 2) == 0) ? kFaceTextureWidth : -kFaceTextureWidth;
          destIndex += rowOffset;

          if ((destIndex >= 0) && (destIndex < (kFaceTextureWidth * kFaceTextureHeight))) {
            _DrawToFacePixels[destIndex] = color;
          }
        }
      }
    }

    public void DrawLine(float x1, float y1, float x2, float y2, byte color) {
      int x1Int = Mathf.RoundToInt(x1);
      int y1Int = Mathf.RoundToInt(y1);
      int x2Int = Mathf.RoundToInt(x2);
      int y2Int = Mathf.RoundToInt(y2);
      DrawLineInt(x1Int, y1Int, x2Int, y2Int, color);
    }

    public void DrawLineInt(int x1, int y1, int x2, int y2, byte color) {
      // Fairly standard Bresenham's algorithm, taken from VizManager.cs,
      // which took it from http://wiki.unity3d.com/index.php?title=TextureDrawLine
      var dy = y2 - y1;
      var dx = x2 - x1;

      var stepy = 1;
      var stepx = 1;

      if (dy < 0) {
        dy = -dy;
        stepy = -1;
      }
      if (dx < 0) {
        dx = -dx;
        stepx = -1;
      }
      dy <<= 1;
      dx <<= 1;

      DrawPixel(x1, y1, color);
      if (dx > dy) {
        var fraction = dy - (dx >> 1);
        while (x1 != x2) {
          if (fraction >= 0) {
            y1 += stepy;
            fraction -= dx;
          }
          x1 += stepx;
          fraction += dy;

          DrawPixel(x1, y1, color);
        }
      }
      else {
        var fraction = dx - (dy >> 1);
        while (y1 != y2) {
          if (fraction >= 0) {
            x1 += stepx;
            fraction -= dy;
          }
          y1 += stepy;
          fraction += dx;

          DrawPixel(x1, y1, color);
        }
      }
    }

    public void FillCircle(float x, float y, float radius, byte color) {
      int xInt = Mathf.RoundToInt(x);
      int yInt = Mathf.RoundToInt(y);
      int radiusInt = Mathf.RoundToInt(radius); // use the smaller of the 2 for radius

      FillCircleInt(xInt, yInt, radiusInt, color);
    }

    public void FillCircleInt(int cx, int cy, int radius, byte color) {
      // Find (clamped) rectangle that bounds this circle
      int x1 = Math.Max(cx - radius, 0);
      int x2 = Math.Min(cx + radius, kFaceTextureWidth - 1);
      int y1 = Math.Max(cy - radius, 0);
      int y2 = Math.Min(cy + radius, kFaceTextureHeight - 1);

      int maxRadiusSqr = radius * radius;
      for (int y = y1; y <= y2; ++y) {
        for (int x = x1; x <= x2; ++x) {
          int radiusSqr = ((cx - x) * (cx - x)) + ((cy - y) * (cy - y));
          if (radiusSqr <= maxRadiusSqr) {
            DrawPixel(x, y, color);
          }
        }
      }
    }

    public void DrawCircle(float x, float y, float radius, byte color) {
      int xInt = Mathf.RoundToInt(x);
      int yInt = Mathf.RoundToInt(y);
      int radiusInt = Mathf.RoundToInt(radius); // use the smaller of the 2 for radius

      DrawCircleInt(xInt, yInt, radiusInt, color);
    }

    public void DrawCircleInt(int cx, int cy, int radius, byte color) {
      // MidPoint cirlce algorithm from https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
      int x = radius - 1;
      int y = 0;
      int dx = 1;
      int dy = 1;
      int err = dx - (radius << 1);

      while (x >= y) {
        DrawPixel(cx + x, cy + y, color);
        DrawPixel(cx + y, cy + x, color);
        DrawPixel(cx - y, cy + x, color);
        DrawPixel(cx - x, cy + y, color);
        DrawPixel(cx - x, cy - y, color);
        DrawPixel(cx - y, cy - x, color);
        DrawPixel(cx + y, cy - x, color);
        DrawPixel(cx + x, cy - y, color);

        if (err <= 0) {
          y++;
          err += dy;
          dy += 2;
        }
        if (err > 0) {
          x--;
          dx += 2;
          err += (-radius << 1) + dx;
        }
      }
    }

    public void DrawRect(float x1, float y1, float x2, float y2, byte color) {
      int x1Int = Mathf.RoundToInt(x1);
      int y1Int = Mathf.RoundToInt(y1);
      int x2Int = Mathf.RoundToInt(x2);
      int y2Int = Mathf.RoundToInt(y2);

      DrawRectInt(x1Int, y1Int, x2Int, y2Int, color);
    }

    public void DrawRectInt(int x1, int y1, int x2, int y2, byte color) {
      // make sure our points are in order (TopLeft to BottomRight)
      if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
      }
      if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
      }

      // Clamp to screen
      var minX = Math.Max(x1, 0);
      var maxX = Math.Min(x2, kFaceTextureWidth - 1);
      var minY = Math.Max(y1, 0);
      var maxY = Math.Min(y2, kFaceTextureHeight - 1);

      // Horizontal lines (if not clipped)

      if (y1 == minY) {
        for (int x = minX; x <= maxX; ++x) {
          DrawPixel(x, y1, color);
        }
      }
      if (y2 == maxY) {
        for (int x = minX; x <= maxX; ++x) {
          DrawPixel(x, y2, color);
        }
      }

      // Vertical lines (if not clipped)

      if (x1 == minX) {
        for (int y = minY; y <= maxY; ++y) {
          DrawPixel(minX, y, color);
        }
      }
      if (x2 == maxX) {
        for (int y = minY; y <= maxY; ++y) {
          DrawPixel(maxX, y, color);
        }
      }
    }

    public void FillRect(float x1, float y1, float x2, float y2, byte color) {
      int x1Int = Mathf.RoundToInt(x1);
      int y1Int = Mathf.RoundToInt(y1);
      int x2Int = Mathf.RoundToInt(x2);
      int y2Int = Mathf.RoundToInt(y2);

      FillRectInt(x1Int, y1Int, x2Int, y2Int, color);
    }

    public void FillRectInt(int x1, int y1, int x2, int y2, byte color) {
      // make sure our points are in order (TL to BR)
      if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
      }
      if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
      }

      // If erasing (color==0), then expand the fill rectangle up/down to cover the matching pixel
      // Otherwise the other row will be undone when DoubleUpRowPixels is called later on.
      if (color == 0) {
        if ((y1 % 2) != 0) {
          // Move the top up 1 pixel, otherwise that pixel will also control the destination image
          --y1;
        }
        if ((y2 % 2) == 0) {
          // Move the bottom down 1 pixel, otherwise that pixel will also control the destination image
          ++y2;
        }
      }

      // Clamp to screen
      x1 = Math.Max(x1, 0);
      x2 = Math.Min(x2, kFaceTextureWidth - 1);
      y1 = Math.Max(y1, 0);
      y2 = Math.Min(y2, kFaceTextureHeight - 1);

      for (int y = y1; y <= y2; ++y) {
        int dI = x1 + (y * kFaceTextureWidth);
        for (int x = x1; x <= x2; ++x) {
          _DrawToFacePixels[dI++] = color;
        }
      }
    }

    private void DoubleUpRowPixels() {
      // Double up the vertical pixels to counter the effect of being interlaced when displayed on the robot
      // (so the image looks the same regardless of if it's even or odd rows that are removed)
      int i = 0;
      int rowOffset = kFaceTextureWidth;
      for (int y = 0; y < kFaceTextureHeight; ++y) {
        for (int x = 0; x < kFaceTextureWidth; ++x) {
          _DrawToFacePixels[i] |= _DrawToFacePixels[i + rowOffset];
          ++i;
        }
        rowOffset = (rowOffset == kFaceTextureWidth) ? -kFaceTextureWidth : kFaceTextureWidth;
      }
    }

    private void PackFaceTextureToBitArray() {
      // Converts the source from:
      // _DrawToFacePixels (one byte per pixel)
      // to destination:
      // _DrawToFaceBytes (one bit per pixel)

      // Double up the vertical pixels to counter the effect of being interlaced when displayed on the robot
      DoubleUpRowPixels();

      int destIndex = 0;
      int numPendingPixels = 0;
      byte pendingPixels = 0; // Stores 8 pixel bits
      for (int srcIndex = 0; srcIndex < _DrawToFacePixels.Length; ++srcIndex) {
        byte srcPixel = _DrawToFacePixels[srcIndex];

        // Add each pixel as 1 bit, 1 times, to pixelByte
        // For every 8 of these, write out the pixelByte
        pendingPixels <<= 1;
        pendingPixels |= srcPixel;
        numPendingPixels += 1;
        // If 8 pixels are ready, write them out in 1 byte
        if (numPendingPixels == 8) {
          _DrawToFaceBytes[destIndex++] = pendingPixels;
          pendingPixels = 0;
          numPendingPixels = 0;
        }
      }
    }

    public void Display() {
      uint duration_ms = 1000 * 30;  // 30s is the longest engine will play this action (to avoid burn-in)
      var robot = RobotEngineManager.Instance.CurrentRobot;
      PackFaceTextureToBitArray();
      robot.DisplayFaceImage(duration_ms, _DrawToFaceBytes, queueActionPosition: QueueActionPosition.IN_PARALLEL);
    }

  }
}
