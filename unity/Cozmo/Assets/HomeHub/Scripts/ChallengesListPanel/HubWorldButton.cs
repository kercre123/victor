using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using DataPersistence;

namespace Cozmo.HubWorld {
  public class HubWorldButton : MonoBehaviour {
    public delegate void ButtonClickedHandler(string challengeClickedId, Transform buttonTransform);

    public event ButtonClickedHandler OnButtonClicked;

    [SerializeField]
    private Cozmo.UI.CozmoButtonLegacy _ButtonScript;

    [SerializeField]
    private Cozmo.UI.IconProxy _IconProxy;

    [SerializeField]
    private Transform _IconContainer;

    [SerializeField]
    private Anki.UI.AnkiTextLegacy _ChallengeTitle;

    private string _ChallengeId;

    [SerializeField]
    private GameObject _AvailableContainer;
    [SerializeField]
    private GameObject _UnavailableContainer;

    [SerializeField]
    private GameObject _AffordableIndicator;

    [SerializeField]
    private GameObject _NewUnlockIndicator;

    [SerializeField]
    private GameObject _ComingSoonContainer;

    private ChallengeData _ChallengeData;
    private bool _IsUnlocked = true;
    private bool _IsAvailable = true;

    public virtual void Initialize(ChallengeData challengeData, string dasParentViewName, bool isEnd = false, bool isNew = false, bool isUnlocked = true, bool isAvailable = true) {
      _NewUnlockIndicator.SetActive(isNew);
      _AffordableIndicator.SetActive(false);
      if (challengeData != null) {
        _ChallengeData = challengeData;
        _ChallengeId = challengeData.ChallengeID;
        _IconProxy.SetIcon(challengeData.ChallengeIcon);
        _ChallengeTitle.text = Localization.Get(challengeData.ChallengeTitleLocKey);
        _IsUnlocked = isUnlocked;
        _IsAvailable = isAvailable;

        CheckAffordabilityAndSetIndicator();

        Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.ItemAdded += HandleItemValueChanged;
        playerInventory.ItemRemoved += HandleItemValueChanged;
        playerInventory.ItemCountSet += HandleItemValueChanged;

        UnlockableInfo unlockabeInfo = UnlockablesManager.Instance.GetUnlockableInfo(_ChallengeData.UnlockId.Value);
        _ComingSoonContainer.SetActive(unlockabeInfo.NeverAvailable);
      }

      _ButtonScript.Initialize(HandleButtonClicked, string.Format("see_{0}_details_button", _ChallengeId), dasParentViewName);
      _ButtonScript.onPress.AddListener(HandlePointerDown);
      _ButtonScript.onRelease.AddListener(HandlePointerUp);
      _AvailableContainer.SetActive(isAvailable);
      _UnavailableContainer.SetActive(!isAvailable);
    }

    private void OnDestroy() {
      if (DataPersistenceManager.Instance != null) {
        Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.ItemAdded -= HandleItemValueChanged;
        playerInventory.ItemRemoved -= HandleItemValueChanged;
        playerInventory.ItemCountSet -= HandleItemValueChanged;
      }
    }

    private void HandleItemValueChanged(string itemId, int delta, int newCount) {
      CheckAffordabilityAndSetIndicator();
    }

    private void CheckAffordabilityAndSetIndicator() {
      if (!_IsUnlocked) {
        if (_IsAvailable) {
          UnlockableInfo uInfo = UnlockablesManager.Instance.GetUnlockableInfo(_ChallengeData.UnlockId.Value);
          Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
          bool canAfford = playerInventory.CanRemoveItemAmount(uInfo.UpgradeCostItemId, uInfo.UpgradeCostAmountNeeded);
          _AffordableIndicator.SetActive(canAfford);
        }
        _IconProxy.SetAlpha(0.4f);
      }

    }

    private void HandlePointerDown() {
      _AvailableContainer.transform.localScale = new Vector3(0.9f, 0.9f, 1.0f);
      _UnavailableContainer.transform.localScale = new Vector3(0.9f, 0.9f, 1.0f);
    }

    private void HandlePointerUp() {
      _AvailableContainer.transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
      _UnavailableContainer.transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
    }

    private void HandleButtonClicked() {
      RaiseButtonClicked(_ChallengeId);
    }

    private void RaiseButtonClicked(string challenge) {
      if (OnButtonClicked != null) {
        OnButtonClicked(challenge, this.transform);
      }
    }

  }
}