using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;
using System;
using System.Collections;


/// <summary>
/// this component manages a button within the GameMenu that can load a specific game scene
/// </summary>
public class LevelSelectButton : MonoBehaviour {

  [SerializeField] Text text_ID;
  [SerializeField] Button button;
  [SerializeField] Image imagePreview;
  [SerializeField] Image[] fullStars;

  public void Initialize(string id, Sprite preview, int stars, bool interactable, UnityAction clickListener) {
    text_ID.text = id;
    imagePreview.sprite = preview; 

    for (int i = 0; i < fullStars.Length; i++) {
      fullStars[i].gameObject.SetActive(i < stars);
    }

    button.interactable = interactable;

    if (interactable) {
      button.onClick.AddListener(clickListener);
    }

  }

}
