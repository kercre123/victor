using UnityEngine;
using DataPersistence;
using Anki.Cozmo.ExternalInterface;
using UnityEngine.EventSystems;

public class SayTextSlide : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.CozmoButtonLegacy _SayTextButton;

  [SerializeField]
  private UnityEngine.UI.InputField _TextInput;

  [SerializeField]
  private Color _TextFieldInactiveColor;

  [SerializeField]
  private Color _TextFieldActiveColor;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _TotalSparksLabel;

  [SerializeField]
  [Cozmo.ItemId]
  private string _SparkItemId;

  [SerializeField]
  private int _SayCost;

  [SerializeField]
  private Anki.UI.AnkiTextLegacy _CostLabel;

  [SerializeField]
  private GameObject _ActiveContentContainer;

  [SerializeField]
  private GameObject _SparkSpinner;

  private int _SparksInInventory;

  private CozmoSays.CozmoSaysGame _CozmoSaysGame;

  private bool _PlayingSayAnimation = false;
  private bool _TextFieldEmpty = true;
  private bool _NotEnoughSparks = false;
  private bool _PlayingReactionaryBehavior = false;

  private bool _PendingClearField = false;

  public void Initialize(CozmoSays.CozmoSaysGame cozmoSaysGame) {
    _CozmoSaysGame = cozmoSaysGame;
  }

  private void Awake() {
    _SayTextButton.Initialize(HandleSayTextButton, "say_text_button", "say_text_slide");
    _TextInput.textComponent.color = _TextFieldActiveColor;
    UpdateTotalSparkCount();
    _CostLabel.text = _SayCost.ToString();
    _TextInput.onValueChanged.AddListener(HandleOnTextFieldChange);
    _TextInput.onValidateInput += HandleInputValidation;
    _TextInput.GetComponent<InputFieldEventListeners>().onSelect += OnSelect;

    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.ReactionTriggerTransition>(HandleRobotReactionaryBehavior);
    SetButtonInteractivity();
  }

  private void OnDestroy() {
    _TextInput.onValueChanged.RemoveListener(HandleOnTextFieldChange);
    _TextInput.onValidateInput -= HandleInputValidation;
    _TextInput.GetComponent<InputFieldEventListeners>().onSelect -= OnSelect;

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.ReactionTriggerTransition>(HandleRobotReactionaryBehavior);
  }

  private void OnSelect(BaseEventData data) {
    if (_PendingClearField) {
      _TextInput.text = "";
      _PendingClearField = false;
    }
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
    _NotEnoughSparks = _SparksInInventory < _SayCost;
  }

  private char HandleInputValidation(string input, int charIndex, char addedChar) {
    if (char.IsLetter(addedChar) || char.IsWhiteSpace(addedChar) || IsPunctuation(addedChar)) {
      return addedChar;
    }
    return '\0';
  }

  // less forgiving than char.IsPunctuation();
  private bool IsPunctuation(char c) {
    return c == '.' || c == ';' || c == '\'' || c == ',' || c == '?' || c == '!' || c == ':';
  }

  private void HandleOnTextFieldChange(string inputText) {
    _TextFieldEmpty = string.IsNullOrEmpty(inputText);
    SetButtonInteractivity();
  }

  private void SetSayTextReactionaryBehaviors(bool enable) {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("say_text_slide", Anki.Cozmo.ReactionTrigger.UnexpectedMovement, enable);
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("say_text_slide", Anki.Cozmo.ReactionTrigger.RobotPickedUp, enable);
      RobotEngineManager.Instance.CurrentRobot.RequestEnableReactionTrigger("say_text_slide", Anki.Cozmo.ReactionTrigger.CliffDetected, enable);
    }
  }

  private void HandleSayTextButton() {

    _PlayingSayAnimation = true;
    _TextInput.interactable = false;
    _ActiveContentContainer.SetActive(false);
    _SparkSpinner.SetActive(true);
    _TextInput.textComponent.color = _TextFieldInactiveColor;
    _PendingClearField = true;

    SetSayTextReactionaryBehaviors(false);
    SetButtonInteractivity();

    bool hasBadWords = BadWordsFilterManager.Instance.Contains(_TextInput.text);

    if (hasBadWords) {
      RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CozmoSaysBadWord, (success) => {
        ResetInputStates();
        SetSayTextReactionaryBehaviors(true);
      });
    }
    else {

      GameEventManager.Instance.FireGameEvent(Anki.Cozmo.GameEvent.OnCozmoSayText);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Cozmo_Says_Speaking);

      Anki.Cozmo.AnimationTrigger getInTrigger;
      Anki.Cozmo.AnimationTrigger getOutTrigger;

      SetGetInOutTriggers(_TextInput.text.Length, _TextInput.characterLimit, out getInTrigger, out getOutTrigger);

      RobotActionUnion[] actions = {
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(RobotEngineManager.Instance.CurrentRobot.ID, 1, getInTrigger, true, false, false, false)),
        new RobotActionUnion().Initialize(new SayTextWithIntent().Initialize(
          _TextInput.text,
          Anki.Cozmo.AnimationTrigger.CozmoSaysSpeakLoop,
          Anki.Cozmo.SayTextIntent.Cozmo_Says,
          true)),
        new RobotActionUnion().Initialize(new PlayAnimationTrigger().Initialize(RobotEngineManager.Instance.CurrentRobot.ID, 1, getOutTrigger, true, false, false, false))
      };

      RobotEngineManager.Instance.CurrentRobot.SendQueueCompoundAction(actions, (success) => {
        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(_CozmoSaysGame.GetDefaultMusicState());
        if (success) {
          DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.RemoveItemAmount(_SparkItemId, _SayCost);
          UpdateTotalSparkCount();
          SetSayTextReactionaryBehaviors(true);
        }
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
    _PlayingSayAnimation = false;
    _TextInput.interactable = true;
    _ActiveContentContainer.SetActive(true);
    _SparkSpinner.SetActive(false);
    _TextInput.textComponent.color = _TextFieldActiveColor;

    SetButtonInteractivity();
  }

  private void SetButtonInteractivity() {
    if (_TextFieldEmpty || _NotEnoughSparks || _PlayingSayAnimation || _PlayingReactionaryBehavior) {
      _CostLabel.color = _SayTextButton.TextDisabledColor;
      _SayTextButton.Interactable = false;
    }
    else {
      _CostLabel.color = _SayTextButton.TextEnabledColor;
      _SayTextButton.Interactable = true;
    }
  }

  private void HandleRobotReactionaryBehavior(Anki.Cozmo.ExternalInterface.ReactionTriggerTransition message) {
    _PlayingReactionaryBehavior = message.newTrigger != Anki.Cozmo.ReactionTrigger.NoneTrigger;
    SetButtonInteractivity();
  }
}
