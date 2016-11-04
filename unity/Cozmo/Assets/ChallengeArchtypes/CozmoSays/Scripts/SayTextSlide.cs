using UnityEngine;
using DataPersistence;
using Anki.Cozmo.ExternalInterface;

public class SayTextSlide : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.CozmoButton _SayTextButton;

  [SerializeField]
  private UnityEngine.UI.InputField _TextInput;

  [SerializeField]
  private Color _TextFieldInactiveColor;

  [SerializeField]
  private Color _TextFieldActiveColor;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _TotalSparksLabel;

  [SerializeField]
  [Cozmo.ItemId]
  private string _SparkItemId;

  [SerializeField]
  private int _SayCost;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _CostLabel;

  [SerializeField]
  private GameObject _ActiveContentContainer;

  [SerializeField]
  private GameObject _SparkSpinner;

  private int _SparksInInventory;

  private void Awake() {
    _SayTextButton.Initialize(HandleSayTextButton, "say_text_button", "say_text_slide");
    _TextInput.textComponent.color = _TextFieldActiveColor;
    UpdateTotalSparkCount();
    _CostLabel.text = _SayCost.ToString();
    _TextInput.onValueChanged.AddListener(HandleOnTextFieldChange);
    _SayTextButton.Interactable = false;
    _CostLabel.color = _SayTextButton.TextDisabledColor;
  }

  private void OnDestroy() {
    _TextInput.onValueChanged.RemoveListener(HandleOnTextFieldChange);
  }

  public void RegisterInputFocus() {
#if UNITY_IOS
    _TextInput.Select();
    _TextInput.ActivateInputField();
#endif
  }

  private void UpdateTotalSparkCount() {
    _SparksInInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(_SparkItemId);
    _TotalSparksLabel.text = Localization.GetWithArgs(LocalizationKeys.kLabelTotalSparks, new object[] { _SparksInInventory });
  }

  private void HandleOnTextFieldChange(string inputText) {
    if (!string.IsNullOrEmpty(inputText) && _SparksInInventory >= _SayCost) {
      // only enable this button if there's text and we can afford it.
      _SayTextButton.Interactable = true;
      _CostLabel.color = _SayTextButton.TextEnabledColor;
    }
    else {
      _SayTextButton.Interactable = false;
      _CostLabel.color = _SayTextButton.TextDisabledColor;
    }
  }

  private void HandleSayTextButton() {

    _SayTextButton.Interactable = false;
    _TextInput.interactable = false;
    _ActiveContentContainer.SetActive(false);
    _SparkSpinner.SetActive(true);
    _TextInput.textComponent.color = _TextFieldInactiveColor;

    bool hasBadWords = BadWordsFilterManager.Instance.Contains(_TextInput.text);

    if (hasBadWords) {
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CozmoSaysBadWord, (success) => {
        ResetInputStates();
      });
    }
    else {

      DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.RemoveItemAmount(_SparkItemId, _SayCost);
      UpdateTotalSparkCount();

      Anki.Cozmo.AnimationTrigger getInTrigger;
      Anki.Cozmo.AnimationTrigger getOutTrigger;

      SetGetInOutTriggers(_TextInput.text.Length, _TextInput.characterLimit, out getInTrigger, out getOutTrigger);

      RobotActionUnion[] actions = {
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(RobotEngineManager.Instance.CurrentRobot.ID, 1, getInTrigger, true)),
        new RobotActionUnion().Initialize(new SayTextWithIntent().Initialize(
          _TextInput.text,
          Anki.Cozmo.AnimationTrigger.CozmoSaysSpeakLoop,
          Anki.Cozmo.SayTextIntent.Cozmo_Says,
          true)),
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(RobotEngineManager.Instance.CurrentRobot.ID, 1, getOutTrigger, true))
      };

      RobotEngineManager.Instance.CurrentRobot.SendQueueCompoundAction(actions, (success) => {
        ResetInputStates();
      });
    }
  }

  private void SetGetInOutTriggers(int textLength, int maxLength, out Anki.Cozmo.AnimationTrigger inTrigger, out Anki.Cozmo.AnimationTrigger outTrigger) {
    if (textLength > (maxLength / 3) * 2) {
      inTrigger = Anki.Cozmo.AnimationTrigger.CozmoSaysSpeakGetInLong;
      outTrigger = Anki.Cozmo.AnimationTrigger.CozmoSaysSpeakGetOutLong;
    }
    else if (textLength > maxLength / 3) {
      inTrigger = Anki.Cozmo.AnimationTrigger.CozmoSaysSpeakGetInMedium;
      outTrigger = Anki.Cozmo.AnimationTrigger.CozmoSaysSpeakGetOutMedium;
    }
    else {
      inTrigger = Anki.Cozmo.AnimationTrigger.CozmoSaysSpeakGetInShort;
      outTrigger = Anki.Cozmo.AnimationTrigger.CozmoSaysSpeakGetOutShort;
    }
  }

  private void ResetInputStates() {
    _SayTextButton.Interactable = true;
    _TextInput.interactable = true;
    _ActiveContentContainer.SetActive(true);
    _SparkSpinner.SetActive(false);
    _TextInput.textComponent.color = _TextFieldActiveColor;
    // clears text after saying it
    _TextInput.text = "";
#if UNITY_IOS
    RegisterInputFocus();
#endif
  }
}
