using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

/// <summary>
/// unity representation of cozmo's LightCubes
///   adds functionality for controlling the four LEDs
///   adds awareness of accelerometer messages to detect LightCube movements
/// </summary>
public class ActiveBlock : ObservedObject {
  public class Light : Robot.Light {
    [System.FlagsAttribute]
    public new enum PositionFlag {
      NONE = 0,
      BACK_EAST = 0x01,
      LEFT_NORTH = 0x02,
      FRONT_WEST = 0x04,
      RIGHT_SOUTH = 0x08,
      ALL = 0xff}

    ;

    public static PositionFlag IndexToPosition(int i) {
      switch (i) {
      case 0:
        return PositionFlag.BACK_EAST;
      case 1:
        return PositionFlag.LEFT_NORTH;
      case 2:
        return PositionFlag.FRONT_WEST;
      case 3:
        return PositionFlag.RIGHT_SOUTH;
      }
      
      return PositionFlag.NONE;
    }

    public static int GetIndexForEdgeClosestToAngle(float angleFromNorth) {
      if (angleFromNorth >= -45f && angleFromNorth < 45f)
        return 1; //LEFT_NORTH
      if (angleFromNorth >= 45f && angleFromNorth < 135f)
        return 2; //FRONT_WEST
      if (angleFromNorth >= 135f || angleFromNorth < -135f)
        return 3; //RIGHT_SOUTH
      if (angleFromNorth >= -135f && angleFromNorth < -45f)
        return 0; //BACK_EAST

      return 0;
    }

    private PositionFlag position;

    public Light(int position) {
      this.position = IndexToPosition(position);
    }

    public bool Position(PositionFlag s) {
      return (position | s) == s;
    }

    public static new float messageDelay = 0f;

    public override void ClearData() {
      base.ClearData();
      
      messageDelay = 0f;
    }
  }

  public enum Mode {
    Off = 0,
    White,
    Red,
    Yellow,
    Green,
    Cyan,
    Blue,
    Magenta,
    Count
  }

  public bool isMoving { get; private set; }

  public UpAxisClad upAxis { get; private set; }

  public float xAccel { get; private set; }

  public float yAccel { get; private set; }

  public float zAccel { get; private set; }

  private U2G.SetAllActiveObjectLEDs SetAllActiveObjectLEDsMessage;

  public AudioClip modeSound { get { return GameActions.instance != null ? GameActions.instance.GetActiveBlockModeSound(mode) : null; } }

  public float modeDelay { get { return modeSound != null ? modeSound.length : 0f; } }

  public Light[] lights { get; private set; }

  public bool lightsChanged {
    get {
      if (lastRelativeMode != relativeMode || lastRelativeToX != relativeToX || lastRelativeToY != relativeToY)
        return true;

      for (int i = 0; i < lights.Length; ++i) {
        if (lights[i].changed)
          return true;
      }

      return false;
    }
  }

  private byte lastRelativeMode;
  public byte relativeMode;

  private float lastRelativeToX;
  public float relativeToX;
  private float lastRelativeToY;
  public float relativeToY;

  public Mode mode { get; private set; }

  public event Action<ActiveBlock> OnAxisChange;

  public static Action<int, int> TappedAction;

  public ActiveBlock(int objectID, uint objectFamily, uint objectType) {
    Constructor(objectID, objectFamily, objectType);

    upAxis = UpAxisClad.Unknown;
    xAccel = byte.MaxValue;
    yAccel = byte.MaxValue;
    zAccel = byte.MaxValue;
    isMoving = false;

    SetAllActiveObjectLEDsMessage = new U2G.SetAllActiveObjectLEDs();

    lights = new Light[SetAllActiveObjectLEDsMessage.onColor.Length];

    for (int i = 0; i < lights.Length; ++i) {
      lights[i] = new Light(i);
    }

    SetMode(Mode.Off);
  }

  public void Moving(G2U.ActiveObjectMoved message) {
    if (isMoving)
      return;

    isMoving = true;

    upAxis = message.upAxis;
    xAccel = message.xAccel;
    yAccel = message.yAccel;
    zAccel = message.zAccel;
  }

  public void StoppedMoving(G2U.ActiveObjectStoppedMoving message) {
    if (!isMoving)
      return;

    isMoving = false;

    if (message.rolled) {
      if (OnAxisChange != null)
        OnAxisChange(this);
    }
  }

  public void Tapped(G2U.ActiveObjectTapped message) {
    Debug.Log("Tapped Message Received for ActiveBlock(" + ID + "): " + message.numTaps + " taps");
    if (TappedAction != null)
      TappedAction(ID, message.numTaps);
  }

  public void SetAllLEDs() { // should only be called from update loop
    SetAllActiveObjectLEDsMessage.objectID = (uint)ID;
    SetAllActiveObjectLEDsMessage.robotID = (byte)RobotID;

    for (int i = 0; i < lights.Length; ++i) {
      SetAllActiveObjectLEDsMessage.onPeriod_ms[i] = lights[i].onPeriod_ms;
      SetAllActiveObjectLEDsMessage.offPeriod_ms[i] = lights[i].offPeriod_ms;
      SetAllActiveObjectLEDsMessage.transitionOnPeriod_ms[i] = lights[i].transitionOnPeriod_ms;
      SetAllActiveObjectLEDsMessage.transitionOffPeriod_ms[i] = lights[i].transitionOffPeriod_ms;
      SetAllActiveObjectLEDsMessage.onColor[i] = lights[i].onColor;
      SetAllActiveObjectLEDsMessage.offColor[i] = lights[i].offColor;
    }

    SetAllActiveObjectLEDsMessage.makeRelative = relativeMode;
    SetAllActiveObjectLEDsMessage.relativeToX = relativeToX;
    SetAllActiveObjectLEDsMessage.relativeToY = relativeToY;

    //Debug.Log( "SetAllActiveObjectLEDs for Object with ID: " + ID );

    RobotEngineManager.instance.Message.SetAllActiveObjectLEDs = SetAllActiveObjectLEDsMessage;
    RobotEngineManager.instance.SendMessage();

    SetLastLEDs();
    Light.messageDelay = Time.time + GameController.MessageDelay;
  }

  private void SetLastLEDs() {
    lastRelativeMode = relativeMode;
    lastRelativeToX = relativeToX;
    lastRelativeToY = relativeToY;

    for (int i = 0; i < lights.Length; ++i) {
      lights[i].SetLastInfo();
    }
  }

  public void SetLEDs(uint onColor = 0, uint offColor = 0, byte whichLEDs = byte.MaxValue, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, 
                      uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {

    //Debug.Log("SetLEDs transitionOnPeriod_ms("+transitionOnPeriod_ms+")");

    for (int i = 0; i < lights.Length; ++i) {
      if (lights[i].Position((Light.PositionFlag)whichLEDs)) {
        lights[i].onColor = onColor;
        lights[i].offColor = offColor;
        lights[i].onPeriod_ms = onPeriod_ms;
        lights[i].offPeriod_ms = offPeriod_ms;
        lights[i].transitionOnPeriod_ms = transitionOnPeriod_ms;
        lights[i].transitionOffPeriod_ms = transitionOffPeriod_ms;
      }
      else if (turnOffUnspecifiedLEDs > 0) {
        lights[i].onColor = 0;
        lights[i].offColor = 0;
        lights[i].onPeriod_ms = Light.FOREVER;
        lights[i].offPeriod_ms = 0;
        lights[i].transitionOnPeriod_ms = 0;
        lights[i].transitionOffPeriod_ms = 0;
      }
    }

    relativeMode = 0;
    relativeToX = 0;
    relativeToY = 0;
  }

  public void SetLEDsRelative(Vector2 relativeTo, uint onColor = 0, uint offColor = 0, byte whichLEDs = byte.MaxValue, byte relativeMode = 1,
                              uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {  
    SetLEDsRelative(relativeTo.x, relativeTo.y, onColor, offColor, whichLEDs, relativeMode, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms, turnOffUnspecifiedLEDs);
  }

  public void SetLEDsRelative(float relativeToX, float relativeToY, uint onColor = 0, uint offColor = 0, byte whichLEDs = byte.MaxValue, byte relativeMode = 1,
                              uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {
    SetLEDs(onColor, offColor, whichLEDs, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms, turnOffUnspecifiedLEDs);

    this.relativeMode = relativeMode;
    this.relativeToX = relativeToX;
    this.relativeToY = relativeToY;
  }

  public void SetMode(Mode m, bool playSound = false) {
    mode = m;

    if (CozmoPalette.instance != null)
      SetLEDs(CozmoPalette.instance.GetUIntColorForActiveBlockType(mode));
    if (playSound && GameActions.instance != null)
      AudioManager.PlayOneShot(modeSound, GameActions.instance.actionButtonnDelay);
  }

  public void CycleMode(bool playSound = false) {
    int typeIndex = (int)mode + 1;
    if (typeIndex >= (int)Mode.Count)
      typeIndex = 0;
    
    Debug.Log("active block id(" + ID + ") from " + mode + " to " + (ActiveBlock.Mode)typeIndex);
    
    SetMode((ActiveBlock.Mode)typeIndex, playSound);
  }
}
