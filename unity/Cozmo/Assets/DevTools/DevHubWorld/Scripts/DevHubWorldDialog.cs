using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class DevHubWorldDialog : BaseDialog {

  public delegate void DevButtonClickedHandler(GameBase miniGameClicked);

  public event DevButtonClickedHandler OnDevButtonClicked;

  private void RaiseButtonClicked(GameBase minigame) {
    if (OnDevButtonClicked != null) {
      OnDevButtonClicked(minigame);
    }
  }

  [SerializeField]
  private DevHubWorldButton _DevHubWorldButtonPrefab;

  [SerializeField]
  private RectTransform _ButtonContainer;

  [SerializeField]
  private ScrollRect _ScrollRect;

  public void Initialize(GameBase[] minigames) {
    GameObject newButton;
    DevHubWorldButton buttonScript;
    foreach (GameBase game in minigames) {
      newButton = UIManager.CreateUI(_DevHubWorldButtonPrefab.gameObject, _ButtonContainer);
      buttonScript = newButton.GetComponent<DevHubWorldButton>();
      buttonScript.Initialize(game);
      buttonScript.OnDevButtonClicked += HandleOnDevButtonClicked;
    }
    _ScrollRect.verticalNormalizedPosition = 1.0f;
  }

  protected override void CleanUp() {

  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {

  }

  private void HandleOnDevButtonClicked(GameBase miniGameClicked) {
    RaiseButtonClicked(miniGameClicked);
  }
}
