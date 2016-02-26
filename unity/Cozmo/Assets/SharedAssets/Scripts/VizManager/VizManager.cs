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

    private class VizUdpChannel : UdpChannel<MessageVizWrapper, MessageVizWrapper> { }

    private VizUdpChannel _Channel = null;

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


    [SerializeField]
    private VizLabel _VizLabelPrefab;

    [SerializeField]
    private VizQuad _VizQuadPrefab;

    [SerializeField]
    private VizPath _VizPathPrefab;

    [SerializeField]
    private Camera _Camera;

    [SerializeField]
    private RectTransform _LabelCanvas;


    [SerializeField]
    private Transform _VizScene;

    private bool _ShowingObjects = true;

    private readonly Dictionary<uint, Dictionary<uint, VizQuad>> _Quads = new Dictionary<uint, Dictionary<uint, VizQuad>>();
    private readonly Dictionary<uint, VizQuad> _Objects = new Dictionary<uint, VizQuad>();
    private readonly Dictionary<uint, VizLabel> _ObjectLabels = new Dictionary<uint, VizLabel>();
    private readonly Dictionary<string, VizQuad> _SimpleQuadVectors = new Dictionary<string, VizQuad>();

    private readonly Dictionary<uint, VizPath> _Paths = new Dictionary<uint, VizPath>();
    private readonly Dictionary<string, VizPath> _SegmentPrimatives = new Dictionary<string, VizPath>();

    // temp list used for clearing ranges of items.
    private readonly List<uint> _TmpList = new List<uint>();

    public float[] Emotions { get; private set; }
    public string[] RecentMoodEvents { get; private set; }
    public string Behavior { get; private set; }
    public BehaviorScoreData[] BehaviorScoreData { get; private set; }
    public RobotInterface.AnimationState AnimationState { get; private set; }

    public Camera Camera { get { return _Camera; } }

    private Vector3 _InitialCameraPosition;
    private Quaternion _InitialCameraRotation;

    private MemoryStream _MemStream = new MemoryStream();
    // required for Minimized Jpeg
    private MemoryStream _MemStream2 = new MemoryStream();
      
    public static VizManager Instance { get; private set; }
      
    private void OnEnable() {
      
      DAS.Info(this, "Enabling VizManager");
      if (Instance != null && Instance != this) {
        Destroy(gameObject);
        return;
      }
      else {
        Instance = this;
        DontDestroyOnLoad(gameObject);
      }
        
      _Channel = new VizUdpChannel();
      _Channel.ConnectedToClient += Connected;
      _Channel.DisconnectedFromClient += Disconnected;
      _Channel.MessageReceived += ReceivedMessage;

      _InitialCameraPosition = _Camera.transform.localPosition;
      _InitialCameraRotation = _Camera.transform.localRotation;

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

      ClearAllVizObjects();

      // Reconnect
      _Channel.Connect(_UIDeviceID, _UILocalPort);
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
        DrawCameraLine(message.CameraLine);
        break;
      case MessageViz.Tag.CameraOval:
        DrawCameraOval(message.CameraOval);
        break;
      case MessageViz.Tag.CameraQuad:
        DrawCameraQuad(message.CameraQuad);
        break;
      case MessageViz.Tag.CameraText:
        // This will require magic
        break;
      case MessageViz.Tag.VisionMarker:
        DrawVisionMarker(message.VisionMarker);
        break;
      case MessageViz.Tag.FaceDetection:
        DrawFaceDetection(message.FaceDetection);
        break;

      // World Drawing
      case MessageViz.Tag.Quad:
        DrawQuad(message.Quad);
        break;
      case MessageViz.Tag.EraseQuad:
        EraseQuad(message.EraseQuad);
        break;

      case MessageViz.Tag.Object:
        DrawObject(message.Object);
        break;
      case MessageViz.Tag.EraseObject:
        EraseObject(message.EraseObject);
        break;
      case MessageViz.Tag.ShowObjects:
        ShowObjects(message.ShowObjects);
        break;

      case MessageViz.Tag.SimpleQuadVectorMessageBegin:
        BeginSimpleQuadVector(message.SimpleQuadVectorMessageBegin);
        break;
      case MessageViz.Tag.SimpleQuadVectorMessage:
        SimpleQuadVector(message.SimpleQuadVectorMessage);
        break;
      case MessageViz.Tag.SimpleQuadVectorMessageEnd:
        EndSimpleQuadVector(message.SimpleQuadVectorMessageEnd);
        break;

      case MessageViz.Tag.AppendPathSegmentArc:
        AppendPathSegmentArc(message.AppendPathSegmentArc);
        break;
      case MessageViz.Tag.AppendPathSegmentLine:
        AppendPathSegmentLine(message.AppendPathSegmentLine);
        break;
      case MessageViz.Tag.SetPathColor:
        SetPathColor(message.SetPathColor);
        break;
      case MessageViz.Tag.ErasePath:
        ErasePath(message.ErasePath);
        break;

      case MessageViz.Tag.SegmentPrimitive:
        DrawSegmentPrimative(message.SegmentPrimitive);
        break;
      case MessageViz.Tag.EraseSegmentPrimitives:
        EraseSegmentPrimitives(message.EraseSegmentPrimitives);
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

        // TODO: None of the following are implemented
        // Not sure which ones we actually use
      case MessageViz.Tag.DefineColor:
      case MessageViz.Tag.DockingErrorSignal:
      case MessageViz.Tag.RobotStateMessage:
      case MessageViz.Tag.SetLabel:
      case MessageViz.Tag.SetRobot:
      case MessageViz.Tag.SetVizOrigin:
      case MessageViz.Tag.StartRobotUpdate:
      case MessageViz.Tag.EndRobotUpdate:
      case MessageViz.Tag.TrackerQuad:
        break;
      }
    }

    #region WorldDrawing


    public void ResetCamera() {
      _Camera.transform.localPosition = _InitialCameraPosition;
      _Camera.transform.localRotation = _InitialCameraRotation;
    }


    private void ClearAllVizObjects() {
      foreach (var dict in _Quads.Values) {
        EraseItem(dict, (uint)VizConstants.ALL_OBJECT_IDs);
      }
      EraseItem(_Objects, (uint)VizConstants.ALL_OBJECT_IDs);
      EraseItem(_ObjectLabels, (uint)VizConstants.ALL_OBJECT_IDs);
      EraseItem(_Paths, (uint)VizConstants.ALL_OBJECT_IDs);

      foreach (var obj in _SegmentPrimatives.Values) {
        Destroy(obj.gameObject);
      }
      _SegmentPrimatives.Clear();

      foreach (var obj in _SimpleQuadVectors.Values) {
        Destroy(obj.gameObject);
      }
      _SimpleQuadVectors.Clear();

    }

    // Shared Erase Function.
    public void EraseItem<T>(Dictionary<uint, T> dict, uint id, uint lower_bound_id = 0, uint upper_bound_id = 0) where T : Component {

      switch (id) {
      case (uint)VizConstants.ALL_OBJECT_IDs:
        foreach (var obj in dict.Values) {
          Destroy(obj.gameObject);
        }
        dict.Clear();
        break;
      case (uint)VizConstants.OBJECT_ID_RANGE:
        _TmpList.Clear();
        foreach (var kvp in dict) {
          if (kvp.Key >= lower_bound_id && kvp.Key < upper_bound_id) {
            Destroy(kvp.Value.gameObject);
            _TmpList.Add(kvp.Key);
          }
        }
        foreach (var key in _TmpList) {
          dict.Remove(key);
        }
        break;
      default:
        T vizObj;
        if (dict.TryGetValue(id, out vizObj)) {
          dict.Remove(id);
          Destroy(vizObj.gameObject);
        }
        break;
      }
    }

    public void DrawQuad(Quad quad) {
      VizQuad vizQuad;
      Dictionary<uint, VizQuad> quadDict;
      if (!_Quads.TryGetValue((uint)quad.quadType, out quadDict)) {
        quadDict = new Dictionary<uint, VizQuad>();
        _Quads.Add((uint)quad.quadType, quadDict);
      }

      if (!quadDict.TryGetValue(quad.quadID, out vizQuad)) {
        vizQuad = UIManager.CreateUIElement(_VizQuadPrefab, _VizScene).GetComponent<VizQuad>();
        vizQuad.Initialize("Quad_"+quad.quadID.ToString());
        quadDict.Add(quad.quadID, vizQuad);
      }

      var a = new Vector3(quad.xUpperLeft, quad.yUpperLeft, quad.zUpperLeft);
      var b = new Vector3(quad.xUpperRight, quad.yUpperRight, quad.zUpperRight);
      var c = new Vector3(quad.xLowerRight, quad.yLowerRight, quad.zLowerRight);
      var d = new Vector3(quad.xLowerLeft, quad.yLowerLeft, quad.zLowerLeft);

      // lets move the position of this vizQuad to the center of our points
      // and adjust the points accordingly.

      var center = (a + b + c + d) * 0.25f;

      vizQuad.UpdateQuad(
        a - center,
        b - center,
        c - center,
        d - center,
        Color.clear,
        quad.color.ToColor());
      vizQuad.transform.localPosition = center;
    }

    public void EraseQuad(EraseQuad eraseQuad) {

      if (eraseQuad.quadType == (uint)VizConstants.ALL_QUAD_TYPEs) {
        // Erase all quads
        foreach (var dict in _Quads.Values) {
          EraseItem(dict, (uint)VizConstants.ALL_QUAD_IDs);
        }
      }
      else {
        // Erase the specific quad
        Dictionary<uint, VizQuad> quadDict;
        if (_Quads.TryGetValue(eraseQuad.quadType, out quadDict)) {
          EraseItem(quadDict, eraseQuad.quadID);
        }
      }
    }

    public void DrawObject(Anki.Cozmo.VizInterface.Object obj) {
      VizQuad vizQuad;
      if (!_Objects.TryGetValue(obj.objectID, out vizQuad)) {
        vizQuad = UIManager.CreateUIElement(_VizQuadPrefab, _VizScene).GetComponent<VizQuad>();
        vizQuad.Initialize("Object_" + obj.objectID.ToString());
        vizQuad.gameObject.SetActive(_ShowingObjects);
        _Objects.Add(obj.objectID, vizQuad);

        var label = UIManager.CreateUIElement(_VizLabelPrefab, _LabelCanvas).GetComponent<VizLabel>();
        label.Text = obj.objectID == 1 ? "" : (obj.objectID % 1000).ToString();
        label.Initialize("ObjectLabel_" + obj.objectID.ToString(), _Camera, _VizScene);
        _ObjectLabels.Add(obj.objectID, label);
      }
      var vizLabel = _ObjectLabels[obj.objectID];

      var halfAngle = obj.rot_deg * Mathf.Deg2Rad * 0.5f;
      var sin = Mathf.Sin(halfAngle);
      var cos = Mathf.Cos(halfAngle);

      var rotation = new Quaternion(obj.rot_axis_x * sin, obj.rot_axis_y * sin, obj.rot_axis_z * sin, cos);
      var origin = new Vector3(obj.x_trans_m, obj.y_trans_m, obj.z_trans_m);
      var size = new Vector3(obj.x_size_m, obj.y_size_m, obj.z_size_m);
      var color = obj.color.ToColor();

      // set the position and rotation using transform, set size using mesh deformation
      vizQuad.transform.localRotation = rotation;
      vizQuad.transform.localPosition = origin;

      vizLabel.SetPosition(origin + rotation * (size * 0.5f));
      vizLabel.Color = color;

      if (vizQuad.DrawColor != color || !vizQuad.DrawScale.Approximately(size)) {
        switch (obj.objectTypeID) {
        case VizObjectType.VIZ_OBJECT_CUBOID:
          DrawCuboidObject(vizQuad, size, color);
          break;
        case VizObjectType.VIZ_OBJECT_ROBOT:
          DrawRobotObject(vizQuad, size, color);
          break;
        case VizObjectType.VIZ_OBJECT_HUMAN_HEAD:
          DrawHumanHeadObject(vizQuad, size, color);
          break;

        // TODO: Implement any other VizObjects that we don't make primitives
        default:
          DrawCuboidObject(vizQuad, size, color);
          break;
        }
        vizQuad.DrawColor = color;
        vizQuad.DrawScale = size;
      }
    }

    private void DrawCuboidObject(VizQuad vizQuad, Vector3 size, Color color) {
      var d = size * 0.5f;
      vizQuad.StartUpdateQuadList();

      var innerColor = Color.clear;
      var outerColor = color;

      vizQuad.StartUpdateQuadList();
      // front
      vizQuad.AddToQuadList(
        new Vector3(-d.x, d.y, d.z),
        new Vector3(d.x, d.y, d.z),
        new Vector3(d.x, -d.y, d.z),
        new Vector3(-d.x, -d.y, d.z),
        innerColor,
        outerColor);

      // back
      vizQuad.AddToQuadList(
        new Vector3(d.x, d.y, -d.z),
        new Vector3(-d.x, d.y, -d.z),
        new Vector3(-d.x, -d.y, -d.z),
        new Vector3(d.x, -d.y, -d.z),
        innerColor,
        outerColor);

      // right
      vizQuad.AddToQuadList(
        new Vector3(d.x, d.y, d.z),
        new Vector3(d.x, d.y, -d.z),
        new Vector3(d.x, -d.y, -d.z),
        new Vector3(d.x, -d.y, d.z),
        innerColor,
        outerColor);

      // left
      vizQuad.AddToQuadList(
        new Vector3(-d.x, d.y, -d.z),
        new Vector3(-d.x, d.y, d.z),
        new Vector3(-d.x, -d.y, d.z),
        new Vector3(-d.x, -d.y, -d.z),
        innerColor,
        outerColor);

      // top
      vizQuad.AddToQuadList(
        new Vector3(-d.x, d.y, -d.z),
        new Vector3(d.x, d.y, -d.z),
        new Vector3(d.x, d.y, d.z),
        new Vector3(-d.x, d.y, d.z),
        innerColor,
        outerColor);

      // bottom
      vizQuad.AddToQuadList(
        new Vector3(d.x, -d.y, -d.z),
        new Vector3(-d.x, -d.y, -d.z),
        new Vector3(-d.x, -d.y, d.z),
        new Vector3(d.x, -d.y, d.z),
        innerColor,
        outerColor);

      vizQuad.EndUpdateQuadList();
    }

    private void DrawRobotObject(VizQuad vizQuad, Vector3 size, Color color) {
      var innerColor = color;
      var outerColor = Color.clear;

      var heightOffset = Vector3.forward * CozmoUtil.kHeadHeightMM * 0.001f;

      vizQuad.StartUpdateQuadList();
      // top
      vizQuad.AddToQuadList(
        Vector3.right * 0.02f + heightOffset,
        Vector3.right * 0.02f + heightOffset,
        new Vector3(-0.01f, -0.01f, 0.01f) + heightOffset,
        new Vector3(-0.01f, 0.01f, 0.01f) + heightOffset, 
        innerColor,
        outerColor);

      // right
      vizQuad.AddToQuadList(
        Vector3.right * 0.02f + heightOffset,
        Vector3.right * 0.02f + heightOffset,
        new Vector3(-0.01f, 0.01f, 0.01f) + heightOffset,
        new Vector3(-0.01f, 0.01f, -0.01f) + heightOffset, 
        innerColor,
        outerColor);

      // bottom
      vizQuad.AddToQuadList(
        Vector3.right * 0.02f + heightOffset,
        Vector3.right * 0.02f + heightOffset,
        new Vector3(-0.01f, 0.01f, -0.01f) + heightOffset,
        new Vector3(-0.01f, -0.01f, -0.01f) + heightOffset, 
        innerColor,
        outerColor);

      // left
      vizQuad.AddToQuadList(
        Vector3.right * 0.02f + heightOffset,
        Vector3.right * 0.02f + heightOffset,
        new Vector3(-0.01f, -0.01f, -0.01f) + heightOffset,
        new Vector3(-0.01f, -0.01f, 0.01f) + heightOffset, 
        innerColor,
        outerColor);

      // back
      vizQuad.AddToQuadList(
        new Vector3(-0.01f, -0.01f, 0.01f) + heightOffset, 
        new Vector3(-0.01f, 0.01f, 0.01f) + heightOffset, 
        new Vector3(-0.01f, 0.01f, -0.01f) + heightOffset,
        new Vector3(-0.01f, -0.01f, -0.01f) + heightOffset, 
        innerColor,
        outerColor);

      vizQuad.EndUpdateQuadList();
    }

    private void DrawHumanHeadObject(VizQuad vizQuad, Vector3 size, Color color) {
      var innerColor = Color.clear;
      var outerColor = color;

      vizQuad.StartUpdateQuadList();

      var d = size * 0.5f;

      Vector2 lastXZ = new Vector2(d.x, 0);

      //Draw the head as a huge oval in space.
      for (int i = 1; i <= 18; i++) {
        float angle = Mathf.PI * i / 9f;

        Vector2 nextXZ = new Vector2(d.x * Mathf.Cos(angle), d.z * Mathf.Sin(angle));

        vizQuad.AddToQuadList(
          new Vector3(nextXZ.x, -d.y, nextXZ.y),
          new Vector3(nextXZ.x, d.y, nextXZ.y),
          new Vector3(lastXZ.x, d.y, lastXZ.y),
          new Vector3(lastXZ.x, -d.y, lastXZ.y),
          innerColor,
          outerColor);
        lastXZ = nextXZ;
      }

      vizQuad.EndUpdateQuadList();
    }


    public void EraseObject(EraseObject eraseObj) {
      EraseItem(_Objects, eraseObj.objectID, eraseObj.lower_bound_id, eraseObj.upper_bound_id);
      EraseItem(_ObjectLabels, eraseObj.objectID, eraseObj.lower_bound_id, eraseObj.upper_bound_id);
    }

    public void ShowObjects(ShowObjects showObjects){
      
      _ShowingObjects = (showObjects.show == 0 ? false : true);
      foreach (var obj in _Objects.Values) {
        obj.gameObject.SetActive(_ShowingObjects);
      }
      foreach (var obj in _ObjectLabels.Values) {
        obj.gameObject.SetActive(_ShowingObjects);
      }
    }

    public void BeginSimpleQuadVector(SimpleQuadVectorMessageBegin begin) {
      VizQuad vizQuad;
      if (!_SimpleQuadVectors.TryGetValue(begin.identifier, out vizQuad)) {
        vizQuad = UIManager.CreateUIElement(_VizQuadPrefab, _VizScene).GetComponent<VizQuad>();
        vizQuad.Initialize("SimpleQuadVector_" + begin.identifier);
        _SimpleQuadVectors.Add(begin.identifier, vizQuad);
      }
      vizQuad.StartUpdateQuadList();
    }

    public void SimpleQuadVector(SimpleQuadVectorMessage msg) {
      VizQuad vizQuad;
      if (!_SimpleQuadVectors.TryGetValue(msg.identifier, out vizQuad)) {
        DAS.Error(this, "Could not find SimpleQuadVector with identifier " + msg.identifier);
        return;
      }
      foreach (var quad in msg.quads) {
        Vector3 center = new Vector3(quad.center[0], quad.center[1], quad.center[2]);
        float r = quad.sideSize * 0.5f;

        var a = center + new Vector3(-r, r, 0);
        var b = center + new Vector3(r, r, 0);
        var c = center + new Vector3(r, -r, 0);
        var d = center + new Vector3(-r, -r, 0);

        // lets move the position of this vizQuad to the center of our points
        // and adjust the points accordingly.

        vizQuad.AddToQuadList(
          a,
          b,
          c,
          d,
          quad.color.ToColor(),
          Color.clear);
      }
    }

    public void EndSimpleQuadVector(SimpleQuadVectorMessageEnd end) {
      VizQuad vizQuad;
      if (!_SimpleQuadVectors.TryGetValue(end.identifier, out vizQuad)) {
        DAS.Error(this, "Could not find SimpleQuadVector with identifier " + end.identifier);
        return;
      }
      vizQuad.EndUpdateQuadList();
    }

    public void AppendPathSegmentLine(AppendPathSegmentLine line) {
      VizPath vizPath;
      if (!_Paths.TryGetValue(line.pathID, out vizPath)) {
        vizPath = UIManager.CreateUIElement(_VizPathPrefab, _VizScene).GetComponent<VizPath>();
        vizPath.Initialize("Path_" + line.pathID);
        _Paths.Add(line.pathID, vizPath);
        vizPath.BeginPath();
      }
      vizPath.AppendLine(
        new Vector3(line.x_start_m, line.y_start_m, line.z_start_m), 
        new Vector3(line.x_end_m, line.y_end_m, line.z_end_m));
    }

    public void AppendPathSegmentArc(AppendPathSegmentArc arc) {
      VizPath vizPath;
      if (!_Paths.TryGetValue(arc.pathID, out vizPath)) {
        vizPath = UIManager.CreateUIElement(_VizPathPrefab, _VizScene).GetComponent<VizPath>();
        vizPath.Initialize("Path_" + arc.pathID);
        _Paths.Add(arc.pathID, vizPath);
        vizPath.BeginPath();
      }
      vizPath.AppendArc(
        new Vector2(arc.x_center_m, arc.y_center_m), 
        arc.radius_m, 
        arc.start_rad, 
        arc.sweep_rad);
    }

    public void SetPathColor(SetPathColor path) {
      VizPath vizPath;
      if (!_Paths.TryGetValue(path.pathID, out vizPath)) {
        DAS.Warn(this, "Could not set color for path " + path.pathID);
        return;
      }
      // Not sure if we are using the color as raw color, as a lookup
      vizPath.Color = path.colorID.ToColor();
    }

    public void ErasePath(ErasePath path) {
      EraseItem(_Paths, path.pathID);
    }

    public void DrawSegmentPrimative(SegmentPrimitive segment) {
      VizPath vizPath;
      if (!_SegmentPrimatives.TryGetValue(segment.identifier, out vizPath)) {
        vizPath = UIManager.CreateUIElement(_VizPathPrefab, _VizScene).GetComponent<VizPath>();
        vizPath.Initialize("SegmentPrimative_" + segment.identifier);
        _SegmentPrimatives.Add(segment.identifier, vizPath);
      }
      var a = new Vector3(segment.origin[0], segment.origin[1], segment.origin[2]);
      var b = new Vector3(segment.dest[0], segment.dest[1], segment.dest[2]);
      if (segment.clearPrevious) {
        vizPath.SetSegment(a, b, segment.color.ToColor());
      }
      else {
        vizPath.AppendLine(a, b);
      }
    }

    public void EraseSegmentPrimitives(EraseSegmentPrimitives eraseSegment) {
      VizPath vizPath;
      if (_SegmentPrimatives.TryGetValue(eraseSegment.identifier, out vizPath)) {
        _SegmentPrimatives.Remove(eraseSegment.identifier);
        Destroy(vizPath.gameObject);
      }
    }

    #endregion // WorldDrawing

    #region OverlayRender

    private void ClearOverlay() {
      if (_OverlayImage == null) {
        return;
      }
      if (_OverlayClearBuffer == null) {
        _OverlayClearBuffer = new Color32[_OverlayImage.width * _OverlayImage.height];
      }

      _OverlayImage.SetPixels32(_OverlayClearBuffer);
    }
    private void DrawCameraLine(CameraLine camLine) {

      DrawOverlayLine(camLine.xStart, camLine.yStart, camLine.xEnd, camLine.yEnd, camLine.color.ToColor());
    }

    private void DrawCameraOval(CameraOval camOval) {
      DrawOverlayOval(camOval.xCen, camOval.yCen, camOval.xRad, camOval.yRad, camOval.color.ToColor());
    }

    private void DrawCameraQuad(CameraQuad camQuad) {
      DrawOverlayLine(camQuad.xLowerLeft, camQuad.yLowerLeft, camQuad.xLowerRight, camQuad.yLowerRight, camQuad.color.ToColor());
      DrawOverlayLine(camQuad.xLowerLeft, camQuad.yLowerLeft, camQuad.xUpperLeft, camQuad.yUpperLeft, camQuad.color.ToColor());
      DrawOverlayLine(camQuad.xUpperRight, camQuad.yUpperRight, camQuad.xLowerRight, camQuad.yLowerRight, camQuad.color.ToColor());
      DrawOverlayLine(camQuad.xUpperRight, camQuad.yUpperRight, camQuad.xUpperLeft, camQuad.yUpperLeft, camQuad.color.ToColor());
    }

    private void DrawVisionMarker(VisionMarker visionMarker) {
      DrawOverlayLine(visionMarker.bottomLeft_x, visionMarker.bottomLeft_y, visionMarker.bottomRight_x, visionMarker.bottomRight_y, Color.red);
      DrawOverlayLine(visionMarker.bottomLeft_x, visionMarker.bottomLeft_y, visionMarker.topLeft_x, visionMarker.topLeft_y, Color.red);
      DrawOverlayLine(visionMarker.topRight_x, visionMarker.topRight_y, visionMarker.bottomRight_x, visionMarker.bottomRight_y, Color.red);
      DrawOverlayLine(visionMarker.topRight_x, visionMarker.topRight_y, visionMarker.topLeft_x, visionMarker.topLeft_y, Color.red);
    }

    private void DrawFaceDetection(FaceDetection faceDetection) {      
      DrawOverlayOval((faceDetection.x_upperLeft + 0.5f * faceDetection.width), (faceDetection.y_upperLeft + 0.5f * faceDetection.height), (0.5f * faceDetection.width), (0.5f * faceDetection.height), Color.blue);
    }


    private void DrawOverlayLine(float x0f, float y0f, float x1f, float y1f, Color col) {

      if (_OverlayImage == null) {
        return;
      }

      int h = _OverlayImage.height;
      // invert our y
      y0f = h - y0f - 1;
      y1f = h - y1f - 1;

      int x0 = (int)(x0f - 0.5f);
      int y0 = (int)(y0f - 0.5f);
      int x1 = (int)(x1f - 0.5f);
      int y1 = (int)(y1f - 0.5f);


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

    private void DrawOverlayOval(float cxf, float cyf, float rx, float ry, Color col) {

      if (_OverlayImage == null) {
        return;
      }

      int h = _OverlayImage.height;
      // invert our y
      cyf = h - cyf - 1;

      int cx = (int)(cxf - 0.5f);
      int cy = (int)(cyf - 0.5f);

      var y = Mathf.Max(rx, ry);

      if (y == 0) {
        SetOverlayPixel(cx, cy, col);
        return;
      }

      float xratio = rx / y;
      float yratio = ry / y;

      var d = 0.25f - y;
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

    #endregion // OverlayRender
    #region ImageChunk


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
    #endregion // ImageChunk
  }
}

