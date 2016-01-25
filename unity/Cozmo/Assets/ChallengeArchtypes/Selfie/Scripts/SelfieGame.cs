using UnityEngine;
using System.Collections;
using Selfie;

public class SelfieGame : GameBase {

  [SerializeField]
  private GameObject _CameraPanelPrefab;

  private CameraPanel _CameraPanel;

  private SelfieGameConfig _Config;

  public int CountdownTimer { get { return _Config.CountdownTimer; } }

  protected override void Initialize(MinigameConfigBase minigameConfig) {
    _Config = (SelfieGameConfig)minigameConfig;

    if (WebCamTexture.devices == null || WebCamTexture.devices.Length == 0) {
      RaiseMiniGameQuit();
      return;
    }
    var go = GameObject.Instantiate<GameObject>(_CameraPanelPrefab);
    go.transform.SetParent(SharedMinigameViewInstanceParent, false);
    _CameraPanel = go.GetComponent<CameraPanel>();

    _StateMachine.SetNextState(new PickupCozmoState());
  }

  public void TakePhoto() {
    _CameraPanel.TakePhoto();
  }

  public void PrepareForPhoto() {
    _CameraPanel.PrepareForPhoto();
  }

  protected override void CleanUpOnDestroy() {   
    if (_CameraPanel) {
      GameObject.Destroy(_CameraPanel.gameObject);
    }
  }
}
