using UnityEngine;
using System.Collections;

public class FollowCubeGamePanel : BaseDialog {

  [SerializeField]
  private UnityEngine.UI.Button slowSpeedButton_;

  [SerializeField]
  private UnityEngine.UI.Button fastSpeedButton_;

  [SerializeField]
  private UnityEngine.UI.Button demonSpeedButton_;

  public delegate void FollowCubeGameButtonHandler();

  public FollowCubeGameButtonHandler OnSlowSpeedPressed;
  public FollowCubeGameButtonHandler OnFastSpeedPressed;
  public FollowCubeGameButtonHandler OnDemonSpeedPressed;

  // Use this for initialization
  void Start() {
    slowSpeedButton_.onClick.AddListener(OnSlowButton);
    fastSpeedButton_.onClick.AddListener(OnFastButton);
    demonSpeedButton_.onClick.AddListener(OnDemonButton);
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
