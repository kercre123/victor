using System;
using System.IO;
using Anki.Cozmo;
using UnityEngine;
using Anki.Cozmo.ExternalInterface;

public class ImageReceiver : IDisposable {

  public event Action<Texture2D> OnImageReceived;
  public event Action<float, float> OnImageSizeChanged;

  private MemoryStream _MemStream = new MemoryStream();
  // required for Minimized Jpeg
  private MemoryStream _MemStream2 = new MemoryStream();

  private Texture2D _ReceivedImage;

  public Texture2D Image { get { return _ReceivedImage; } }

  public readonly string Name;

  public ImageReceiver(string name) {
    Name = name;
  }

  private ImageSendMode _SendMode = ImageSendMode.Off;

  public void CaptureSnapshot() {
    Initialize(ImageSendMode.SingleShot);
  }

  public void CaptureStream() {
    Initialize(ImageSendMode.Stream);
  }

  public void Initialize(ImageSendMode sendMode) {
    if (_SendMode == ImageSendMode.Off) {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ImageChunk>(ProcessImageChunk);
    }
    _SendMode = sendMode;
    RequestImage(_SendMode);
  }

  private void RequestImage(ImageSendMode sendMode) {
    RobotEngineManager.Instance.Message.ImageRequest =
      Singleton<ImageRequest>.Instance.Initialize((byte)RobotEngineManager.Instance.CurrentRobotID, sendMode);
    RobotEngineManager.Instance.SendMessage();
  }

  public void DestroyTexture() {
    if (_ReceivedImage != null) {
      Texture2D.Destroy(_ReceivedImage);
      _ReceivedImage = null;
    }
  }

  public void StopCapture() {
    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ImageChunk>(ProcessImageChunk);
    if (_SendMode != ImageSendMode.Off) {
      RequestImage(ImageSendMode.Off);
      _SendMode = ImageSendMode.Off;
    }
  }

  /// <summary>
  /// Does Not Destroy Texture! if you need to destroy the image call DestroyTexture.
  /// </summary>
  /// <remarks>Call <see cref="Dispose"/> when you are finished using the <see cref="ImageReceiver"/>. The <see cref="Dispose"/>
  /// method leaves the <see cref="ImageReceiver"/> in an unusable state. After calling <see cref="Dispose"/>, you must
  /// release all references to the <see cref="ImageReceiver"/> so the garbage collector can reclaim the memory that the
  /// <see cref="ImageReceiver"/> was occupying.</remarks>
  public void Dispose() {
    StopCapture();
    _MemStream.Dispose();
    _MemStream2.Dispose();
  }

  //allow manual processing
  public void ProcessImageChunk(ImageChunk imageChunk) {
    if (imageChunk.chunkId == 0) {
      _MemStream.Seek(0, SeekOrigin.Begin);
      _MemStream.SetLength(0);
    }

    var dims = ImageResolutionTable.GetDimensions(imageChunk.resolution);

    _MemStream.Write(imageChunk.data, 0, imageChunk.data.Length);

    if (imageChunk.chunkId == imageChunk.imageChunkCount - 1) {

      if (_ReceivedImage == null) {
        _ReceivedImage = new Texture2D(dims.Width, dims.Height);
        _ReceivedImage.name = Name;
        if (OnImageSizeChanged != null) {
          OnImageSizeChanged(dims.Width, dims.Height);
        }
      }

      switch (imageChunk.imageEncoding) {
      case ImageEncoding.JPEGColor:
      case ImageEncoding.JPEGColorHalfWidth:
      case ImageEncoding.JPEGGray:
        _ReceivedImage.LoadImage(_MemStream.GetBuffer());
        break;
      case ImageEncoding.JPEGMinimizedGray: // This is what the robot is sending
        ImageUtil.MinimizedGreyToJpeg(_MemStream, _MemStream2, dims.Height, dims.Width);
        _ReceivedImage.LoadImage(_MemStream2.GetBuffer());
        break;
      case ImageEncoding.RawGray:
        for (int i = 0; i < _MemStream.Length; i++) {
          _MemStream.Seek(0, SeekOrigin.Begin);
          _MemStream.Read(ImageUtil.OneByteBuffer, 0, 1);
          float c = ImageUtil.OneByteBuffer[0] / 255f;
          _ReceivedImage.SetPixel(i % _ReceivedImage.width, i / _ReceivedImage.width, new Color(c, c, c, 1f));
        }
        _ReceivedImage.Apply();
        break;
      case ImageEncoding.RawRGB:
        for (int i = 0; i < _MemStream.Length; i += 3) {
          _MemStream.Seek(0, SeekOrigin.Begin);
          _MemStream.Read(ImageUtil.ThreeByteBuffer, 0, 3);
          float r = ImageUtil.ThreeByteBuffer[0] / 255f;
          float g = ImageUtil.ThreeByteBuffer[0] / 255f;
          float b = ImageUtil.ThreeByteBuffer[0] / 255f;
          _ReceivedImage.SetPixel(i % _ReceivedImage.width, i / _ReceivedImage.width, new Color(r, g, b, 1f));
        }
        _ReceivedImage.Apply();
        break;
      }

      if (OnImageReceived != null) {
        OnImageReceived(_ReceivedImage);
      }
      if (_SendMode == ImageSendMode.SingleShot) {
        _SendMode = ImageSendMode.Off;
        StopCapture();
      }
    }
  }
}

