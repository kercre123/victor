using UnityEngine;
using Cozmo.UI;

namespace FaceEnrollment {
  public class FaceDetailsShelfContent : MonoBehaviour {
    public System.Action EraseFacePressed;

    [SerializeField]
    private CozmoButton _EraseFaceButton;

    private void Awake() {
      _EraseFaceButton.Initialize(() => {
        if (EraseFacePressed != null) {
          EraseFacePressed();
        }
      }, "erase_face_button", "face_details_shelf_content");
    }
  }
}