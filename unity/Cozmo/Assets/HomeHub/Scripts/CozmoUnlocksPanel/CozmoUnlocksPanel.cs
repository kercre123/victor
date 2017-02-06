using UnityEngine;
using System.Collections.Generic;
using Cozmo.UI;

namespace Cozmo.Upgrades {
  public class CozmoUnlocksPanel : MonoBehaviour {

    public enum CozmoUnlockState {
      Unlocked,
      Unlockable,
      Locked,
      NeverAvailable
    }

    public enum CozmoUnlockPosition {
      Beginning,
      Middle,
      End
    }

    private List<CozmoUnlockableTile> _UnlockedTiles = new List<CozmoUnlockableTile>();
    private List<CozmoUnlockableTile> _UnlockableTiles = new List<CozmoUnlockableTile>();
    private List<CozmoUnlockableTile> _LockedTiles = new List<CozmoUnlockableTile>();

    [SerializeField]
    private RectTransform _UnlocksContainer;

    [SerializeField]
    private CozmoUnlockableTile _UnlocksTilePrefab;

    [SerializeField]
    private CoreUpgradeDetailsModal _CoreUpgradeDetailsModalPrefab;
    private ModalPriorityData _CoreUpgradeDetailsModalPriorityData = new ModalPriorityData(ModalPriorityLayer.VeryLow, 1,
                                                                                           LowPriorityModalAction.CancelSelf,
                                                                                           HighPriorityModalAction.Stack);
    private BaseModal _CoreUpgradeDetailsModalInstance;

    [SerializeField]
    private UnlockAnimation _CoreUpgradeAnimatedIconPrefab;
    private UnlockAnimation _CoreupgradeAnimatedIconInstance;

    [SerializeField]
    private AlertModal _ComingSoonAlertPrefab;

    void Start() {
      // Did they already complete this section of onboarding on a different robot?
      // Needs to be evaluated before we load tiles since thier interactable state depends on this
      bool isOldRobot = UnlockablesManager.Instance.IsUnlocked(Anki.Cozmo.UnlockId.StackTwoCubes);
      if (isOldRobot && OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Upgrades)) {
        OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.Upgrades);
      }

      LoadTiles();
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleRequestSetUnlockResult);

      // Now that the tiles have loaded highlight if it's a new robot and kick off.
      if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Upgrades)) {
        if (_UnlockableTiles.Count > 0) {
          OnboardingManager.Instance.SetOutlineRegion(_UnlockableTiles[0].transform);
        }
        OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.Upgrades);
        OnboardingManager.Instance.OnOnboardingPhaseCompleted += HandleOnboardingCompleted;
      }
    }

    void OnDestroy() {
      if (_CoreUpgradeDetailsModalInstance != null) {
        _CoreUpgradeDetailsModalInstance.CloseDialogImmediately();
      }

      ClearTiles();

      OnboardingManager.Instance.OnOnboardingPhaseCompleted -= HandleOnboardingCompleted;
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleRequestSetUnlockResult);
    }

    private void LoadTiles() {

      ClearTiles();

      string viewControllerName = "home_hub_cozmo_unlock_panel";
      int numTilesMade = 0;


      List<UnlockableInfo> allUnlockData = UnlockablesManager.Instance.GetUnlockablesByType(UnlockableType.Action);
      // Sort within themselves on "SortOrder" since locked doesn't show anything, no need to sort.
      allUnlockData.Sort();


      GameObject tileInstance;
      CozmoUnlockableTile unlockableTile;
      // Don't let people tap around if they have the one to unlock and haven't seen onboarding yet.
      bool isUnlockedDisabled = OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Upgrades) &&
                                UnlockablesManager.Instance.GetAvailableAndLocked().Count > 0;
      for (int i = 0; i < allUnlockData.Count; ++i) {
        tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
        unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
        CozmoUnlockState unlockState = CozmoUnlockState.Locked;
        if (allUnlockData[i].NeverAvailable) {
          unlockState = CozmoUnlockState.NeverAvailable;
          unlockableTile.OnTapped += HandleTappedUnavailable;
        }
        else if (UnlockablesManager.Instance.IsUnlocked(allUnlockData[i].Id.Value)) {
          unlockState = CozmoUnlockState.Unlocked;
          _UnlockedTiles.Add(unlockableTile);
          unlockableTile.OnTapped += HandleTappedUnlocked;
        }
        else if (UnlockablesManager.Instance.IsUnlockableAvailable(allUnlockData[i].Id.Value)) {
          unlockState = CozmoUnlockState.Unlockable;
          _UnlockableTiles.Add(unlockableTile);
          unlockableTile.OnTapped += HandleTappedUnlockable;
        }
        else {
          unlockState = CozmoUnlockState.Locked;
          _LockedTiles.Add(unlockableTile);
          unlockableTile.OnTapped += HandleTappedLocked;
        }
        unlockableTile.Initialize(allUnlockData[i], unlockState, viewControllerName);

        numTilesMade++;
        // don't click on anything else during onboarding except upgrade.
        if (isUnlockedDisabled && unlockState != CozmoUnlockState.Unlockable) {
          unlockableTile._TileButton.Interactable = false;
        }
      }

    }

    private void ClearTiles() {
      for (int i = 0; i < _UnlockedTiles.Count; ++i) {
        _UnlockedTiles[i].OnTapped -= HandleTappedUnlocked;
      }
      _UnlockedTiles.Clear();

      for (int i = 0; i < _UnlockableTiles.Count; ++i) {
        _UnlockableTiles[i].OnTapped -= HandleTappedUnlockable;
      }
      _UnlockableTiles.Clear();

      for (int i = 0; i < _LockedTiles.Count; ++i) {
        _LockedTiles[i].OnTapped -= HandleTappedUnavailable;
        _LockedTiles[i].OnTapped -= HandleTappedLocked;
      }
      _LockedTiles.Clear();

      for (int i = 0; i < _UnlocksContainer.transform.childCount; ++i) {
        Destroy(_UnlocksContainer.transform.GetChild(i).gameObject);
      }
    }

    private void HandleTappedUnlocked(UnlockableInfo unlockInfo) {
      DAS.Debug(this, "Tapped Unlocked: " + unlockInfo.Id);
      System.Action<BaseModal> detailsModalCreatedCallback = (detailModal) => {
        ((CoreUpgradeDetailsModal)detailModal).Initialize(unlockInfo, CozmoUnlockState.Unlocked, null);
        _CoreUpgradeDetailsModalInstance = detailModal;
      };
      UIManager.OpenModal(_CoreUpgradeDetailsModalPrefab, _CoreUpgradeDetailsModalPriorityData, detailsModalCreatedCallback);
    }

    private void HandleTappedUnlockable(UnlockableInfo unlockInfo) {
      DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
      System.Action<BaseModal> detailsModalCreatedCallback = (detailModal) => {
        ((CoreUpgradeDetailsModal)detailModal).Initialize(unlockInfo, CozmoUnlockState.Unlockable, HandleUnlockableUpgradeUnlocked);
        _CoreUpgradeDetailsModalInstance = detailModal;
      };
      UIManager.OpenModal(_CoreUpgradeDetailsModalPrefab, _CoreUpgradeDetailsModalPriorityData, detailsModalCreatedCallback);

      _LockedTiles.Clear();
      if (_UnlocksContainer != null) {
        for (int i = 0; i < _UnlocksContainer.transform.childCount; ++i) {
          Destroy(_UnlocksContainer.transform.GetChild(i).gameObject);
        }
      }
    }

    private void HandleTappedLocked(UnlockableInfo unlockInfo) {
      DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
      System.Action<BaseModal> detailsModalCreatedCallback = (detailModal) => {
        ((CoreUpgradeDetailsModal)detailModal).Initialize(unlockInfo, CozmoUnlockState.Locked, HandleUnlockableUpgradeUnlocked);
        _CoreUpgradeDetailsModalInstance = detailModal;
      };
      UIManager.OpenModal(_CoreUpgradeDetailsModalPrefab, _CoreUpgradeDetailsModalPriorityData, detailsModalCreatedCallback);
    }

    private void HandleTappedUnavailable(UnlockableInfo unlockInfo) {
      DAS.Debug(this, "Tapped unavailable: " + unlockInfo.Id);
      // Open "Coming Soon" alert view
      var closeAlertButtonData = new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose);
      var comingSoonAlertData = new AlertModalData("coming_soon_alert", LocalizationKeys.kUnlockableComingSoonTitle,
                                                              unlockInfo.DescriptionKey, closeAlertButtonData, icon: unlockInfo.CoreUpgradeIcon);

      System.Action<BaseModal> comingSoonAlertCreatedCallback = (alertModal) => {
        ((AlertModal)alertModal).InitializeAlertData(comingSoonAlertData);
        _CoreUpgradeDetailsModalInstance = alertModal;
      };

      UIManager.OpenModal(_ComingSoonAlertPrefab, _CoreUpgradeDetailsModalPriorityData, comingSoonAlertCreatedCallback,
                          overrideCloseOnTouchOutside: true);
    }

    private void HandleOnboardingCompleted(OnboardingManager.OnboardingPhases phase) {
      // should be after Spark's "okay"
      for (int i = 0; i < _UnlockedTiles.Count; ++i) {
        _UnlockedTiles[i]._TileButton.Interactable = true;
      }
      for (int i = 0; i < _UnlockableTiles.Count; ++i) {
        _UnlockableTiles[i]._TileButton.Interactable = true;
      }
      for (int i = 0; i < _LockedTiles.Count; ++i) {
        _LockedTiles[i]._TileButton.Interactable = true;
      }
    }

    private void HandleUnlockableUpgradeUnlocked(UnlockableInfo unlockInfo) {
      UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);
    }

    private void HandleRequestSetUnlockResult(Anki.Cozmo.ExternalInterface.RequestSetUnlockResult message) {
      LoadTiles();
      AnimateNewlyUnlocked(message.unlockID);
    }

    private void AnimateNewlyUnlocked(Anki.Cozmo.UnlockId newlyUnlockedId) {
      // look for the newly unlocked in the unlocks panel list
      CozmoUnlockableTile unlockableTile = null;
      for (int i = 0; i < _UnlockedTiles.Count; ++i) {
        if (_UnlockedTiles[i].GetUnlockData().Id.Value == newlyUnlockedId) {
          unlockableTile = _UnlockedTiles[i];
        }
      }

      if (unlockableTile != null) {
        _CoreupgradeAnimatedIconInstance = GameObject.Instantiate(_CoreUpgradeAnimatedIconPrefab.gameObject).GetComponent<UnlockAnimation>();
        _CoreupgradeAnimatedIconInstance.Initialize(unlockableTile._UnlockedIconSprite.sprite, unlockableTile.transform);
        // making this a child of CozmoUnlocksPanel so it gets cleaned up if CozmoUnlocksPanel is destroyed
        _CoreupgradeAnimatedIconInstance.transform.SetParent(transform, false);
      }
    }

  }
}