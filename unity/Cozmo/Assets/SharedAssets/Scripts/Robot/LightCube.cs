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
public class LightCube : ObservedObject {
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

  public bool IsMoving { get; private set; }

  public UpAxis UpAxis { get; private set; }

  public float XAccel { get; private set; }

  public float YAccel { get; private set; }

  public float ZAccel { get; private set; }

  private U2G.SetAllActiveObjectLEDs SetAllActiveObjectLEDsMessage;

  public Light[] Lights { get; private set; }

  public bool lightsChanged {
    get {
      if (lastRelativeMode != relativeMode || lastRelativeToX != relativeToX || lastRelativeToY != relativeToY)
        return true;

      for (int i = 0; i < Lights.Length; ++i) {
        if (Lights[i].changed)
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

  public event Action<LightCube> OnAxisChange;

  public static Action<int, int> TappedAction;
  public static Action<int, float, float, float> MovedAction;
  public static Action<int> StoppedAction;

  public LightCube(int objectID, ObjectFamily objectFamily, ObjectType objectType) {
    Constructor(objectID, objectFamily, objectType);

    UpAxis = UpAxis.Unknown;
    XAccel = byte.MaxValue;
    YAccel = byte.MaxValue;
    ZAccel = byte.MaxValue;
    IsMoving = false;

    SetAllActiveObjectLEDsMessage = new U2G.SetAllActiveObjectLEDs();

    Lights = new Light[SetAllActiveObjectLEDsMessage.onColor.Length];

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i] = new Light();
    }

  }

  public void Moving(ObjectMoved message) {
    IsMoving = true;

    UpAxis = message.upAxis;
    XAccel = message.accel.x;
    YAccel = message.accel.y;
    ZAccel = message.accel.z;

    if (MovedAction != null) {
      MovedAction(ID, XAccel, YAccel, ZAccel);
    }
  }

  public void StoppedMoving(ObjectStoppedMoving message) {
    IsMoving = false;

    if (message.rolled) {
      if (OnAxisChange != null)
        OnAxisChange(this);
    }
    if (StoppedAction != null) {
      StoppedAction(ID);
    }
  }

  public void Tapped(ObjectTapped message) {
    DAS.Debug("LightCube", "Tapped Message Received for LightCube(" + ID + "): " + message.numTaps + " taps");
    if (TappedAction != null)
      TappedAction(ID, message.numTaps);
  }

  public void SetAllLEDs() { // should only be called from update loop
    SetAllActiveObjectLEDsMessage.objectID = (uint)ID;
    SetAllActiveObjectLEDsMessage.robotID = (byte)RobotID;

    for (int i = 0; i < Lights.Length; ++i) {
      SetAllActiveObjectLEDsMessage.onPeriod_ms[i] = Lights[i].onPeriod_ms;
      SetAllActiveObjectLEDsMessage.offPeriod_ms[i] = Lights[i].offPeriod_ms;
      SetAllActiveObjectLEDsMessage.transitionOnPeriod_ms[i] = Lights[i].transitionOnPeriod_ms;
      SetAllActiveObjectLEDsMessage.transitionOffPeriod_ms[i] = Lights[i].transitionOffPeriod_ms;
      SetAllActiveObjectLEDsMessage.onColor[i] = Lights[i].onColor;
      SetAllActiveObjectLEDsMessage.offColor[i] = Lights[i].offColor;
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

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].SetLastInfo();
    }
  }

  public void SetLEDs(uint onColor = 0, uint offColor = 0, uint onPeriod_ms = Light.FOREVER, uint offPeriod_ms = 0, 
                      uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0, byte turnOffUnspecifiedLEDs = 1) {

    for (int i = 0; i < Lights.Length; ++i) {
      Lights[i].onColor = onColor;
      Lights[i].offColor = offColor;
      Lights[i].onPeriod_ms = onPeriod_ms;
      Lights[i].offPeriod_ms = offPeriod_ms;
      Lights[i].transitionOnPeriod_ms = transitionOnPeriod_ms;
      Lights[i].transitionOffPeriod_ms = transitionOffPeriod_ms;
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
