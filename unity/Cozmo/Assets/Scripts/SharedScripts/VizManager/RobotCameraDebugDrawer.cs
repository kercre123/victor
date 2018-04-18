using UnityEngine;
using UnityEngine.UI;

namespace Anki.Cozmo.Viz {
  public class RobotCameraDebugDrawer : MonoBehaviour {

    [SerializeField]
    private RawImage _CameraImage;
    [SerializeField]
    private RawImage _OverlayImageWithFrame;

    private bool _Initialized = false;

    void Start() {
      VizManager.Enabled = true;
    }

    void OnDestroy() {
      VizManager.Enabled = false;
    }

    // Update is called once per frame
    void Update() {
      if (_Initialized) {
        return;
      }

      if (VizManager.Instance != null && VizManager.Instance.RobotCameraImage != null) {
        _CameraImage.texture = VizManager.Instance.RobotCameraImage;
        _OverlayImageWithFrame.texture = VizManager.Instance.RobotCameraOverlay;
        _Initialized = true;
      }
    }
  }
}
