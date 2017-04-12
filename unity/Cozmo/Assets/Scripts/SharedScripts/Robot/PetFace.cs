using UnityEngine;
using G2U = Anki.Cozmo.ExternalInterface;

public class PetFace : IVisibleInCamera {
  private const uint _kFindPetFaceTimeoutFrames = 1;
  private int _ConsecutiveVisionFramesNotSeen = 0;

  public delegate void InFieldOfViewStateChangedHandler(PetFace faceChanged, bool newState);

  public event InFieldOfViewStateChangedHandler InFieldOfViewStateChanged;

  public long PetID { get; private set; }

  // There is no way for engine to track the position of a pet's face
  public Vector3? VizWorldPosition { get { return null; } }

  public uint LastSeenEngineTimestamp { get; private set; }

  private Anki.Vision.PetType _PetType; // PetType
  public Anki.Vision.PetType PetType { get { return _PetType; } }

  // Faces are created when they are seen and destroyed when not seen for a while
  private bool _IsInFieldOfView = true;
  public bool IsInFieldOfView {
    get { return _IsInFieldOfView; }
    private set {
      if (_IsInFieldOfView != value) {
        _IsInFieldOfView = value;
        if (InFieldOfViewStateChanged != null) {
          InFieldOfViewStateChanged(this, _IsInFieldOfView);
        }
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
      return (LocalizationKeys.kDroneModePetLabel);
    }
  }
  public string ReticleLabelStringArg {
    get {
      return "";
    }
  }

  public PetFace(G2U.RobotObservedPet message) {
    UpdateInfo(message);
  }

  public static implicit operator long(PetFace petFaceObject) {
    if (petFaceObject == null)
      return -1;

    return petFaceObject.PetID;
  }

  public void UpdateInfo(G2U.RobotObservedPet message) {
    PetID = message.petID;
    LastSeenEngineTimestamp = message.timestamp;
    VizRect = new Rect(message.img_rect.x_topLeft, message.img_rect.y_topLeft, message.img_rect.width, message.img_rect.height);
    _PetType = message.petType;
    IsInFieldOfView = true;
    _ConsecutiveVisionFramesNotSeen = 0;
  }

  public void MarkNotVisibleThisFrame() {
    // If not seen frame count of cube is greater than limit, mark object NotVisible
    _ConsecutiveVisionFramesNotSeen++;
    if (_ConsecutiveVisionFramesNotSeen > _kFindPetFaceTimeoutFrames) {
      IsInFieldOfView = false;
    }
  }
}
