using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class DevHubWorldButton : MonoBehaviour {
  
  public delegate void DevButtonClickedHandler(GameBase miniGameClicked);
  public event DevButtonClickedHandler OnDevButtonClicked;
  private void RaiseButtonClicked(GameBase minigame){
    if (OnDevButtonClicked != null) {
    }
  }
  
  [SerializeField]
  private Button buttonScript_;

	[SerializeField]
  private Text buttonLabel_;

  private GameBase minigame_;

  public void Initialize(GameBase minigame){
  }

  private void OnButtonClicked(){
  }
}
