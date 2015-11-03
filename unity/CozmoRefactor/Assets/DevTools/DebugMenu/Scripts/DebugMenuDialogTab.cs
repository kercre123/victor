using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class DebugMenuDialogTab : MonoBehaviour {

  public delegate void DebugMenuTabHandler(int idTab);
  public event DebugMenuTabHandler OnTabTapped;

  [SerializeField]
  private Text buttonLabel_;

  private int id_;

	public void Initialize(int tabId, string tabName){
    id_ = tabId;
    buttonLabel_.text = tabName;
  }
	
	public void OnTabButtonTapped(){
    if (OnTabTapped != null) {
      OnTabTapped(id_);
    }
  }
}
