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

  public UpAxis upAxis { get; private set; }

  public float xAccel { get; private set; }

  public float yAccel { get; private set; }

  public float zAccel { get; private set; }

  private U2G.SetAllActiveObjectLEDs SetAllActiveObjectLEDsMessage;

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

  private MakeRelativeMode lastRelativeMode;
  public MakeRelativeMode relativeMode;

  private float lastRelativeToX;
  public float relativeToX;
  private float lastRelativeToY;
  public float relativeToY;

  public Mode mode { get; private set; }

  public event Action<ActiveBlock> OnAxisChange;

  public static Action<int, int> TappedAction;
  public static Action<int, float, float, float> MovedAction;
  public static Action<int> StoppedAction;

  public ActiveBlock(int objectID, ObjectFamily objectFamily, ObjectType objectType) {
    Constructor(objectID, objectFamily, objectType);

    upAxis = UpAxis.Unknown;
    xAccel = byte.MaxValue;
    yAccel = byte.MaxValue;
    zAccel = byte.MaxValue;
    isMoving = false;

    SetAllActiveObjectLEDsMessage = new U2G.SetAllActiveObjectLEDs();

    lights = new Light[SetAllActiveObjectLEDsMessage.onColor.Length];

    for (int i = 0; i < lights.Length; ++i) {
      lights[i] = new Light();
    }

  }

  public void Moving(ObjectMoved message) {
    isMoving = true;

    upAxis = message.upAxis;
    xAccel = message.accel.x;
    yAccel = message.accel.y;
    zAccel = message.accel.z;

    if (MovedAction != null) {
      MovedAction(ID, xAccel, yAccel, zAccel);
    }
  }

  public void StoppedMoving(ObjectStoppedMoving message) {
    isMoving = false;

    if (message.rolled) {
      if (OnAxisChange != null)
        OnAxisChange(this);
    }
    if (StoppedAction != null) {
      StoppedAction(ID);
    }
  }

  public void Tapped(ObjectTapped message) {
    DAS.Debug("ActiveBlock", "Tapped Message Received for ActiveBlock(" + ID + "): " + message.numTaps + " taps");
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

    RobotEngineManager.instance.Message.SetAllActiveObjectLEDs = SetAllActiveObjectLEDsMessage;
    RobotEngineManager.instance.SendMessage();

    SetLastLEDs();
  }

  private void SetLastLEDs() {
    lastRelativeMode = relativeMode;
    lastRelativeToX = relativeToX;
    lastRelativeToY = relativeToY;

    for (int i = 0; i < lights.Length; ++i) {
      lights[i].SetLastInfo();
    }
  }

  public void SetLEDs(uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, 
                      uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {

    for (int i = 0; i < lights.Length; ++i) {
      lights[i].onColor = onColor;
      lights[i].offColor = offColor;
      lights[i].onPeriod_ms = onPeriod_ms;
      lights[i].offPeriod_ms = offPeriod_ms;
      lights[i].transitionOnPeriod_ms = transitionOnPeriod_ms;
      lights[i].transitionOffPeriod_ms = transitionOffPeriod_ms;
    }

    relativeMode = 0;
    relativeToX = 0;
    relativeToY = 0;
  }

  public void SetLEDsRelative(Vector2 relativeTo, uint onColor = 0, uint offColor = 0, MakeRelativeMode relativeMode = MakeRelativeMode.RELATIVE_LED_MODE_BY_CORNER,
                              uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {  
    SetLEDsRelative(relativeTo.x, relativeTo.y, onColor, offColor, relativeMode, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms, turnOffUnspecifiedLEDs);
  }

  public void SetLEDsRelative(float relativeToX, float relativeToY, uint onColor = 0, uint offColor = 0, MakeRelativeMode relativeMode = MakeRelativeMode.RELATIVE_LED_MODE_BY_CORNER,
                              uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {
    SetLEDs(onColor, offColor, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms, turnOffUnspecifiedLEDs);

    this.relativeMode = relativeMode;
    this.relativeToX = relativeToX;
    this.relativeToY = relativeToY;
  }
}
