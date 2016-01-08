using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System;

public class TimelineEntry : MonoBehaviour {

  [SerializeField]
  private Image _Fillbar;

  [SerializeField]
  private GameObject _TimelineNodeActive;

  [SerializeField]
  private GameObject _TimelineNodeInactive;

  [SerializeField]
  private Button _FillbarButton;

  [SerializeField]
  private Button _TimelineNodeButton;

  private DateTime _Date;

  public event Action<DateTime> OnSelect;

  private void Awake() {
    _FillbarButton.onClick.AddListener(HandleClick);
    _TimelineNodeButton.onClick.AddListener(HandleClick);
  }
    
  private void HandleClick() {
    if (OnSelect != null) {
      OnSelect(_Date);
    }
  }

  public void Inititialize(DateTime date, bool active, float progress) {
    _Date = date;
    _Fillbar.fillAmount = progress;
    _TimelineNodeActive.SetActive(active);
    _TimelineNodeInactive.SetActive(!active);
  }
}
