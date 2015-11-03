using UnityEngine;
using UnityEngine.UI;
using System.Collections;

/// <summary>
/// on screen text to help debug any issues with the ActionSlider
/// </summary>
public class SliderText : MonoBehaviour {
  [SerializeField] private Text text_pressed;
  [SerializeField] private Text text_upLastFrame;
  [SerializeField] private Text text_downLastFrame;
  [SerializeField] private Text text_currentAction;

  private const string prefix_pressed = "Pressed: ";
  private const string prefix_upLastFrame = "UpLastFrame: ";
  private const string prefix_downLastFrame = "DownLastFrame: ";
  private const string prefix_currentAction = "Action: ";

  private ActionSliderPanel actionSliderPanel;

  private void OnEnable() { 
    if (ActionSliderPanel.instance != null) {
      actionSliderPanel = ActionSliderPanel.instance as ActionSliderPanel;
    }
  }

  private void Update() {
    if (actionSliderPanel == null) {
      if (ActionSliderPanel.instance != null) {
        actionSliderPanel = ActionSliderPanel.instance as ActionSliderPanel;

        if (actionSliderPanel == null) {
          gameObject.SetActive(false);

          return;
        }
      }

      return;
    }

    text_pressed.text = prefix_pressed + actionSliderPanel.actionSlider.Pressed;
    text_upLastFrame.text = prefix_upLastFrame + actionSliderPanel.upLastFrame;
    text_downLastFrame.text = prefix_downLastFrame + actionSliderPanel.downLastFrame;
    text_currentAction.text = prefix_currentAction + actionSliderPanel.actionSlider.currentAction.mode;
  }
}
