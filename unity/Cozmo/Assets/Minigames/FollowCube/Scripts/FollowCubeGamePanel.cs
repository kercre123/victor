using UnityEngine;
using System.Collections;

public class FollowCubeGamePanel : BaseDialog {

  [SerializeField]
  private UnityEngine.UI.Button _SlowSpeedButton;

  [SerializeField]
  private UnityEngine.UI.Button _FastSpeedButton;

  [SerializeField]
  private UnityEngine.UI.Button _DemonSpeedButton;

  public delegate void FollowCubeGameButtonHandler();

  public FollowCubeGameButtonHandler OnSlowSpeedPressed;
  public FollowCubeGameButtonHandler OnFastSpeedPressed;
  public FollowCubeGameButtonHandler OnDemonSpeedPressed;

  // Use this for initialization
  void Start() {
    _SlowSpeedButton.onClick.AddListener(OnSlowButton);
    _FastSpeedButton.onClick.AddListener(OnFastButton);
    _DemonSpeedButton.onClick.AddListener(OnDemonButton);
  }

  void OnSlowButton() {
    if (OnSlowSpeedPressed != null) {
      OnSlowSpeedPressed();
    }
  }

  void OnFastButton() {
    if (OnFastSpeedPressed != null) {
      OnFastSpeedPressed();
    }
  }

  void OnDemonButton() {
    if (OnDemonSpeedPressed != null) {
      OnDemonSpeedPressed();
    }
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }
}
