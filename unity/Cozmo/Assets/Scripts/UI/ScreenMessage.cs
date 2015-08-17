using UnityEngine;
using UnityEngine.UI;
using System.Collections;

/// <summary>
/// used for displaying information to the user on screen, with optional timer
/// </summary>
public class ScreenMessage : MonoBehaviour {

  [SerializeField] protected Text text;

  float hideTimer = 0f;

  void Awake () {
    text.text = null;
    text.gameObject.SetActive(true);
  }

  void Update() {
    if(hideTimer > 0f) {
      hideTimer -= Time.deltaTime;

      if(hideTimer <= 0f) {
        text.text = string.Empty;
      }
    }
  }

  public void ShowMessageForDuration(string message, float time_in_seconds, Color color)
  {
    ShowMessage (message, color);
    hideTimer = time_in_seconds;
  }

  public void ShowMessage(string message, Color color)
  {
    if (text == null) 
    {
      Debug.LogError("text is null for some reason");
      return;
    }
    text.text = message;
    text.color = color;
    hideTimer = 0f;
  }

  public void KillMessage()
  {
    if (text == null) 
    {
      Debug.LogError("text is null for some reason");
      return;
    }
    text.text = string.Empty;
    hideTimer = 0f;
  }

  public void TurnOffText(float time_in_seconds)
  {
    hideTimer = time_in_seconds;
  }
}
