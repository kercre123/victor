using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class CozmoStatus : MonoBehaviour {

  [SerializeField] LayoutBlock2d carriedBlock2d;
  [SerializeField] ChangeCubeModeButton button_change;
  [SerializeField] CanvasGroup statusFrameCanvasGroup;

  public static CozmoStatus instance = null;

  Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  float fadeTimer = 0f;
  float startingAlpha = 0f;
  bool fadeIn = true;

  void OnEnable() {
    if(instance != null && instance != this) {
      GameObject.Destroy(gameObject);
      return;
    }
    instance = this;
  }

  void OnDisable() {
    if(instance == this) instance = null;
  }

  void Update () {
    if(robot == null) return;

    if(robot.carryingObject == null) {
      carriedBlock2d.gameObject.SetActive(false);
      button_change.gameObject.SetActive(false);
    }
    else {
      carriedBlock2d.Initialize(robot.carryingObject);
      carriedBlock2d.gameObject.SetActive(true);
      button_change.gameObject.SetActive(false); //robot.carryingObject.isActive);
    }

    if(fadeTimer > 0f) {
      fadeTimer = Mathf.Max( 0f, fadeTimer - Time.deltaTime);

      if(fadeIn) {
        statusFrameCanvasGroup.alpha = Mathf.Lerp(startingAlpha, 1f, 1f - fadeTimer);
      }
      else {
        statusFrameCanvasGroup.alpha = Mathf.Lerp(startingAlpha, 0.4f, 1f - fadeTimer);
      }
    }

  }

  public void FadeIn() {
    if(fadeTimer > 0f && fadeIn) return;
    fadeTimer = 1f;
    fadeIn = true;
    startingAlpha = statusFrameCanvasGroup.alpha;
  }

  public void FadeOut() {
    if(fadeTimer > 0f && !fadeIn) return;
    fadeTimer = 1f;
    fadeIn = false;
    startingAlpha = statusFrameCanvasGroup.alpha;
  }
}
