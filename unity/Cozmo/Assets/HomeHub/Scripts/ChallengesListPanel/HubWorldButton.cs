using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using DataPersistence;

namespace Cozmo.HubWorld {
  public class HubWorldButton : MonoBehaviour {
    public delegate void ButtonClickedHandler(string challengeClickedId, Transform buttonTransform);

    public event ButtonClickedHandler OnButtonClicked;

    [SerializeField]
    private Cozmo.UI.CozmoButton _ButtonScript;

    [SerializeField]
    private Cozmo.UI.IconProxy _IconProxy;

    [SerializeField]
    private Transform _IconContainer;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _ChallengeTitle;

    [SerializeField]
    private GameObject _LockedBadgeContainer;

    private string _ChallengeId;

    [SerializeField]
    private GameObject _AvailableContainer;
    [SerializeField]
    private GameObject _UnavailableContainer;

    [SerializeField]
    private GameObject _AffordableIndicator;

    [SerializeField]
    private GameObject _NewUnlockIndicator;

    public virtual void Initialize(ChallengeData challengeData, string dasParentViewName, bool isEnd = false, bool isNew = false, bool isUnlocked = true, bool isAvailable = true) {
      _NewUnlockIndicator.SetActive(isNew);
      _LockedBadgeContainer.SetActive(!isUnlocked);
      _AffordableIndicator.SetActive(false);
      if (challengeData != null) {
        _ChallengeId = challengeData.ChallengeID;
        _IconProxy.SetIcon(challengeData.ChallengeIcon);
        _ChallengeTitle.text = Localization.Get(challengeData.ChallengeTitleLocKey);
        if (!isUnlocked) {
          if (isAvailable) {
            UnlockableInfo uInfo = UnlockablesManager.Instance.GetUnlockableInfo(challengeData.UnlockId.Value);
            Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
            bool canAfford = playerInventory.CanRemoveItemAmount(uInfo.UpgradeCostItemId, uInfo.UpgradeCostAmountNeeded);
            _AffordableIndicator.SetActive(canAfford);
            _LockedBadgeContainer.SetActive(!canAfford);

          }
          _IconProxy.SetAlpha(0.4f);
        }
      }

      _ButtonScript.Initialize(HandleButtonClicked, string.Format("see_{0}_details_button", _ChallengeId), dasParentViewName);
      _ButtonScript.onPress.AddListener(HandlePointerDown);
      _ButtonScript.onRelease.AddListener(HandlePointerUp);
      _AvailableContainer.SetActive(isAvailable);
      _UnavailableContainer.SetActive(!isAvailable);
    }

    private void HandlePointerDown() {
      _IconContainer.localScale = new Vector3(0.9f, 0.9f, 1.0f);
    }

    private void HandlePointerUp() {
      _IconContainer.localScale = new Vector3(1.0f, 1.0f, 1.0f);
    }

    private void Update() {
      OnUpdate();
    }

    protected virtual void OnUpdate() {
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