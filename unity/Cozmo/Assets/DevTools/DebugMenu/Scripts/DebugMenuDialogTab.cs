using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class DebugMenuDialogTab : MonoBehaviour {

  public delegate void DebugMenuTabHandler(int idTab);

  public event DebugMenuTabHandler OnTabTapped;

  [SerializeField]
  private Text _ButtonLabel;

  private int _Id;

  public void Initialize(int tabId, string tabName) {
    _Id = tabId;
    _ButtonLabel.text = tabName;
  }

  public void OnTabButtonTapped() {
    if (OnTabTapped != null) {
      OnTabTapped(_Id);
    }
  }
}
