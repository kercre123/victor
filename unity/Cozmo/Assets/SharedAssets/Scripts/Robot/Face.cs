using UnityEngine;
using G2U = Anki.Cozmo.ExternalInterface;

public class Face : IVisibleInCamera { // TODO Implement IHaveCameraPosition
  private const uint _kFindFaceTimeoutFrames = 1;
  private int _ConsecutiveVisionFramesNotSeen = 0;

  public delegate void InFieldOfViewStateChangedHandler(Face faceChanged, bool newState);

  public event InFieldOfViewStateChangedHandler InFieldOfViewStateChanged;

  public int ID { get; private set; }

  public uint RobotID { get; private set; }

  public Vector3? VizWorldPosition { get { return WorldPosition; } }

  public Vector3 WorldPosition { get; private set; }

  public Quaternion Rotation { get; private set; }

  public Vector3 Forward { get { return Rotation * Vector3.right; } }

  public Vector3 Right { get { return Rotation * -Vector3.up; } }

  public Vector3 Up { get { return Rotation * Vector3.forward; } }

  public uint LastSeenEngineTimestamp { get; private set; }

  // Faces are created when they are seen and destroyed when not seen for a while
  private bool _IsInFieldOfView = true;

  public bool IsInFieldOfView {
    get { return _IsInFieldOfView; }
    private set {
      _IsInFieldOfView = value;
      if (InFieldOfViewStateChanged != null) {
        InFieldOfViewStateChanged(this, _IsInFieldOfView);
      }
    }
  }

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

  public string ReticleLabelLocKey {
    get {
      string key = LocalizationKeys.kDroneModeUnknownFaceReticleLabel;
      if (!string.IsNullOrEmpty(this.Name)) {
        key = LocalizationKeys.kLabelEmptyWithArg;
      }
      return key;
    }
  }

  public string ReticleLabelStringArg {
    get {
      return this.Name;
    }
  }

  public string Name { get; private set; }

  public Face(int faceId, float world_x, float world_y, float world_z) {
    ID = faceId;
    WorldPosition = new Vector3(world_x, world_y, world_z);
    LastSeenEngineTimestamp = 0;
  }

  public Face(G2U.RobotObservedFace message) {
    UpdateInfo(message);
  }

  public static implicit operator long(Face faceObject) {
    if (faceObject == null)
      return -1;

    return faceObject.ID;
  }

  public void UpdateInfo(G2U.RobotObservedFace message) {
    RobotID = message.robotID;
    ID = message.faceID;

    Name = message.name;

    Vector3 newPos = new Vector3(message.pose.x, message.pose.y, message.pose.z);

    //dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
    WorldPosition = newPos;
    Rotation = new Quaternion(message.pose.q1, message.pose.q2, message.pose.q3, message.pose.q0);
    // Size = Vector3.one * CozmoUtil.kBlockLengthMM;

    // TopFaceNorthAngle = message.topFaceOrientation_rad + Mathf.PI * 0.5f;

    LastSeenEngineTimestamp = message.timestamp;

    VizRect = new Rect(message.img_rect.x_topLeft, message.img_rect.y_topLeft, message.img_rect.width, message.img_rect.height);

    _ConsecutiveVisionFramesNotSeen = 0;
    IsInFieldOfView = true;
  }


  // this should only be called in response to RobotChangedObservedFaceID
  public void UpdateFaceID(int id) {
    ID = id;
  }

  public void MarkNotVisibleThisFrame() {
    // If not seen frame count of cube is greater than limit, mark object NotVisible
    _ConsecutiveVisionFramesNotSeen++;
    if (_ConsecutiveVisionFramesNotSeen > _kFindFaceTimeoutFrames) {
      IsInFieldOfView = false;
    }
  }
}
