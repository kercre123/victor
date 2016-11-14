using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;
using Cozmo.UI;
using Cozmo.HomeHub;

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

  private List<CozmoUnlockableTile> _UnlockedTiles;
  private List<CozmoUnlockableTile> _UnlockableTiles;
  private List<CozmoUnlockableTile> _LockedTiles;

  [SerializeField]
  private RectTransform _UnlocksContainer;

  [SerializeField]
  private CozmoUnlockableTile _UnlocksTilePrefab;

  [SerializeField]
  private CoreUpgradeDetailsDialog _CoreUpgradeDetailsViewPrefab;
  private BaseView _CoreUpgradeDetailsViewInstance;

  [SerializeField]
  private UnlockAnimation _CoreUpgradeAnimatedIconPrefab;
  private UnlockAnimation _CoreupgradeAnimatedIconInstance;

  void Start() {
    _UnlockedTiles = new List<CozmoUnlockableTile>();
    _UnlockableTiles = new List<CozmoUnlockableTile>();
    _LockedTiles = new List<CozmoUnlockableTile>();

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
    }
  }

  void OnDestroy() {
    if (_CoreUpgradeDetailsViewInstance != null) {
      _CoreUpgradeDetailsViewInstance.CloseViewImmediately();
    }

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
    }
    _LockedTiles.Clear();

    for (int i = 0; i < _UnlocksContainer.transform.childCount; ++i) {
      Destroy(_UnlocksContainer.transform.GetChild(i).gameObject);
    }
  }

  private void HandleTappedUnlocked(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped Unlocked: " + unlockInfo.Id);

    if (_CoreUpgradeDetailsViewInstance == null && !HomeHub.Instance.HomeViewInstance.HomeViewCurrentlyOccupied) {
      CoreUpgradeDetailsDialog detailView = UIManager.OpenView<CoreUpgradeDetailsDialog>(_CoreUpgradeDetailsViewPrefab);
      detailView.Initialize(unlockInfo, CozmoUnlockState.Unlocked, null);
      _CoreUpgradeDetailsViewInstance = detailView;
    }
  }

  private void HandleTappedUnlockable(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
    if (_CoreUpgradeDetailsViewInstance == null && !HomeHub.Instance.HomeViewInstance.HomeViewCurrentlyOccupied) {
      CoreUpgradeDetailsDialog detailView = UIManager.OpenView<CoreUpgradeDetailsDialog>(_CoreUpgradeDetailsViewPrefab);
      detailView.Initialize(unlockInfo, CozmoUnlockState.Unlockable, HandleUnlockableUpgradeUnlocked);
      _CoreUpgradeDetailsViewInstance = detailView;
    }
  }


  private void HandleTappedLocked(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
    if (_CoreUpgradeDetailsViewInstance == null && !HomeHub.Instance.HomeViewInstance.HomeViewCurrentlyOccupied) {
      CoreUpgradeDetailsDialog detailView = UIManager.OpenView<CoreUpgradeDetailsDialog>(_CoreUpgradeDetailsViewPrefab);
      detailView.Initialize(unlockInfo, CozmoUnlockState.Locked, HandleUnlockableUpgradeUnlocked);
      _CoreUpgradeDetailsViewInstance = detailView;
    }
  }

  private void HandleTappedUnavailable(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped unavailable: " + unlockInfo.Id);
    if (_CoreUpgradeDetailsViewInstance == null && !HomeHub.Instance.HomeViewInstance.HomeViewCurrentlyOccupied) {

      UnlockableInfo preReqInfo = null;
      for (int i = 0; i < unlockInfo.Prerequisites.Length; i++) {
        // if available but we haven't unlocked it yet, then it is the upgrade that is blocking us
        if (!UnlockablesManager.Instance.IsUnlocked(unlockInfo.Prerequisites[i].Value)) {
          preReqInfo = UnlockablesManager.Instance.GetUnlockableInfo(unlockInfo.Prerequisites[i].Value);
        }
      }
      // Create alert view with Icon
      AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab_ComingSoon, overrideCloseOnTouchOutside: true);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonClose, null);
      alertView.TitleLocKey = unlockInfo.TitleKey;
      alertView.SetIcon(unlockInfo.CoreUpgradeIcon);

      if (unlockInfo.NeverAvailable) {
        alertView.TitleLocKey = LocalizationKeys.kUnlockableComingSoonTitle;
        alertView.DescriptionLocKey = unlockInfo.DescriptionKey;
      }
      else if (preReqInfo == null) {
        alertView.DescriptionLocKey = LocalizationKeys.kUnlockableUnavailableDescription;
        alertView.SetMessageArgs(new object[] { Localization.Get(unlockInfo.TitleKey) });
      }
      else {

        string preReqTypeKey = "unlockable.Unlock";

        switch (preReqInfo.UnlockableType) {
        case UnlockableType.Action:
          preReqTypeKey = LocalizationKeys.kUnlockableUpgrade;
          break;
        case UnlockableType.Game:
          preReqTypeKey = LocalizationKeys.kUnlockableApp;
          break;
        default:
          preReqTypeKey = LocalizationKeys.kUnlockableUnlock;
          break;
        }

        alertView.DescriptionLocKey = LocalizationKeys.kUnlockablePreReqNeededDescription;
        alertView.SetMessageArgs(new object[] { Localization.Get(preReqInfo.TitleKey), Localization.Get(preReqTypeKey) });
      }

      _CoreUpgradeDetailsViewInstance = alertView;
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
