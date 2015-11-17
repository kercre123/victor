using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class EmotionPane : MonoBehaviour {

  [SerializeField]
  private Slider _Slider;

  [SerializeField]
  private Text _EmotionValue;

  [SerializeField]
  private Dropdown _EmotionSelect;

  private void Start() {
    _Slider.minValue = 0;
    _Slider.maxValue = 1;
    _Slider.value = 0.5f;

    _EmotionSelect.options.Clear();

    _EmotionSelect.onValueChanged.AddListener(HandleDropDownChanged);
    _Slider.onValueChanged.AddListener(HandleSliderChanged);

    // populate with known emotions.
    Robot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      int emoCount = (int)EmotionType.Count;
      for (int i = 0; i < emoCount; ++i) {
        EmotionType emoEnum = (EmotionType)i;
        _EmotionSelect.options.Add(new Dropdown.OptionData(emoEnum.ToString()));
      }
    }
    // Init values and selections where they need to be. ( this will trigger an HandleDropDownChanged )
    _EmotionSelect.value = 1;
  }

  private void HandleSliderChanged(float newValue) {
    //DAS.Info("EmotionPane", "HandleSliderChanged: " + newValue);
    Robot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      EmotionType emoType = (EmotionType)(_EmotionSelect.value);
      robot.SetEmotion(emoType, newValue);
      _EmotionValue.text = newValue.ToString("F2");
    }
  }

  private void HandleDropDownChanged(int newIndex) {
    //DAS.Info("EmotionPane", "HandleDropdown: " + newIndex);
    // Sets the slider and label at the correct emotion value
    Robot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      float emoVal = robot.EmotionValues[newIndex];
      _EmotionValue.text = emoVal.ToString("F2");
      _Slider.value = emoVal;
    }
  }
}
