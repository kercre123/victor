using Anki.UI;
using System;
using UnityEngine;

public class AndroidNetworkCell : MonoBehaviour {

  [SerializeField]
  private AnkiTextLabel _Label;

  [SerializeField]
  private AnkiButton _Button;

  [SerializeField]
  private GameObject _SelectedHighlight;

  public void Init(string labelText, UnityEngine.Events.UnityAction buttonAction) {
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
