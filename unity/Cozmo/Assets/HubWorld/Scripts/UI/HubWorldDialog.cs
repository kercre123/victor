using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class HubWorldDialog : BaseDialog {

  public delegate void ButtonClickedHandler(GameBase miniGameClicked);

  public event ButtonClickedHandler OnButtonClicked;

  [SerializeField]
  private RectTransform _ButtonContainer;

  [SerializeField]
  private ScrollRect _ScrollRect;

  [SerializeField]
  private TextAsset _LevelJSON;

  public void Initialize() {
    // TODO: Parse _LevelJSON and create a bunch of buttons and add it to the list.
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }

  private void HandleOnDevButtonClicked(GameBase minigame) {
    if (OnButtonClicked != null) {
      OnButtonClicked(minigame);
    }
  }
}
