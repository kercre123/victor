using UnityEngine;
using Cozmo.UI;

namespace FaceEnrollment {
  public class FaceEnrollmentDetailsSlide : MonoBehaviour {

    public System.Action<int, string> OnRequestReEnrollFace;

    [SerializeField]
    private CozmoButton _ReEnrollFaceButton;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _NameForFace;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _LastEnrolledText;

    public void Initialize(int faceID, string nameForFace) {

      _ReEnrollFaceButton.Initialize(() => {
        if (OnRequestReEnrollFace != null) {
          OnRequestReEnrollFace(faceID, nameForFace);
        }
      }, "re_enroll_face_button", "face_enrollment_details_slide");

      _NameForFace.text = nameForFace;
      _ReEnrollFaceButton.Text = Localization.GetWithArgs(LocalizationKeys.kFaceEnrollmentButtonScanAgain, nameForFace);

      string lastEnrolledLocKey = "faceEnrollment.label.dayCount";
      const int secondsInADay = 86400;
      int daysCount = (int)((Time.time - RobotEngineManager.Instance.CurrentRobot.EnrolledFacesLastEnrolledTime[faceID]) / secondsInADay);

      if (daysCount < 1) {
        _LastEnrolledText.text = Localization.Get(LocalizationKeys.kFaceEnrollmentLabelToday);
      }
      else {
        _LastEnrolledText.text = Localization.GetCountLabel(lastEnrolledLocKey, daysCount);
      }


    }
  }

}
