using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System;
using DataPersistence;

public class TimelineEntry : MonoBehaviour {

  [SerializeField]
  private Image _Fillbar;

  [SerializeField]
  private GameObject _TimelineNodeBonus;

  [SerializeField]
  private GameObject _TimelineNodeActive;

  [SerializeField]
  private GameObject _TimelineNodeComplete;

  [SerializeField]
  private GameObject _TimelineNodeInactive;

  [SerializeField]
  private Button _FillbarButton;

  [SerializeField]
  private Button _TimelineNodeButton;

  private Date _Date;

  public event Action<Date> OnSelect;

  private void Awake() {
    _FillbarButton.onClick.AddListener(HandleClick);
    _TimelineNodeButton.onClick.AddListener(HandleClick);
  }

  private void HandleClick() {
    if (OnSelect != null) {
      OnSelect(_Date);
    }
  }

  public void Inititialize(Date date, bool active, float progress) {
    _Date = date;
    _Fillbar.fillAmount = progress;

    _FillbarButton.gameObject.SetActive(active);
    _TimelineNodeActive.SetActive(active && progress < 1.0f);
    _TimelineNodeComplete.SetActive(progress >= 1.0f);
    _TimelineNodeInactive.SetActive(!active);
    _TimelineNodeBonus.SetActive(progress > 1.0f);
  }
}
