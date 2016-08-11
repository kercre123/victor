using UnityEngine;
using UnityEngine.UI;
using System.Collections.Generic;
using Anki.UI;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeCameraFeed : MonoBehaviour {

    [SerializeField]
    private RawImage _CameraFeedImage;
    private ImageReceiver _ImageProcessor;

    [SerializeField]
    private RectTransform _CameraFeedImageContainer;

    [SerializeField]
    private RectTransform _DroneModeReticlePrefab;

    [SerializeField]
    private RectTransform _FocusedObjectFrameContainer;

    [SerializeField]
    private AnkiTextLabel _FocusedObjectTextLabel;

    private IRobot _CurrentRobot;
    private Dictionary<IVisibleInCamera, DroneModeCameraReticle> _ObjToReticle = new Dictionary<IVisibleInCamera, DroneModeCameraReticle>();
    private SimpleObjectPool<DroneModeCameraReticle> _ReticlePool;

    private Vector2 _CameraImageSize;
    private float _CameraImageScale;

    // TODO: Replace text field
    public AnkiTextLabel DebugTextField;

    public void Initialize(IRobot currentRobot) {
      _ImageProcessor = new ImageReceiver("DroneModeCameraFeedImageProcessor");
      _ImageProcessor.CaptureStream();
      _ImageProcessor.OnImageSizeChanged += HandleImageSizeChanged;
      _CurrentRobot = currentRobot;

      _ReticlePool = new SimpleObjectPool<DroneModeCameraReticle>(CreateReticle, ResetReticle, 0);
    }

    private void Update() {
      ShowDataForClosestVisibleObject();
      DebugTextField.text = FormatCurrentSeenObjects();
    }

    private void ShowDataForClosestVisibleObject() {
      IVisibleInCamera closestFocus = null;
      DroneModeCameraReticle closestReticle = null;
      float smallestDistanceSqrd = float.MaxValue;
      foreach (var kvp in _ObjToReticle) {
        kvp.Value.ShowReticleLabelText(false);
        if (kvp.Key.IsInFieldOfView) {
          float distanceSqrd = (kvp.Key.WorldPosition - _CurrentRobot.WorldPosition).sqrMagnitude;
          if (distanceSqrd < smallestDistanceSqrd) {
            smallestDistanceSqrd = distanceSqrd;
            closestReticle = kvp.Value;
            closestFocus = kvp.Key;
          }
        }
      }

      ShowTextForReticle(closestFocus, closestReticle);
    }

    private void OnDestroy() {
      _ImageProcessor.OnImageSizeChanged -= HandleImageSizeChanged;
      _ImageProcessor.Dispose();
      _ImageProcessor.DestroyTexture();
      _CurrentRobot.OnFaceAdded -= HandleOnFaceAdded;
      _CurrentRobot.OnFaceRemoved -= HandleOnFaceRemoved;
      _CurrentRobot.OnLightCubeAdded -= HandleOnCubeAdded;
      _CurrentRobot.OnLightCubeRemoved -= HandleOnCubeRemoved;
      _CurrentRobot.OnChargerAdded -= HandleOnChargerAdded;
      _CurrentRobot.OnChargerRemoved -= HandleOnChargerRemoved;

      foreach (var face in _CurrentRobot.Faces) {
        face.InFieldOfViewStateChanged -= HandleFaceInFieldOfViewChanged;
        face.OnVizRectChanged -= HandleVizRectChanged;
      }

      foreach (var cube in _CurrentRobot.LightCubes) {
        cube.Value.InFieldOfViewStateChanged -= HandleInFieldOfViewChanged;
        cube.Value.OnVizRectChanged -= HandleVizRectChanged;
      }

      if (_CurrentRobot.Charger != null) {
        _CurrentRobot.Charger.InFieldOfViewStateChanged -= HandleInFieldOfViewChanged;
        _CurrentRobot.Charger.OnVizRectChanged -= HandleVizRectChanged;
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
      _CurrentRobot.OnLightCubeAdded += HandleOnCubeAdded;
      _CurrentRobot.OnLightCubeRemoved += HandleOnCubeRemoved;
      _CurrentRobot.OnChargerAdded += HandleOnChargerAdded;
      _CurrentRobot.OnChargerRemoved += HandleOnChargerRemoved;

      foreach (var face in _CurrentRobot.Faces) {
        CreateFaceReticle(face);
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

    private void CreateObservedObjectReticle(ObservedObject observedObject) {
      observedObject.InFieldOfViewStateChanged += HandleInFieldOfViewChanged;
      CreateReticleIfVisible(observedObject);
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
      if (reticleFocus.IsInFieldOfView && !_ObjToReticle.ContainsKey(reticleFocus)) {
        DroneModeCameraReticle newReticle = _ReticlePool.GetObjectFromPool();
        _ObjToReticle.Add(reticleFocus, newReticle);
        reticleFocus.OnVizRectChanged += HandleVizRectChanged;

        PositionReticle(newReticle, reticleFocus.VizRect);
      }
    }

    private void RemoveReticle(IVisibleInCamera reticleFocus) {
      DroneModeCameraReticle toRemove;
      if (_ObjToReticle.TryGetValue(reticleFocus, out toRemove)) {
        _ObjToReticle.Remove(reticleFocus);
        reticleFocus.OnVizRectChanged -= HandleVizRectChanged;
        _ReticlePool.ReturnToPool(toRemove);

        DAS.Info("DroneModeCameraFeed.RemoveReticle", toRemove.ToString());
      }
    }

    private void HandleVizRectChanged(IVisibleInCamera reticleFocus, Rect newVizRect) {
      DroneModeCameraReticle cachedReticle;
      if (_ObjToReticle.TryGetValue(reticleFocus, out cachedReticle)) {
        PositionReticle(cachedReticle, newVizRect);
      }
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
      // Create the text label as a child under the parent container for the pool
      GameObject newLabelObject = UIManager.CreateUIElement(_DroneModeReticlePrefab.gameObject, _CameraFeedImageContainer.transform);
      newLabelObject.SetActive(false);
      DroneModeCameraReticle reticleScript = newLabelObject.GetComponent<DroneModeCameraReticle>();
      reticleScript.ReticleLabel = "";
      reticleScript.ShowReticleLabelText(false);
      return reticleScript;
    }

    private void ResetReticle(DroneModeCameraReticle toReset, bool spawned) {
      if (!spawned) {
        toReset.gameObject.SetActive(false);

        // TODO reset text?
      }
    }

    private void ShowTextForReticle(IVisibleInCamera reticleFocus, DroneModeCameraReticle reticle) {
      if (reticle != null) {
        if (!string.IsNullOrEmpty(reticleFocus.ReticleLabelLocKey)) {
          string text = "";
          if (!string.IsNullOrEmpty(reticleFocus.ReticleLabelStringArg)) {
            text = Localization.GetWithArgs(reticleFocus.ReticleLabelLocKey, reticleFocus.ReticleLabelStringArg);
          }
          else {
            text = Localization.Get(reticleFocus.ReticleLabelLocKey);
          }
          _FocusedObjectFrameContainer.gameObject.SetActive(true);
          _FocusedObjectTextLabel.text = text;
          reticle.ReticleLabel = text;
          reticle.ShowReticleLabelText(true);
        }
        else {
          _FocusedObjectFrameContainer.gameObject.SetActive(false);
          reticle.ShowReticleLabelText(false);
        }
      }
      else {
        _FocusedObjectFrameContainer.gameObject.SetActive(false);
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
