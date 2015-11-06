using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class DevHubWorldButton : MonoBehaviour {
  
  public delegate void DevButtonClickedHandler(GameBase miniGameClicked);
  public event DevButtonClickedHandler OnDevButtonClicked;
  private void RaiseButtonClicked(GameBase minigame){
    if (OnDevButtonClicked != null) {
      OnDevButtonClicked(minigame);
    }
  }
  
  [SerializeField]
  private Button buttonScript_;

	[SerializeField]
  private Text buttonLabel_;

  private GameBase minigame_;

  public void Initialize(GameBase minigame){
    minigame_ = minigame;
    buttonLabel_.text = minigame.GameName;
    buttonScript_.onClick.AddListener(OnButtonClicked);
  }

  private void OnDestroy(){
    buttonScript_.onClick.RemoveListener(OnButtonClicked);
  }

  private void OnButtonClicked(){
    RaiseButtonClicked(minigame_);
  }
}
