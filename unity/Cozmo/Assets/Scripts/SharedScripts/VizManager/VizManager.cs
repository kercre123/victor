#if !UNITY_EDITOR && (UNITY_IOS || UNITY_ANDROID)
#define VIZ_ON_DEVICE
#endif

using System;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo.VizInterface;
using System.IO;
using Cozmo.Util;

namespace Anki.Cozmo.Viz {
  public class VizManager : MonoBehaviour {

    private class VizUdpChannel : UdpChannel<MessageVizWrapper, MessageVizWrapper> {

    }

#if VIZ_ON_DEVICE
    private VizDirectChannel _Channel = null;
#else
    private VizUdpChannel _Channel = null;
#endif

    private DisconnectionReason _LastDisconnectionReason;

    public event Action<string> ConnectedToClient;
    public event Action<DisconnectionReason> DisconnectedFromClient;

    private const int _UIDeviceID = 1;
    // 0 means random unused port
    // Used to be 5106
    private const int _UILocalPort = 5352;

    public Texture2D RobotCameraImage { get { return _ImageReceiver.Image; } }

    private ImageReceiver _ImageReceiver = new ImageReceiver("RobotCameraImage");

    private bool _OverlayDirty;
    private Texture2D _OverlayImageWithFrame;

    public Texture2D RobotCameraOverlay { get { return _OverlayImageWithFrame; } }

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
    private Font _CameraFont;

    [SerializeField]
    private Transform _VizScene;

    private bool _ShowingObjects = true;

    // Whether or not we should render the Memory Map
    private bool _RenderMemoryMap = true;

    public bool RenderMemoryMap {
      get {
        return _RenderMemoryMap;
      }
      set {
        // Set new value
        _RenderMemoryMap = value;

        // Send engine message because it is the one doing render requests for the quads of the map
        Anki.Cozmo.ExternalInterface.SetMemoryMapRenderEnabled renderEnabledMsg = new Anki.Cozmo.ExternalInterface.SetMemoryMapRenderEnabled();
        renderEnabledMsg.enabled = value;
        RobotEngineManager.Instance.Message.SetMemoryMapRenderEnabled = renderEnabledMsg;
        RobotEngineManager.Instance.SendMessage();
      }
    }

    private readonly Dictionary<uint, Dictionary<uint, VizQuad>> _Quads = new Dictionary<uint, Dictionary<uint, VizQuad>>();
    private readonly Dictionary<uint, VizQuad> _Objects = new Dictionary<uint, VizQuad>();
    private readonly Dictionary<uint, VizLabel> _ObjectLabels = new Dictionary<uint, VizLabel>();
    private readonly Dictionary<string, VizQuad> _SimpleQuadVectors = new Dictionary<string, VizQuad>();
    private readonly Dictionary<uint, ExternalInterface.MemoryMapInfo> _MemoryMapInfos = new Dictionary<uint, ExternalInterface.MemoryMapInfo>();
    private readonly Dictionary<uint, List<ExternalInterface.MemoryMapQuadInfoDebugViz>> _QuadInfos = new Dictionary<uint, List<ExternalInterface.MemoryMapQuadInfoDebugViz>>();

    private readonly Dictionary<uint, VizPath> _Paths = new Dictionary<uint, VizPath>();
    private readonly Dictionary<string, VizPath> _SegmentPrimatives = new Dictionary<string, VizPath>();

    // temp list used for clearing ranges of items.
    private readonly List<uint> _TmpList = new List<uint>();

    public float[] Emotions { get; private set; }

    public string[] RecentMoodEvents { get; private set; }

    public string Reaction { get; private set; }

    public BehaviorID BehaviorID { get; private set; }

    public BehaviorScoreData[] BehaviorScoreData { get; private set; }

    public RobotInterface.AnimationState AnimationState { get; private set; }

    public Camera Camera { get { return _Camera; } }

    private Vector3 _InitialCameraPosition;
    private Quaternion _InitialCameraRotation;

    public static VizManager Instance { get; private set; }

    private static int _EnabledReferenceCount = 0;
    public static bool Enabled {
      get { return _EnabledReferenceCount > 0; }
      set {
        if (value) {
          _EnabledReferenceCount++;
        }
        else {
          _EnabledReferenceCount--;
          if (_EnabledReferenceCount < 0) {
            _EnabledReferenceCount = 0;
          }
        }
      }
    }

    private void OnEnable() {

      DAS.Info("VizManager.OnEnable", string.Empty);
      if (Instance != null && Instance != this) {
        Destroy(gameObject);
        return;
      }
      else {
        Instance = this;
      }

#if VIZ_ON_DEVICE
      _Channel = new VizDirectChannel();
#else
      _Channel = new VizUdpChannel();
#endif
      _Channel.ConnectedToClient += Connected;
      _Channel.DisconnectedFromClient += Disconnected;
      _Channel.MessageReceived += ReceivedMessage;

      _InitialCameraPosition = _Camera.transform.localPosition;
      _InitialCameraRotation = _Camera.transform.localRotation;

      _ImageReceiver.OnImageReceived += HandleReceivedImage;

      Listen();
    }

    private void Update() {
      if (Enabled == false) {
        // since we the texture isn't enabled lets
        // dump all of our messages we're getting.
        _Channel.DumpReceiveBuffer();
        return;
      }

      _Channel.Update();
      if (_OverlayImageWithFrame != null && _OverlayDirty) {
        _OverlayImageWithFrame.Apply();
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
            DAS.Warn("VizManager.Disconnect.DidNotWaitToFinishSending", "Not waiting for disconnect to finish sending.");
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
      DAS.Debug("VizManager.Disconnected", reason.ToString());

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
        _ImageReceiver.ProcessImageChunk(message.ImageChunk);
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
        DrawCameraText(message.CameraText);
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

      case MessageViz.Tag.MemoryMapMessageDebugVizBegin:
        BeginMemoryMapMessageDebugViz(message.MemoryMapMessageDebugVizBegin);
        break;
      case MessageViz.Tag.MemoryMapMessageDebugViz:
        MemoryMapMessageDebugViz(message.MemoryMapMessageDebugViz);
        break;
      case MessageViz.Tag.MemoryMapMessageDebugVizEnd:
        EndMemoryMapMessageDebugViz(message.MemoryMapMessageDebugVizEnd);
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
        BehaviorID = message.NewBehaviorSelected.newCurrentBehavior;
        break;
      case MessageViz.Tag.NewReactionTriggered:
        Reaction = message.NewReactionTriggered.reactionStrategyTriggered;
        break;
      case MessageViz.Tag.RobotBehaviorSelectData:
        BehaviorScoreData = message.RobotBehaviorSelectData.scoreData;
        break;
      case MessageViz.Tag.AnimationState:
        AnimationState = message.AnimationState;
        break;

      // TODO: None of the following are implemented
      // Not sure which ones we actually use
      case MessageViz.Tag.SetLabel:
      case MessageViz.Tag.DefineColor:
      case MessageViz.Tag.DockingErrorSignal:
      case MessageViz.Tag.RobotStateMessage:
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
        vizQuad.Initialize("Quad_" + quad.quadID.ToString());
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

    public void ShowObjects(ShowObjects showObjects) {

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
        DAS.Error("VizManager.SimpleQuadVector.IdentifierNotFoundError", "Could not find SimpleQuadVector with identifier " + msg.identifier);
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
        DAS.Error("VizManager.SimpleQuadVector.IdentifierNotFoundError", "Could not find SimpleQuadVector with identifier " + end.identifier);
        return;
      }
      vizQuad.EndUpdateQuadList();
    }

    public void BeginMemoryMapMessageDebugViz(MemoryMapMessageDebugVizBegin msg) {
      _MemoryMapInfos.Add(msg.originId, msg.info);
      _QuadInfos.Add(msg.originId, new List<ExternalInterface.MemoryMapQuadInfoDebugViz>());
    }

    public void MemoryMapMessageDebugViz(MemoryMapMessageDebugViz msg) {
      _QuadInfos[msg.originId].AddRange(msg.quadInfos);
    }

    public void EndMemoryMapMessageDebugViz(MemoryMapMessageDebugVizEnd msg) {
      // Now that we've received the entire list of quad infos, decode them into a list of drawable quads

      var info = _MemoryMapInfos[msg.originId];
      var rootCenter = new Vector3(info.rootCenterX * .001f, info.rootCenterY * .001f, info.rootCenterZ * .001f);
      var rootNode = new MemoryMapNode(info.rootDepth, info.rootSize_mm * .001f, rootCenter);

      var key = "MemMap_" + msg.originId;
      VizQuad vizQuad;
      if (!_SimpleQuadVectors.TryGetValue(key, out vizQuad)) {
        vizQuad = UIManager.CreateUIElement(_VizQuadPrefab, _VizScene).GetComponent<VizQuad>();
        vizQuad.Initialize("SimpleQuadVectorMemMap_" + msg.originId);
        _SimpleQuadVectors.Add(key, vizQuad);
      }
      vizQuad.StartUpdateQuadList();

      foreach (var quadInfo in _QuadInfos[msg.originId]) {
        rootNode.AddChild(vizQuad, quadInfo.content, quadInfo.depth);
      }
      vizQuad.EndUpdateQuadList();

      _MemoryMapInfos.Remove(msg.originId);
      _QuadInfos.Remove(msg.originId);
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
        DAS.Warn("VizManager.SetPathColor.CouldNotSetColor", "Could not set color for path " + path.pathID);
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
      if (_OverlayImageWithFrame == null) {
        return;
      }
      if (_OverlayClearBuffer == null) {
        _OverlayClearBuffer = new Color32[_OverlayImageWithFrame.width * _OverlayImageWithFrame.height];
      }

      _OverlayImageWithFrame.SetPixels32(_OverlayClearBuffer);
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

      if (_OverlayImageWithFrame == null) {
        return;
      }

      int h = _OverlayImageWithFrame.height;
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
      if (dx > dy) {
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

      if (_OverlayImageWithFrame == null) {
        return;
      }

      int h = _OverlayImageWithFrame.height;
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
      var end = Mathf.Ceil(y / Mathf.Sqrt(2));

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

        d += 2 * x + 1;
        if (d > 0) {
          d += 2 - 2 * y--;
        }
      }
      _OverlayDirty = true;
    }

    // Actually set 4 pixels
    private void SetOverlayPixel(int x, int y, Color c) {

      _OverlayImageWithFrame.SetPixel(x, y, c);
      _OverlayImageWithFrame.SetPixel(x + 1, y, c);
      _OverlayImageWithFrame.SetPixel(x, y + 1, c);
      _OverlayImageWithFrame.SetPixel(x + 1, y + 1, c);
    }

    private void DrawCameraText(CameraText camText) {
      if (_OverlayImageWithFrame == null) {
        return;
      }

      string text = camText.text;

      var fontTexture = (Texture2D)_CameraFont.material.mainTexture;

      int x = camText.x, y = _OverlayImageWithFrame.height - 1 - camText.y - _CameraFont.lineHeight;

      var color = camText.color.ToColor();
      string[] textLines = text.Split(new char[] { '\n' }, 10);

      for (int iLine = 0; iLine < textLines.Length; iLine++) {

        for (int i = 0; i < textLines[iLine].Length; i++) {
          CharacterInfo charInfo;
          // our font is ASCII, so if we can't draw a character just draw a ?
          if (!_CameraFont.GetCharacterInfo(textLines[iLine][i], out charInfo)) {
            _CameraFont.GetCharacterInfo('?', out charInfo);
          }

          var uvBottomRight = charInfo.uvBottomRight;
          var uvBottomLeft = charInfo.uvBottomLeft;
          var uvTopLeft = charInfo.uvTopLeft;


          int minX = Mathf.RoundToInt(uvBottomRight.x * fontTexture.width);
          int maxX = Mathf.RoundToInt(uvTopLeft.x * fontTexture.width);
          int minY = Mathf.RoundToInt(uvBottomRight.y * fontTexture.height);
          int maxY = Mathf.RoundToInt(uvTopLeft.y * fontTexture.height);

          int stepX = minX < maxX ? 1 : -1;
          int stepY = minY < maxY ? 1 : -1;

          // if the bottom y changes, it means we need to swap x/y when writing to our texture
          bool swapXY = !Mathf.Approximately(uvBottomRight.y, uvBottomLeft.y);

          int xRange = Mathf.Abs(minX - maxX);
          int yRange = Mathf.Abs(minY - maxY);

          for (int j = 0; j < xRange; j++) {
            for (int k = 0; k < yRange; k++) {

              int jf = minX + stepX * j;
              int kf = minY + stepY * k;

              int jo = x + charInfo.bearing + charInfo.glyphWidth - (swapXY ? 1 + k : j) - charInfo.minX;
              int ko = y + (swapXY ? 1 + j : k) + charInfo.minY;

              Color oldColor = _OverlayImageWithFrame.GetPixel(jo, ko);

              Color fontColor = fontTexture.GetPixel(jf, kf);

              var newColor = oldColor * (1 - fontColor.a) + color * fontColor.a;

              _OverlayImageWithFrame.SetPixel(jo, ko, newColor);
            }
          }

          x += charInfo.advance;
        }
        // Reset to new line
        x = camText.x;
        y -= _CameraFont.lineHeight;
      }
    }

    #endregion // OverlayRender

    #region ImageChunk

    private void HandleReceivedImage(Texture2D image) {
      _OverlayDirty = true;
      if (_OverlayImageWithFrame == null) {
        _OverlayImageWithFrame = new Texture2D(image.width, image.height);
        _OverlayImageWithFrame.name = "RobotOverlayImageWithFrame";
      }
      ClearOverlay();
    }


    #endregion // ImageChunk
  }
}

