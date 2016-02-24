using System;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo.VizInterface;
using System.IO;

namespace Anki.Cozmo {
  public class VizManager : MonoBehaviour {

    private class MessageVizWrapper : IMessageWrapper {
      public readonly MessageViz Message = new MessageViz();

      #region IMessageWrapper implementation

      public void Unpack(System.IO.Stream stream) {
        Message.Unpack(stream);
      }

      public void Unpack(System.IO.BinaryReader reader) {
        Message.Unpack(reader);
      }

      public void Pack(System.IO.Stream stream) {
        Message.Pack(stream);
      }

      public void Pack(System.IO.BinaryWriter writer) {
        Message.Pack(writer);
      }

      public string GetTag() {
        return Message.GetTag().ToString();
      }

      public int Size {
        get {
          return Message.Size;
        }
      }

      public bool IsValid { 
        get {
          return Message.GetTag() != MessageViz.Tag.INVALID;
        }
      }

      #endregion
    }

    private ChannelBase<MessageVizWrapper, MessageVizWrapper> _Channel = null;

    private DisconnectionReason _LastDisconnectionReason;

    public event Action<string> ConnectedToClient;
    public event Action<DisconnectionReason> DisconnectedFromClient;

    private const int _UIDeviceID = 1;
    // 0 means random unused port
    // Used to be 5106
    private const int _UILocalPort = 5352;

    private Texture2D _ReceivedImage;
    public Texture2D RobotCameraImage { get { return _ReceivedImage; } }

    private bool _OverlayDirty;
    private Texture2D _OverlayImage;
    public Texture2D RobotCameraOverlay { get { return _OverlayImage; } }
    private Color32[] _OverlayClearBuffer = null;


    public float[] Emotions { get; private set; }
    public string[] RecentMoodEvents { get; private set; }
    public string Behavior { get; private set; }
    public BehaviorScoreData[] BehaviorScoreData { get; private set; }
    public RobotInterface.AnimationState AnimationState { get; private set; }


    private MemoryStream _MemStream = new MemoryStream();
    // required for Minimized Jpeg
    private MemoryStream _MemStream2 = new MemoryStream();

  
    public static VizManager Instance { get; private set; }
      
    private void OnEnable() {
      DAS.Info("VizManager", "Enabling Robot Engine Manager");
      if (Instance != null && Instance != this) {
        Destroy(gameObject);
        return;
      }
      else {
        Instance = this;
        DontDestroyOnLoad(gameObject);
      }
        
      _Channel = new UdpChannel<MessageVizWrapper, MessageVizWrapper>();
      _Channel.ConnectedToClient += Connected;
      _Channel.DisconnectedFromClient += Disconnected;
      _Channel.MessageReceived += ReceivedMessage;

      Listen();
    }

    private void Update() {
      ClearOverlay();
      _Channel.Update();
      if (_OverlayImage != null && _OverlayDirty) {
        _OverlayImage.Apply();
        _OverlayDirty = false;
      }
    }
  
    public void Listen() {
      _Channel.Connect(_UIDeviceID, _UILocalPort);
    }

    public void Disconnect() {
      if (_Channel != null) {
        _Channel.Disconnect();

        // only really needed in editor in case unhitting play
        #if UNITY_EDITOR
        float limit = Time.realtimeSinceStartup + 2.0f;
        while (_Channel.HasPendingOperations) {
          if (limit < Time.realtimeSinceStartup) {
            DAS.Warn("VizManager", "Not waiting for disconnect to finish sending.");
            break;
          }
          System.Threading.Thread.Sleep(500);
        }
        #endif
      }
    }

    public DisconnectionReason GetLastDisconnectionReason() {
      DisconnectionReason reason = _LastDisconnectionReason;
      _LastDisconnectionReason = DisconnectionReason.None;
      return reason;
    }

    private void Connected(string connectionIdentifier) {
      if (ConnectedToClient != null) {
        ConnectedToClient(connectionIdentifier);
      }
    }

    private void Disconnected(DisconnectionReason reason) {
      DAS.Debug("VizManager", "Disconnected: " + reason.ToString());

      _LastDisconnectionReason = reason;
      if (DisconnectedFromClient != null) {
        DisconnectedFromClient(reason);
      }
    }

    private void ReceivedMessage(MessageVizWrapper messageWrapper) {
      var message = messageWrapper.Message;

      switch (message.GetTag()) {
      // Camera Image
      case MessageViz.Tag.ImageChunk:
        ProcessImageChunk(message.ImageChunk);
        break;
      //Camera Overlay
      case MessageViz.Tag.CameraLine:
        var camLine = message.CameraLine;
        DrawOverlayLine((int)(camLine.xStart - 0.5f), (int)(camLine.yStart - 0.5f), (int)(camLine.xEnd - 0.5f), (int)(camLine.yEnd - 0.5f), camLine.color.ToColor());
        break;
      case MessageViz.Tag.CameraOval:
        var camOval = message.CameraOval;
        DrawOverlayOval((int)(camOval.xCen - 0.5f), (int)(camOval.yCen - 0.5f), (int)(camOval.xRad - 0.5f), (int)(camOval.yRad - 0.5f), camOval.color.ToColor());
        break;
      case MessageViz.Tag.CameraQuad:
        var camQuad = message.CameraQuad;
        DrawOverlayLine((int)(camQuad.xLowerLeft - 0.5f), (int)(camQuad.yLowerLeft - 0.5f), (int)(camQuad.xLowerRight - 0.5f), (int)(camQuad.yLowerRight - 0.5f), camQuad.color.ToColor());
        DrawOverlayLine((int)(camQuad.xLowerLeft - 0.5f), (int)(camQuad.yLowerLeft - 0.5f), (int)(camQuad.xUpperLeft - 0.5f), (int)(camQuad.yUpperLeft - 0.5f), camQuad.color.ToColor());
        DrawOverlayLine((int)(camQuad.xUpperRight - 0.5f), (int)(camQuad.xUpperRight - 0.5f), (int)(camQuad.xLowerRight - 0.5f), (int)(camQuad.yLowerRight - 0.5f), camQuad.color.ToColor());
        DrawOverlayLine((int)(camQuad.xUpperRight - 0.5f), (int)(camQuad.xUpperRight - 0.5f), (int)(camQuad.xUpperLeft - 0.5f), (int)(camQuad.yUpperLeft - 0.5f), camQuad.color.ToColor());
        break;
      case MessageViz.Tag.CameraText:
        // This will require magic
        break;
      case MessageViz.Tag.VisionMarker:
        var visionMarker = message.VisionMarker;
        DrawOverlayLine((int)visionMarker.bottomLeft_x, (int)visionMarker.bottomLeft_y, (int)visionMarker.bottomRight_x, (int)visionMarker.bottomRight_y, Color.red);
        DrawOverlayLine((int)visionMarker.bottomLeft_x, (int)visionMarker.bottomLeft_y, (int)visionMarker.topLeft_x, (int)visionMarker.topLeft_y, Color.red);
        DrawOverlayLine((int)visionMarker.topRight_x, (int)visionMarker.topRight_y, (int)visionMarker.bottomRight_x, (int)visionMarker.bottomRight_y, Color.red);
        DrawOverlayLine((int)visionMarker.topRight_x, (int)visionMarker.topRight_y, (int)visionMarker.topLeft_x, (int)visionMarker.topLeft_y, Color.red);

        break;
      case MessageViz.Tag.FaceDetection:
        var faceDetection = message.FaceDetection;
        DrawOverlayOval((int)(faceDetection.x_upperLeft + 0.5f + 0.5f * faceDetection.width), (int)(faceDetection.y_upperLeft + 0.5f + 0.5f * faceDetection.height), (int)(0.5f * faceDetection.width), (int)(0.5f * faceDetection.height), Color.green);
        break;

      // World Drawing
      case MessageViz.Tag.Quad:

        break;
      // Information
      case MessageViz.Tag.RobotMood:
        Emotions = message.RobotMood.emotion;
        RecentMoodEvents = message.RobotMood.recentEvents;
        break;
      case MessageViz.Tag.NewBehaviorSelected:
        Behavior = message.NewBehaviorSelected.newCurrentBehavior;
        break;
      case MessageViz.Tag.RobotBehaviorSelectData:
        BehaviorScoreData = message.RobotBehaviorSelectData.scoreData;
        break;
      case MessageViz.Tag.AnimationState:
        AnimationState = message.AnimationState;
        break;
      }
    }

    private void ClearOverlay() {
      if (_OverlayImage == null) {
        return;
      }
      if (_OverlayClearBuffer == null) {
        _OverlayClearBuffer = new Color32[_OverlayImage.width * _OverlayImage.height];
      }

      _OverlayImage.SetPixels32(_OverlayClearBuffer);
    }

    private void DrawOverlayLine(int x0, int y0, int x1, int y1, Color col) {
      if (_OverlayImage == null) {
        return;
      }
      int h = _OverlayImage.height;
      // invert our y
      y0 = h - y0 - 1;
      y1 = h - y1 - 1;

      // Bresenham's algorithm, taken from here: http://wiki.unity3d.com/index.php?title=TextureDrawLine
      var dy = y1 - y0;
      var dx = x1 - x0;

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

      SetOverlayPixel(x0, y0, col);
      if (dx > dy) 
      {
        var fraction = dy - (dx >> 1);
        while (x0 != x1) {
          if (fraction >= 0) {
            y0 += stepy;
            fraction -= dx;
          }
          x0 += stepx;
          fraction += dy;
          SetOverlayPixel(x0, y0, col);
        }
      }
      else {
        var fraction = dx - (dy >> 1);
        while (y0 != y1) {
          if (fraction >= 0) {
            x0 += stepx;
            fraction -= dy;
          }
          y0 += stepy;
          fraction += dx;
          SetOverlayPixel(x0, y0, col);
        }
      }

      _OverlayDirty = true;
    }

    private void DrawOverlayOval(int cx, int cy, int rx, int ry, Color col) {
      if (_OverlayImage == null) {
        return;
      }

      var y = Mathf.Max(rx, ry);
      float xratio = rx / y;
      float yratio = ry / y;

      var d = 1/4 - y;
      var end = Mathf.Ceil(y/Mathf.Sqrt(2));

      for (var x = 0; x <= end; x++) {
        int xdx = (int)(x * xratio);
        int xdy = (int)(y * xratio);
        int ydx = (int)(x * yratio);
        int ydy = (int)(y * yratio);
        SetOverlayPixel(cx + xdx, cy + ydy, col);
        SetOverlayPixel(cx + xdx, cy - ydy, col);
        SetOverlayPixel(cx - xdx, cy + ydy, col);
        SetOverlayPixel(cx - xdx, cy - ydy, col);
        SetOverlayPixel(cx + xdy, cy + ydx, col);
        SetOverlayPixel(cx - xdy, cy + ydx, col);
        SetOverlayPixel(cx + xdy, cy - ydx, col);
        SetOverlayPixel(cx - xdy, cy - ydx, col);

        d += 2*x+1;
        if (d > 0) {
          d += 2 - 2*y--;
        }
      }
      _OverlayDirty = true;
    }

    // Actually set 4 pixels
    private void SetOverlayPixel(int x, int y, Color c) {
      
      _OverlayImage.SetPixel(x, y, c);
      _OverlayImage.SetPixel(x + 1, y, c);
      _OverlayImage.SetPixel(x, y + 1, c);
      _OverlayImage.SetPixel(x + 1, y + 1, c);
    }

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
    private static void MinimizedGreyToJpeg(MemoryStream bufferIn, MemoryStream bufferOut, int height, int width)
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
        DAS.Error("MinimizedGrayToJpeg", "No header for quality of %d", quality);
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
        bufferIn.Read(_OneByte,0,1);
        if(_OneByte[0] == 0xff) {
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
        bufferIn.Read(_OneByte, 0, 1);

        bufferOut.Write(_OneByte, 0, 1);
        if (_OneByte[0] == 0xff) {
          bufferOut.WriteByte(0);
        }
      }

      bufferOut.WriteByte(0xFF);
      bufferOut.WriteByte(0xD9);
    } // miniGrayToJpeg()


    private static readonly byte[] _OneByte = new byte[1];
    private static readonly byte[] _ThreeBytes = new byte[3];
    private void ProcessImageChunk(ImageChunk imageChunk) {


      if (imageChunk.chunkId == 0) {
        _MemStream.Seek(0, SeekOrigin.Begin);
        _MemStream.SetLength(0);
      }

      _MemStream.Write(imageChunk.data, 0, imageChunk.data.Length);

      if(imageChunk.chunkId == imageChunk.imageChunkCount - 1) {
        _OverlayDirty = true;

        var dims = ImageResolutionTable.GetDimensions(imageChunk.resolution);

        //UnityEngine.Debug.Log("Image ID: " + imageChunk.imageId + 
        //                      " Chunk: "+ imageChunk.chunkId + "/" + imageChunk.imageChunkCount + 
        //                      " Format: " +imageChunk.imageEncoding + 
        //                      " Resolution: " +imageChunk.resolution+"("+dims.Width+"x"+dims.Height+")");

        if (_ReceivedImage == null) {
          _ReceivedImage = new Texture2D(dims.Width, dims.Height);
          _ReceivedImage.name = "RobotCameraImage";
          _OverlayImage = new Texture2D(dims.Width, dims.Height);
          _OverlayImage.name = "RobotOverlayImage";
          ClearOverlay();
        }
          
        switch (imageChunk.imageEncoding) {
        case ImageEncoding.JPEGColor:
        case ImageEncoding.JPEGColorHalfWidth:
        case ImageEncoding.JPEGGray:
          _ReceivedImage.LoadImage(_MemStream.GetBuffer());
          break;
        case ImageEncoding.JPEGMinimizedGray:
          MinimizedGreyToJpeg(_MemStream, _MemStream2, dims.Height, dims.Width);
          _ReceivedImage.LoadImage(_MemStream2.GetBuffer());
          break;
        case ImageEncoding.RawGray:
          for (int i = 0; i < _MemStream.Length; i++) {
            _MemStream.Seek(0, SeekOrigin.Begin);
            _MemStream.Read(_OneByte, 0, 1);
            float c =  _OneByte[0] / 255f;
            _ReceivedImage.SetPixel(i % _ReceivedImage.width, i / _ReceivedImage.width, new Color(c, c, c, 1f));            
          }
          _ReceivedImage.Apply();
          break;
        case ImageEncoding.RawRGB:
          for (int i = 0; i < _MemStream.Length; i+=3) {
            _MemStream.Seek(0, SeekOrigin.Begin);
            _MemStream.Read(_ThreeBytes, 0, 3);
            float r =  _ThreeBytes[0] / 255f;
            float g =  _ThreeBytes[0] / 255f;
            float b =  _ThreeBytes[0] / 255f;
            _ReceivedImage.SetPixel(i % _ReceivedImage.width, i / _ReceivedImage.width, new Color(r, g, b, 1f));            
          }
          _ReceivedImage.Apply();
          break;
        }
      }
    }
  }
}

