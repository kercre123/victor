using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class DevHubWorldButton : MonoBehaviour {
  
  public delegate void DevButtonClickedHandler(GameBase miniGameClicked);

  public event DevButtonClickedHandler OnDevButtonClicked;

  private void RaiseButtonClicked(GameBase minigame) {
    if (OnDevButtonClicked != null) { 
      OnDevButtonClicked(minigame);
    }
  }

  [SerializeField]
  private Button _ButtonScript;

  [SerializeField]
  private Text _ButtonLabel;

  private GameBase _Minigame;

  public void Initialize(GameBase minigame) {
    _Minigame = minigame;

    string titleKey = string.Format("#{0}.title", minigame.GameId);
    _ButtonLabel.text = titleKey;
    gameObject.name = string.Format("{0}: {1}", minigame.GameId, _ButtonLabel.text);

    _ButtonScript.onClick.AddListener(HandleButtonClicked);
  }

  private void OnDestroy() {
    _ButtonScript.onClick.RemoveListener(HandleButtonClicked);
  }

  private void HandleButtonClicked() {
    RaiseButtonClicked(_Minigame);
  }
}
