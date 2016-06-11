using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

/// <summary>
/// all objects that cozmo sees are transmitted across to unity and represented here as ObservedObjects
///   so far, we only both handling three types of cubes and a charger
/// </summary>
public class ObservedObject {

  public enum PoseState {
    Known,
    Dirty,
    Unknown
  }

  public enum InFieldOfViewState {
    Visible,
    PartiallyVisible,
    NotVisible
  }

  public delegate void InFieldOfViewStateChangedHandler(ObservedObject objectChanged,
                                                        InFieldOfViewState oldState, InFieldOfViewState newState);

  public static event InFieldOfViewStateChangedHandler InFieldOfViewStateChanged;

  public uint RobotID { get; private set; }

  public ObjectFamily Family { get; private set; }

  public ObjectType ObjectType { get; private set; }

  public int ID { get; private set; }

  public uint FactoryID { get; set; }

  public bool MarkersVisible { get; private set; }

  public bool IsInFieldOfView { get { return CurrentInFieldOfViewState == InFieldOfViewState.Visible; } }

  public bool IsLooselyInFieldOfView { get { return CurrentInFieldOfViewState != InFieldOfViewState.NotVisible; } }

  public Rect VizRect { get; private set; }

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

  protected IRobot RobotInstance { get { return RobotEngineManager.Instance != null ? RobotEngineManager.Instance.CurrentRobot : null; } }

  // Maps to isActive from messages in engine; indicates if an object has lights or not
  public bool HasLights { get; private set; }

  public string InfoString { get; private set; }

  public string SelectInfoString { get; private set; }

  public PoseState CurrentPoseState { get; private set; }

  private InFieldOfViewState _CurrentInFieldOfViewState = InFieldOfViewState.NotVisible;
  public InFieldOfViewState CurrentInFieldOfViewState {
    get {
      return _CurrentInFieldOfViewState;
    }
    private set {
      if (value != _CurrentInFieldOfViewState) {
        InFieldOfViewState oldState = _CurrentInFieldOfViewState;
        _CurrentInFieldOfViewState = value;
        if (InFieldOfViewStateChanged != null) {
          InFieldOfViewStateChanged(this, oldState, _CurrentInFieldOfViewState);
        }
      }
    }
  }

  public bool IsMoving { get; private set; }

  private int _ConsecutiveVisionFramesNotSeen = 0;

  public ObservedObject() {
  }

  public ObservedObject(int objectID, uint factoryID, ObjectFamily objectFamily, ObjectType objectType) {
    Constructor(objectID, factoryID, objectFamily, objectType);
  }

  protected void Constructor(int objectID, uint factoryID, ObjectFamily objectFamily, ObjectType objectType) {
    Family = objectFamily;
    ObjectType = objectType;
    ID = objectID;
    FactoryID = factoryID;
    HasLights = false;

    InfoString = "ObjectID: " + ID + " FactoryID = " + FactoryID.ToString("X") + " Family: " + Family + " Type: " + ObjectType;
    SelectInfoString = "Select ObjectID: " + ID + " FactoryID = " + FactoryID.ToString("X") + " Family: " + Family + " Type: " + ObjectType;

    CurrentPoseState = PoseState.Unknown;
    _CurrentInFieldOfViewState = InFieldOfViewState.NotVisible;
    MarkersVisible = false;

    LastSeenEngineTimestamp = 0;
    LastMovementMessageEngineTimestamp = 0;

    _ConsecutiveVisionFramesNotSeen = 0;

    DAS.Debug("ObservedObject.Constructed", "ObservedObject from objectFamily(" + objectFamily + ") objectType(" + objectType + ")");
  }

  public static implicit operator uint(ObservedObject observedObject) {
    if (observedObject == null) {
      DAS.Warn(string.Format("{0}.NullValue", typeof(ObservedObject)), "converting null ObservedObject into uint: returning uint.MaxValue");
      return uint.MaxValue;
    }

    return (uint)observedObject.ID;
  }

  public static implicit operator int(ObservedObject observedObject) {
    if (observedObject == null)
      return -1;

    return observedObject.ID;
  }

  public static implicit operator string(ObservedObject observedObject) {
    return ((int)observedObject).ToString();
  }

  public void UpdateInfo(G2U.RobotObservedObject message) {
    LastSeenEngineTimestamp = message.timestamp;
    _ConsecutiveVisionFramesNotSeen = 0;

    RobotID = message.robotID;
    VizRect = new Rect(message.img_topLeft_x, message.img_topLeft_y, message.img_width, message.img_height);

    Vector3 newPos = new Vector3(message.world_x, message.world_y, message.world_z);
    //dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
    WorldPosition = newPos;
    Rotation = new Quaternion(message.quaternion_x, message.quaternion_y, message.quaternion_z, message.quaternion_w);
    Size = Vector3.one * CozmoUtil.kBlockLengthMM;

    TopFaceNorthAngle = message.topFaceOrientation_rad + Mathf.PI * 0.5f;

    // Andrew Stein / Ivy Ngo:
    // isActive corresponds to whether or not the object has lights
    HasLights = message.isActive > 0;

    // Mark Visible or PartiallyVisible depending on if the markersVisible is true
    // (recieving this message means that it's at least partially in view)
    if (message.markersVisible > 0) {
      CurrentInFieldOfViewState = InFieldOfViewState.Visible;
      MarkersVisible = true;
    }
    else {
      CurrentInFieldOfViewState = InFieldOfViewState.PartiallyVisible;
      MarkersVisible = false;
    }

    // Mark pose known
    CurrentPoseState = PoseState.Known;
    IsMoving = false;
  }

  public void MarkNotVisibleThisFrame() {
    // If not seen frame count of cube is greater than limit, mark object NotVisible
    _ConsecutiveVisionFramesNotSeen++;
    if (_ConsecutiveVisionFramesNotSeen > Cozmo.CubePalette.FindCubesTimeoutFrames) {
      CurrentInFieldOfViewState = InFieldOfViewState.NotVisible;
      MarkersVisible = false;
    }
  }

  public virtual void HandleStartedMoving(ObjectMoved message) {
    IsMoving = true;
    LastMovementMessageEngineTimestamp = message.timestamp;

    if (CurrentPoseState != PoseState.Unknown) {
      CurrentPoseState = PoseState.Dirty;
    }
  }

  public virtual void HandleStoppedMoving(ObjectStoppedMoving message) {
    IsMoving = false;
    LastMovementMessageEngineTimestamp = message.timestamp;
  }

  public void MarkPoseUnknown() {
    CurrentPoseState = PoseState.Unknown;
  }

  public virtual void HandleObjectTapped(ObjectTapped message) {
  }
}
