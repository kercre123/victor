using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using UnityEngine.UI;

namespace Anki.Cozmo.Viz {
  public class RobotCameraDebugDrawer : MonoBehaviour {

    [SerializeField]
    private RawImage _CameraImage;
    [SerializeField]
    private RawImage _OverlayImage;


    private bool _Initialized = false;

    // Update is called once per frame
    void Update () {
      if (_Initialized) {
        return;
      }

      if (VizManager.Instance != null && VizManager.Instance.RobotCameraImage != null) {
        _CameraImage.texture = VizManager.Instance.RobotCameraImage;
        _OverlayImage.texture = VizManager.Instance.RobotCameraOverlay;
        _Initialized = true;
      }
    }
  }
}
