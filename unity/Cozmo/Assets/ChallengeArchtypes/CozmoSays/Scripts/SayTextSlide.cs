using UnityEngine;
using DataPersistence;

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
    DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.RemoveItemAmount(_SparkItemId, _SayCost);
    UpdateTotalSparkCount();

    RobotEngineManager.Instance.CurrentRobot.SayTextWithEvent(_TextInput.text, Anki.Cozmo.AnimationTrigger.MeetCozmoReEnrollmentSayName, callback: (success) => {

      _SayTextButton.Interactable = true;
      _TextInput.interactable = true;
      _ActiveContentContainer.SetActive(true);
      _SparkSpinner.SetActive(false);
      _TextInput.textComponent.color = _TextFieldActiveColor;
      // clears text after saying it
      _TextInput.text = "";

      RegisterInputFocus();
    });

  }
}
