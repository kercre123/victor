using Anki.UI;
using System;
using UnityEngine;
using Cozmo.UI;

public class AndroidNetworkCell : MonoBehaviour {

  [SerializeField]
  private CozmoText _Label;

  [SerializeField]
  private CozmoButton _Button;

  [SerializeField]
  private GameObject _SelectedHighlight;

  public void Init(string labelText, UnityEngine.Events.UnityAction buttonAction) {
    _SelectedHighlight.SetActive(false);
    _Button.onClick.RemoveAllListeners();
    _Button.Initialize(buttonAction, "select", "android.network");
    _Label.text = labelText;
  }

  public string text { get { return _Label.text; } }

  public void Select() {
    _SelectedHighlight.SetActive(true);
  }

  public void Deselect() {
    _SelectedHighlight.SetActive(false);
  }
}
