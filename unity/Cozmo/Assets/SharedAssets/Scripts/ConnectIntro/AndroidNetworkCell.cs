using Anki.UI;
using System;
using UnityEngine;

public class AndroidNetworkCell : MonoBehaviour {

  [SerializeField]
  private AnkiTextLabel _Label;

  [SerializeField]
  private AnkiButton _Button;

  private Color _DefaultColor;

  public void Init(string labelText, UnityEngine.Events.UnityAction buttonAction) {
    _Button.onClick.RemoveAllListeners();
    _Button.Initialize(buttonAction, "select", "android.network");
    _Label.text = labelText;
    _DefaultColor = _Label.color;
  }

  public string text { get { return _Label.text; } }

  public void Select() {
    _Label.color = Color.red;
  }

  public void Deselect() {
    _Label.color = _DefaultColor;
  }
}
