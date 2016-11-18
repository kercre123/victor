using UnityEngine;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.UI;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeCameraFeed : MonoBehaviour {
    public delegate void VisibleInCameraHandler(IVisibleInCamera newFocusedObject);
    public event VisibleInCameraHandler OnCurrentFocusChanged;

    [SerializeField]
    private RawImage _CameraFeedImage;
    private ImageReceiver _ImageProcessor;

    [SerializeField]
    private RectTransform _CameraFeedImageContainer;

    [SerializeField]
    private RectTransform _DroneModeReticlePrefab;

    [SerializeField]
    private Image _FocusedObjectFrameImage;

    [SerializeField]
    private AnkiTextLabel _FocusedObjectTextLabel;

    [SerializeField]
    private Image[] _TopFrameImages;

    [SerializeField]
    private Image[] _BottomFrameImages;

    [SerializeField]
    private Image[] _GradientImages;

    private IRobot _CurrentRobot;
    private Dictionary<IVisibleInCamera, DroneModeCameraReticle> _ObjToReticle = new Dictionary<IVisibleInCamera, DroneModeCameraReticle>();
    private SimpleObjectPool<DroneModeCameraReticle> _ReticlePool;

    // allow reticles to move smoothly
    private const int kReticleSmoothingCount = 5;
    private Dictionary<IVisibleInCamera, List<Rect>> _SmoothingReticleMap = new Dictionary<IVisibleInCamera, List<Rect>>();

    private Vector2 _CameraImageSize;
    private float _CameraImageScale;

    private bool _ShowReticles = true;
    public bool AllowChangeFocus { get; set; }

    // TODO: Replace text field
    public AnkiTextLabel DebugTextField;

    private DroneModeColorSet _CurrentColorSet;

    private IVisibleInCamera _CurrentlyFocusedObject;
    public IVisibleInCamera CurrentlyFocusedObject {
      get { return _CurrentlyFocusedObject; }
      private set {
        if (value != _CurrentlyFocusedObject) {
          _CurrentlyFocusedObject = value;
          if (OnCurrentFocusChanged != null) {
            OnCurrentFocusChanged(_CurrentlyFocusedObject);
          }
        }
      }
    }

    public void Initialize(IRobot currentRobot) {
      _ImageProcessor = new ImageReceiver("DroneModeCameraFeedImageProcessor");
      _ImageProcessor.CaptureStream();
      _ImageProcessor.OnImageSizeChanged += HandleImageSizeChanged;
      _CurrentRobot = currentRobot;

      _ReticlePool = new SimpleObjectPool<DroneModeCameraReticle>(CreateReticle, ResetReticle, 0);
      _FocusedObjectFrameImage.gameObject.SetActive(false);
      AllowChangeFocus = true;
    }

    private void Update() {
      if (_ShowReticles && AllowChangeFocus) {
        ShowDataForClosestVisibleObject();
        DebugTextField.text = FormatCurrentSeenObjects();
      }
    }

    private void ShowDataForClosestVisibleObject() {
      IVisibleInCamera closestFocus = null;
      DroneModeCameraReticle closestReticle = null;
      float smallestDistanceSqrd = float.MaxValue;
      foreach (var kvp in _ObjToReticle) {
        kvp.Value.ShowReticleLabelText(false);
        if (kvp.Key.IsInFieldOfView) {
          float distanceSqrd = 0f;
          if (kvp.Key.VizWorldPosition.HasValue) {
            distanceSqrd = (kvp.Key.VizWorldPosition.Value - _CurrentRobot.WorldPosition).sqrMagnitude;
          }
          if (distanceSqrd < smallestDistanceSqrd) {
            smallestDistanceSqrd = distanceSqrd;
            closestReticle = kvp.Value;
            closestFocus = kvp.Key;
          }

          // For PetFace debugging
          if (kvp.Key is PetFace) {
            ShowTextForReticle(kvp.Key, kvp.Value);
          }
        }
      }

      if (closestFocus != CurrentlyFocusedObject) {
        CurrentlyFocusedObject = closestFocus;
        ShowTextForReticle(closestFocus, closestReticle);
      }
    }

    private void OnDestroy() {
      _ImageProcessor.OnImageSizeChanged -= HandleImageSizeChanged;
      _ImageProcessor.Dispose();
      _ImageProcessor.DestroyTexture();

      if (_CurrentRobot != null) {
        _CurrentRobot.OnFaceAdded -= HandleOnFaceAdded;
        _CurrentRobot.OnFaceRemoved -= HandleOnFaceRemoved;
        foreach (var face in _CurrentRobot.Faces) {
          face.InFieldOfViewStateChanged -= HandleFaceInFieldOfViewChanged;
          face.OnVizRectChanged -= HandleVizRectChanged;
        }

        _CurrentRobot.OnPetFaceAdded -= HandlePetFaceAdded;
        _CurrentRobot.OnPetFaceRemoved -= HandlePetFaceRemoved;
        foreach (var petFace in _CurrentRobot.PetFaces) {
          petFace.InFieldOfViewStateChanged -= HandlePetFaceInFieldOfViewChanged;
          petFace.OnVizRectChanged -= HandleVizRectChanged;
        }

        _CurrentRobot.OnLightCubeAdded -= HandleOnCubeAdded;
        _CurrentRobot.OnLightCubeRemoved -= HandleOnCubeRemoved;
        foreach (var cube in _CurrentRobot.LightCubes) {
          cube.Value.InFieldOfViewStateChanged -= HandleInFieldOfViewChanged;
          cube.Value.OnVizRectChanged -= HandleVizRectChanged;
        }

        _CurrentRobot.OnChargerAdded -= HandleOnChargerAdded;
        _CurrentRobot.OnChargerRemoved -= HandleOnChargerRemoved;
        if (_CurrentRobot.Charger != null) {
          _CurrentRobot.Charger.InFieldOfViewStateChanged -= HandleInFieldOfViewChanged;
          _CurrentRobot.Charger.OnVizRectChanged -= HandleVizRectChanged;
        }
      }
    }

    public void SetCameraFeedColor(DroneModeColorSet colorSet, bool showReticles) {
      _CurrentColorSet = colorSet;
      _CameraFeedImage.material.SetColor("_TopColor", colorSet.TopCameraColor);
      _CameraFeedImage.material.SetColor("_BottomColor", colorSet.BottomCameraColor);

      foreach (var topImage in _TopFrameImages) {
        topImage.color = colorSet.TopCameraColor;
      }

      foreach (var bottomImage in _BottomFrameImages) {
        bottomImage.color = colorSet.BottomCameraColor;
      }

      foreach (var gradientImage in _GradientImages) {
        gradientImage.material.SetColor("_TopColor", colorSet.TopGradientColor);
        gradientImage.material.SetColor("_BottomColor", colorSet.BottomGradientColor);
      }

      _ShowReticles = showReticles;
      if (showReticles) {
        CreateReticlesIfVisible();
      }
      else {
        _FocusedObjectFrameImage.gameObject.SetActive(false);

        IVisibleInCamera[] toRemove = new IVisibleInCamera[_ObjToReticle.Count];
        _ObjToReticle.Keys.CopyTo(toRemove, 0);
        for (int i = 0; i < toRemove.Length; i++) {
          RemoveReticle(toRemove[i]);
        }

        _CurrentlyFocusedObject = null;
      }
    }

    private void HandleImageSizeChanged(float width, float height) {
      _CameraImageSize = new Vector2(width, height);
      _CameraFeedImage.rectTransform.sizeDelta = _CameraImageSize;
      _CameraImageScale = (_CameraFeedImageContainer.sizeDelta.x / width);
      _CameraFeedImage.rectTransform.localScale = new Vector3(_CameraImageScale, _CameraImageScale, _CameraImageScale);
      _CameraFeedImage.texture = _ImageProcessor.Image;

      _CurrentRobot.OnFaceAdded += HandleOnFaceAdded;
      _CurrentRobot.OnFaceRemoved += HandleOnFaceRemoved;

      _CurrentRobot.OnPetFaceAdded += HandlePetFaceAdded;
      _CurrentRobot.OnPetFaceRemoved += HandlePetFaceRemoved;

      _CurrentRobot.OnLightCubeAdded += HandleOnCubeAdded;
      _CurrentRobot.OnLightCubeRemoved += HandleOnCubeRemoved;

      _CurrentRobot.OnChargerAdded += HandleOnChargerAdded;
      _CurrentRobot.OnChargerRemoved += HandleOnChargerRemoved;

      CreateReticlesIfVisible();
    }

    private void CreateReticlesIfVisible() {
      foreach (var face in _CurrentRobot.Faces) {
        CreateFaceReticle(face);
      }

      foreach (var petFace in _CurrentRobot.PetFaces) {
        CreatePetFaceReticle(petFace);
      }

      foreach (var cube in _CurrentRobot.LightCubes) {
        CreateObservedObjectReticle(cube.Value);
      }

      if (_CurrentRobot.Charger != null) {
        CreateObservedObjectReticle(_CurrentRobot.Charger);
      }
    }

    private void CreateFaceReticle(Face face) {
      face.InFieldOfViewStateChanged += HandleFaceInFieldOfViewChanged;
      CreateReticleIfVisible(face);
    }
    private void HandleOnFaceAdded(Face face) {
      // Face is added when visible
      CreateFaceReticle(face);
    }

    private void HandleOnFaceRemoved(Face face) {
      // Face is removed when not seen for a while 
      face.InFieldOfViewStateChanged -= HandleFaceInFieldOfViewChanged;
      RemoveReticle(face);
    }

    private void HandleFaceInFieldOfViewChanged(Face face, bool isInFieldOfView) {
      // If visible spawn reticle and add to dictionary
      if (isInFieldOfView) {
        CreateReticleIfVisible(face);
      }
      else {
        RemoveReticle(face);
      }
    }

    private void CreatePetFaceReticle(PetFace petFace) {
      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.DroneModePetReticlesEnabled) {
        petFace.InFieldOfViewStateChanged += HandlePetFaceInFieldOfViewChanged;
        CreateReticleIfVisible(petFace);
      }
    }

    private void HandlePetFaceAdded(PetFace petFace) {
      // Face is added when visible
      CreatePetFaceReticle(petFace);
    }

    private void HandlePetFaceRemoved(PetFace petFace) {
      // Face is removed when not seen for a while 
      petFace.InFieldOfViewStateChanged -= HandlePetFaceInFieldOfViewChanged;
      RemoveReticle(petFace);
    }

    private void HandlePetFaceInFieldOfViewChanged(PetFace petFace, bool isInFieldOfView) {
      // If visible spawn reticle and add to dictionary
      if (isInFieldOfView) {
        CreateReticleIfVisible(petFace);
      }
      else {
        RemoveReticle(petFace);
      }
    }

    private void CreateObservedObjectReticle(ObservedObject observedObject) {
      observedObject.InFieldOfViewStateChanged += HandleInFieldOfViewChanged;
      CreateReticleIfVisible(observedObject);
    }

    private void HandleOnCubeAdded(LightCube cube) {
      CreateObservedObjectReticle(cube);
    }

    private void HandleOnCubeRemoved(LightCube cube) {
      cube.InFieldOfViewStateChanged -= HandleInFieldOfViewChanged;
      RemoveReticle(cube);
    }

    private void HandleOnChargerAdded(ObservedObject charger) {
      CreateObservedObjectReticle(charger);
    }

    private void HandleOnChargerRemoved(ObservedObject charger) {
      charger.InFieldOfViewStateChanged -= HandleInFieldOfViewChanged;
      RemoveReticle(charger);
    }

    private void HandleInFieldOfViewChanged(ObservedObject observedObject,
                                            ObservedObject.InFieldOfViewState oldState,
                                            ObservedObject.InFieldOfViewState newState) {

      // If visible spawn reticle and add to dictionary
      if (newState == ObservedObject.InFieldOfViewState.Visible) {
        CreateReticleIfVisible(observedObject);
      }
      else {
        RemoveReticle(observedObject);
      }
    }

    private void CreateReticleIfVisible(IVisibleInCamera reticleFocus) {
      if (_ShowReticles && reticleFocus.IsInFieldOfView && !_ObjToReticle.ContainsKey(reticleFocus)) {
        DroneModeCameraReticle newReticle = _ReticlePool.GetObjectFromPool();
        if (newReticle != null) {
          _ObjToReticle.Add(reticleFocus, newReticle);
          reticleFocus.OnVizRectChanged += HandleVizRectChanged;

          PositionReticle(newReticle, reticleFocus.VizRect);
          newReticle.SetColor(_CurrentColorSet.FocusFrameColor, _CurrentColorSet.ButtonColor);
        }
      }
    }

    private void RemoveReticle(IVisibleInCamera reticleFocus) {
      DroneModeCameraReticle toRemove;
      if (_ObjToReticle.TryGetValue(reticleFocus, out toRemove)) {
        _ObjToReticle.Remove(reticleFocus);
        _SmoothingReticleMap.Remove(reticleFocus);
        reticleFocus.OnVizRectChanged -= HandleVizRectChanged;
        _ReticlePool.ReturnToPool(toRemove);

        DAS.Info("DroneModeCameraFeed.RemoveReticle", toRemove.ToString());
      }
    }

    private void HandleVizRectChanged(IVisibleInCamera reticleFocus, Rect newVizRect) {
      DroneModeCameraReticle cachedReticle;
      if (_ObjToReticle.TryGetValue(reticleFocus, out cachedReticle)) {
        Rect averagedVizRect = CalculateAverageVizRect(reticleFocus, newVizRect);
        PositionReticle(cachedReticle, averagedVizRect);
      }
    }

    private Rect CalculateAverageVizRect(IVisibleInCamera reticleFocus, Rect newVizRect) {
      Rect averagedVizRect = newVizRect;
      List<Rect> averagedList;

      if (_SmoothingReticleMap.TryGetValue(reticleFocus, out averagedList)) {
        // update the list
        averagedList.Add(newVizRect);
        int listLen = averagedList.Count;
        if (listLen > kReticleSmoothingCount) {
          averagedList.RemoveAt(0);
          listLen--;
        }

        // average together the rect values
        float allX = 0;
        float allY = 0;
        float allHeight = 0;
        float allWidth = 0;
        foreach (Rect entry in averagedList) {
          allX += entry.x;
          allY += entry.y;
          allHeight += entry.height;
          allWidth += entry.width;
        }
        Vector2 rectSize = new Vector2(allX / listLen, allY / listLen);
        Vector2 rectPosition = new Vector2(allWidth / listLen, allHeight / listLen);
        averagedVizRect = new Rect(rectSize, rectPosition);

      }
      else {
        _SmoothingReticleMap.Add(reticleFocus, new List<Rect> { newVizRect });
      }

      return averagedVizRect;
    }

    private void PositionReticle(DroneModeCameraReticle reticle, Rect vizRect) {
      reticle.SetImageSize(vizRect.width * _CameraImageScale, vizRect.height * _CameraImageScale);
      Vector2 localXY = MapVizCoordToLocalCoord(vizRect.x, vizRect.y);
      reticle.transform.localPosition = new Vector3(localXY.x, localXY.y, 0f);
      reticle.gameObject.SetActive(true);
    }

    private Vector2 MapVizCoordToLocalCoord(float vizX, float vizY) {
      Vector2 localPos = new Vector2();
      localPos.x = (vizX - _CameraImageSize.x * 0.5f) * _CameraImageScale;
      localPos.y = -(vizY - _CameraImageSize.y * 0.5f) * _CameraImageScale;
      return localPos;
    }

    private DroneModeCameraReticle CreateReticle() {
      DroneModeCameraReticle reticleScript = null;
      if (_CameraFeedImageContainer != null) {
        // Create the text label as a child under the parent container for the pool
        GameObject newLabelObject = UIManager.CreateUIElement(_DroneModeReticlePrefab.gameObject, _CameraFeedImageContainer.transform);
        newLabelObject.SetActive(false);
        reticleScript = newLabelObject.GetComponent<DroneModeCameraReticle>();
        reticleScript.ReticleLabel = "";
        reticleScript.ShowReticleLabelText(false);
      }
      return reticleScript;
    }

    private void ResetReticle(DroneModeCameraReticle toReset, bool spawned) {
      if (!spawned) {
        toReset.gameObject.SetActive(false);
      }
    }

    private void ShowTextForReticle(IVisibleInCamera reticleFocus, DroneModeCameraReticle reticle) {
      if (reticle != null) {
        if (!string.IsNullOrEmpty(reticleFocus.ReticleLabelLocKey)) {
          // Never show text on the reticle itself
          reticle.ShowReticleLabelText(false);

          string text = "";
          if (!string.IsNullOrEmpty(reticleFocus.ReticleLabelStringArg)) {
            text = Localization.GetWithArgs(reticleFocus.ReticleLabelLocKey, reticleFocus.ReticleLabelStringArg);
          }
          else {
            text = Localization.Get(reticleFocus.ReticleLabelLocKey);
          }
          _FocusedObjectFrameImage.gameObject.SetActive(true);
          _FocusedObjectTextLabel.text = text;
        }
        else {
          _FocusedObjectFrameImage.gameObject.SetActive(false);
        }
      }
      else {
        _FocusedObjectFrameImage.gameObject.SetActive(false);
      }
    }

    private string FormatCurrentSeenObjects() {
      string toString = "";
      foreach (var obj in _ObjToReticle.Keys) {
        if (obj is ObservedObject) {
          toString += ((ObservedObject)obj).ObjectType + " " + obj.VizRect + "       ";
        }
        else if (obj is Face) {
          toString += "Face ID " + ((Face)obj).ID + " " + obj.VizRect + "       ";
        }
      }
      return toString;
    }
  }
}
