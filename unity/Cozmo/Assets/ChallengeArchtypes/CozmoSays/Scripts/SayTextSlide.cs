using UnityEngine;
using System.Collections;

public class SayTextSlide : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.CozmoButton _SayTextButton;

  [SerializeField]
  private UnityEngine.UI.InputField _TextInput;

  private void Awake() {
    _SayTextButton.Initialize(HandleSayTextButton, "say_text_button", "say_text_slide");
  }

  private void HandleSayTextButton() {
    _SayTextButton.Interactable = false;
    RobotEngineManager.Instance.CurrentRobot.SayTextWithEvent(_TextInput.text, Anki.Cozmo.AnimationTrigger.MeetCozmoReEnrollmentSayName, callback: (success) => {
      _SayTextButton.Interactable = true;
    });
  }
}
