using UnityEngine;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

public class ObservableObject : IVisibleInCamera {

  public class Light : Robot.Light {
    public static new float MessageDelay = 0f;

    public override void ClearData() {
      base.ClearData();
      MessageDelay = 0f;
    }

    public void SetFlashingLED(Color onColor, uint onDurationMs = 200, uint offDurationMs = 200, uint transitionMs = 0) {
      OnColor = onColor.ToUInt();
      OffColor = 0;
      OnPeriodMs = onDurationMs;
      OffPeriodMs = offDurationMs;
      TransitionOnPeriodMs = transitionMs;
      TransitionOffPeriodMs = transitionMs;
      Offset = 0;
    }
  }

  public const int kInvalidObjectID = -1;
  private const uint _kFindCubesTimeoutFrames = 1;

  // Copy the enum from CLAD to keep properly synchronized
  public enum PoseState {
    Invalid = Anki.PoseState.Invalid,
    Known   = Anki.PoseState.Known,
    Dirty   = Anki.PoseState.Dirty
  }

  public enum InFieldOfViewState {
    Visible,
    PartiallyVisible,
    NotVisible
  }

  public delegate void InFieldOfViewStateChangedHandler(ObservableObject objectChanged,
                                                        InFieldOfViewState oldState, InFieldOfViewState newState);

  public static event InFieldOfViewStateChangedHandler AnyInFieldOfViewStateChanged;

  public event InFieldOfViewStateChangedHandler InFieldOfViewStateChanged;

  public delegate void LightChangedHandler();

  /// <summary>
  /// Sent at most once per frame when any values of any lights on this object changes.
  /// </summary>
  public event LightChangedHandler OnLightsChanged;

  public uint RobotID { get; private set; }

  public ObjectFamily Family { get; private set; }

  public ObjectType ObjectType { get; private set; }

  public int ID { get; private set; }

  public uint FactoryID { get; set; }

  public bool IsInFieldOfView { get { return CurrentInFieldOfViewState == InFieldOfViewState.Visible; } }

  public event VizRectChangedHandler OnVizRectChanged;
  private Rect _VizRect;
  public Rect VizRect {
    get { return _VizRect; }
    private set {
      _VizRect = value;
      if (OnVizRectChanged != null) {
        OnVizRectChanged(this, _VizRect);
      }
    }
  }

  public Vector3? VizWorldPosition { get { return WorldPosition; } }

  public Vector3 WorldPosition { get; private set; }

  public Quaternion Rotation { get; private set; }

  public Vector3 Forward { get { return Rotation * Vector3.right; } }

  public Vector3 Right { get { return Rotation * -Vector3.up; } }

  public Vector3 Up { get { return Rotation * Vector3.forward; } }

  public Vector3 TopNorth { get { return Quaternion.AngleAxis(TopFaceNorthAngle * Mathf.Rad2Deg, Vector3.forward) * Vector2.right; } }

  public Vector3 TopEast { get { return Quaternion.AngleAxis(TopFaceNorthAngle * Mathf.Rad2Deg, Vector3.forward) * -Vector2.up; } }

  public Vector3 TopNorthEast { get { return (TopNorth + TopEast).normalized; } }

  public Vector3 TopSouthEast { get { return (-TopNorth + TopEast).normalized; } }

  public Vector3 TopSouthWest { get { return (-TopNorth - TopEast).normalized; } }

  public Vector3 TopNorthWest { get { return (TopNorth - TopEast).normalized; } }

  public float TopFaceNorthAngle { get; private set; }

  public Vector3 Size { get; private set; }

  public uint LastSeenEngineTimestamp { get; private set; }

  public uint LastMovementMessageEngineTimestamp { get; private set; }

  public uint LastUpAxisChangedMessageEngineTimestamp { get; private set; }

  protected IRobot RobotInstance { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  // Maps to isActive from messages in engine; indicates if an object has lights or not
  public bool HasLights { get; private set; }

  public string InfoString { get; private set; }

  public string SelectInfoString { get; private set; }

  public PoseState CurrentPoseState { get; private set; }

  public Light[] Lights { get; private set; }

  public bool LightsChanged {
    get {
      if (animFinished) {
        animFinished = false;
        return true;
      }

      if (lastRelativeMode != relativeMode || lastRelativeToX != relativeToX || lastRelativeToY != relativeToY || lastRotationPeriodMs != rotationPeriodMs)
        return true;

      for (int i = 0; i < Lights.Length; ++i) {
        if (Lights[i].Changed)
          return true;
      }

      return false;
    }
  }

  private InFieldOfViewState _CurrentInFieldOfViewState = InFieldOfViewState.NotVisible;

  public InFieldOfViewState CurrentInFieldOfViewState {
    get {
      return _CurrentInFieldOfViewState;
    }
    private set {
      if (value != _CurrentInFieldOfViewState) {
        InFieldOfViewState oldState = _CurrentInFieldOfViewState;
        _CurrentInFieldOfViewState = value;
        if (AnyInFieldOfViewStateChanged != null) {
          AnyInFieldOfViewStateChanged(this, oldState, _CurrentInFieldOfViewState);
        }
        if (InFieldOfViewStateChanged != null) {
          InFieldOfViewStateChanged(this, oldState, _CurrentInFieldOfViewState);
        }
      }
    }
  }

  public virtual string ReticleLabelLocKey {
    get {
      string key = "";
      if (this.ObjectType == ObjectType.Charger_Basic) {
        key = LocalizationKeys.kDroneModeChargerReticleLabel;
      }
      return key;
    }
  }
  public virtual string ReticleLabelStringArg { get { return null; } }

  public bool IsMoving { get; private set; }

  private int _ConsecutiveVisionFramesNotSeen = 0;

  private float lastRelativeToX;
  public float relativeToX;
  private float lastRelativeToY;
  public float relativeToY;

  private MakeRelativeMode lastRelativeMode;
  public MakeRelativeMode relativeMode;

  private float lastRotationPeriodMs;
  public float rotationPeriodMs;

  private U2G.SetAllActiveObjectLEDs SetAllActiveObjectLEDsMessage;

  private bool animFinished = false;


  public ObservableObject() {
  }

  public ObservableObject(int objectID, uint factoryID, ObjectFamily objectFamily, ObjectType objectType) {
    Family = objectFamily;
    ObjectType = objectType;
    ID = objectID;
    FactoryID = factoryID;
    HasLights = false;

    InfoString = "ObjectID: " + ID + " FactoryID = " + FactoryID.ToString("X") + " Family: " + Family + " Type: " + ObjectType;
    SelectInfoString = "Select ObjectID: " + ID + " FactoryID = " + FactoryID.ToString("X") + " Family: " + Family + " Type: " + ObjectType;

    CurrentPoseState = PoseState.Invalid;
    _CurrentInFieldOfViewState = InFieldOfViewState.NotVisible;

    LastSeenEngineTimestamp = 0;
    LastMovementMessageEngineTimestamp = 0;
    LastUpAxisChangedMessageEngineTimestamp = 0;

    _ConsecutiveVisionFramesNotSeen = 0;

    SetAllActiveObjectLEDsMessage = new U2G.SetAllActiveObjectLEDs();

    Lights = new Light[SetAllActiveObjectLEDsMessage.onColor.Length];

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i] = new Light();
    }

    DAS.Debug("ObservableObject.Constructed", "ObservableObject from objectFamily(" + objectFamily + ") objectType(" + objectType + ")");
  }

  public static implicit operator uint(ObservableObject observableObject) {
    if (observableObject == null) {
      DAS.Warn(string.Format("{0}.NullValue", typeof(ObservableObject)), "converting null ObservableObject into uint: returning uint.MaxValue");
      return uint.MaxValue;
    }

    return (uint)observableObject.ID;
  }

  public static implicit operator int(ObservableObject observableObject) {
    if (observableObject == null)
      return kInvalidObjectID;

    return observableObject.ID;
  }

  public static implicit operator string(ObservableObject observableObject) {
    return ((int)observableObject).ToString();
  }

  // Useful for when the world was rejiggered.
  public void UpdateAvailable(G2U.LocatedObjectState message) {
    CurrentPoseState = (ObservableObject.PoseState)message.poseState;
    if (CurrentPoseState != PoseState.Invalid) {
      Vector3 newPos = new Vector3(message.pose.x, message.pose.y, message.pose.z);
      //dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
      WorldPosition = newPos;
      Rotation = new Quaternion(message.pose.q1, message.pose.q2, message.pose.q3, message.pose.q0);
    }
  }

  public void UpdateInfo(G2U.RobotObservedObject message) {
    LastSeenEngineTimestamp = message.timestamp;
    _ConsecutiveVisionFramesNotSeen = 0;

    RobotID = message.robotID;
    VizRect = new Rect(message.img_rect.x_topLeft, message.img_rect.y_topLeft, message.img_rect.width, message.img_rect.height);

    Vector3 newPos = new Vector3(message.pose.x, message.pose.y, message.pose.z);
    //dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
    WorldPosition = newPos;
    Rotation = new Quaternion(message.pose.q1, message.pose.q2, message.pose.q3, message.pose.q0);
    Size = Vector3.one * CozmoUtil.kBlockLengthMM;

    TopFaceNorthAngle = message.topFaceOrientation_rad + Mathf.PI * 0.5f;

    // Andrew Stein / Ivy Ngo:
    // isActive corresponds to whether or not the object has lights
    HasLights = message.isActive > 0;

    CurrentInFieldOfViewState = InFieldOfViewState.Visible;

    // Mark pose known
    CurrentPoseState = PoseState.Known;
    IsMoving = false;
  }

  public void MarkNotVisibleThisFrame() {
    // If not seen frame count of cube is greater than limit, mark object NotVisible
    _ConsecutiveVisionFramesNotSeen++;
    if (_ConsecutiveVisionFramesNotSeen > _kFindCubesTimeoutFrames) {
      CurrentInFieldOfViewState = InFieldOfViewState.NotVisible;
    }
  }

  public virtual void HandleStartedMoving(ObjectMoved message) {
    IsMoving = true;
    LastMovementMessageEngineTimestamp = message.timestamp;

    if (CurrentPoseState != PoseState.Invalid) {
      CurrentPoseState = PoseState.Dirty;
    }
  }

  public virtual void HandleStoppedMoving(ObjectStoppedMoving message) {
    IsMoving = false;
    LastMovementMessageEngineTimestamp = message.timestamp;
  }

  public virtual void HandleUpAxisChanged(ObjectUpAxisChanged message) {
    LastUpAxisChangedMessageEngineTimestamp = message.timestamp;
  }

  public void MarkPoseUnknown() {
    CurrentPoseState = PoseState.Invalid;
  }

  public virtual void HandleObjectTapped(ObjectTapped message) {
  }

  public void SetAllLEDs() { // should only be called from update loop
    SetAllActiveObjectLEDsMessage.objectID = (uint)ID;
    SetAllActiveObjectLEDsMessage.robotID = (byte)RobotID;

    for (int i = 0; i < Lights.Length; ++i) {
      SetAllActiveObjectLEDsMessage.onPeriod_ms[i] = Lights[i].OnPeriodMs;
      SetAllActiveObjectLEDsMessage.offPeriod_ms[i] = Lights[i].OffPeriodMs;
      SetAllActiveObjectLEDsMessage.transitionOnPeriod_ms[i] = Lights[i].TransitionOnPeriodMs;
      SetAllActiveObjectLEDsMessage.transitionOffPeriod_ms[i] = Lights[i].TransitionOffPeriodMs;
      SetAllActiveObjectLEDsMessage.onColor[i] = Lights[i].OnColor;
      SetAllActiveObjectLEDsMessage.offColor[i] = Lights[i].OffColor;
      SetAllActiveObjectLEDsMessage.offset[i] = Lights[i].Offset;
    }

    SetAllActiveObjectLEDsMessage.makeRelative = relativeMode;
    SetAllActiveObjectLEDsMessage.relativeToX = relativeToX;
    SetAllActiveObjectLEDsMessage.relativeToY = relativeToY;
    SetAllActiveObjectLEDsMessage.rotationPeriod_ms = (uint)rotationPeriodMs;

    RobotEngineManager.Instance.Message.SetAllActiveObjectLEDs = SetAllActiveObjectLEDsMessage;
    RobotEngineManager.Instance.SendMessage();

    if (OnLightsChanged != null) {
      OnLightsChanged();
    }

    SetLastLEDs();
  }

  private void SetLastLEDs() {
    lastRelativeMode = relativeMode;
    lastRelativeToX = relativeToX;
    lastRelativeToY = relativeToY;
    lastRotationPeriodMs = rotationPeriodMs;

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].SetLastInfo();
    }
  }

  public void SetLEDsOff() {
    SetLEDs((uint)LEDColor.LED_OFF);
  }

  public void SetLEDs(Color onColor) {
    SetLEDs(onColor.ToUInt());
  }

  public void SetLEDs(Color[] onColor) {
    uint[] colors = new uint[onColor.Length];
    for (int i = 0; i < onColor.Length; i++) {
      colors[i] = onColor[i].ToUInt();
    }
    SetLEDs(colors);
  }

  public void SetFlashingLEDs(Color onColor, uint onDurationMs = 100, uint offDurationMs = 100, uint transitionMs = 0) {
    SetLEDs(onColor.ToUInt(), 0, onDurationMs, offDurationMs, transitionMs, transitionMs);
  }

  public void SetFlashingLEDs(uint onColor, uint onDurationMs = 100, uint offDurationMs = 100, uint transitionMs = 0) {
    SetLEDs(onColor, 0, onDurationMs, offDurationMs, transitionMs, transitionMs);
  }

  public void SetFlashingLEDs(Color[] onColors, Color offColor, uint onDurationMs = 100, uint offDurationMs = 100, uint transitionMs = 0) {
    uint[] onColorsUint = new uint[onColors.Length];
    for (int i = 0; i < onColorsUint.Length; i++) {
      onColorsUint[i] = onColors[i].ToUInt();
    }
    SetLEDs(onColorsUint, offColor.ToUInt(), onDurationMs, offDurationMs, transitionMs, transitionMs);
  }

  public void SetLEDs(uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0,
                      uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0) {

    Light light;
    for (int i = 0; i < Lights.Length; ++i) {
      light = Lights[i];
      light.OnColor = onColor;
      light.OffColor = offColor;
      light.OnPeriodMs = onPeriod_ms;
      light.OffPeriodMs = offPeriod_ms;
      light.TransitionOnPeriodMs = transitionOnPeriod_ms;
      light.TransitionOffPeriodMs = transitionOffPeriod_ms;
      light.Offset = 0;
    }

    relativeMode = 0;
    relativeToX = 0;
    relativeToY = 0;
    rotationPeriodMs = 0;
  }

  public void SetLEDs(uint onColor, uint offColor, uint onPeriod_ms, uint offPeriod_ms,
                      uint transitionOnPeriod_ms, uint transitionOffPeriod_ms, int[] offset) {

    Light light;
    for (int i = 0; i < Lights.Length; ++i) {
      light = Lights[i];
      light.OnColor = onColor;
      light.OffColor = offColor;
      light.OnPeriodMs = onPeriod_ms;
      light.OffPeriodMs = offPeriod_ms;
      light.TransitionOnPeriodMs = transitionOnPeriod_ms;
      light.TransitionOffPeriodMs = transitionOffPeriod_ms;
      light.Offset = offset[i];
    }

    relativeMode = 0;
    relativeToX = 0;
    relativeToY = 0;
    rotationPeriodMs = 0;
  }

  public void SetLEDs(uint[] lightColors, uint offColor = 0, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0,
    uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0) {

    uint onColor;
    Light light;
    for (int i = 0; i < Lights.Length; ++i) {
      onColor = lightColors[i % lightColors.Length];
      light = Lights[i];
      light.OnColor = onColor;
      light.OffColor = offColor;
      light.OnPeriodMs = onPeriod_ms;
      light.OffPeriodMs = offPeriod_ms;
      light.TransitionOnPeriodMs = transitionOnPeriod_ms;
      light.TransitionOffPeriodMs = transitionOffPeriod_ms;
      light.Offset = 0;
    }

    relativeMode = 0;
    relativeToX = 0;
    relativeToY = 0;
    rotationPeriodMs = 0;
  }

  public void SetLEDsRelative(Vector2 relativeTo, uint onColor = 0, uint offColor = 0, MakeRelativeMode relativeMode = MakeRelativeMode.RELATIVE_LED_MODE_BY_CORNER,
    uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {
    SetLEDsRelative(relativeTo.x, relativeTo.y, onColor, offColor, relativeMode, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms);
  }

  public void SetLEDsRelative(float relativeToX, float relativeToY, uint onColor = 0, uint offColor = 0, MakeRelativeMode relativeMode = MakeRelativeMode.RELATIVE_LED_MODE_BY_CORNER,
    uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0) {
    SetLEDs(onColor, offColor, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms);

    this.relativeMode = relativeMode;
    this.relativeToX = relativeToX;
    this.relativeToY = relativeToY;
  }

  // Play an animation on the cube. If the animation has infinite duration then it must be stopped by calling
  // StopAnim()
  // If you want a callback when the animation finishes use Robot.PlayCubeAnimationTrigger() instead
  public void PlayAnim(CubeAnimationTrigger trigger) {
    G2U.PlayCubeAnim anim = new U2G.PlayCubeAnim();
    anim.trigger = trigger;
    anim.objectID = (uint)ID;

    RobotEngineManager.Instance.Message.PlayCubeAnim = anim;
    RobotEngineManager.Instance.SendMessage();

    animFinished = false;
  }

  public void StopAnim(CubeAnimationTrigger trigger) {
    G2U.StopCubeAnim anim = new U2G.StopCubeAnim();
    anim.trigger = trigger;
    anim.objectID = (uint)ID;

    RobotEngineManager.Instance.Message.StopCubeAnim = anim;
    RobotEngineManager.Instance.SendMessage();

    animFinished = true;
  }

  public Color[] GetLEDs() {
    Color[] lightColors = new Color[Lights.Length];
    for (int i = 0; i < Lights.Length; ++i) {
      lightColors[i] = Lights[i].OnColor.ToColor();
    }
    return lightColors;
  }
}
