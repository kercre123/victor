using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DataPersistence;
using Cozmo.UI;

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

  void Start() {
    _UnlockedTiles = new List<CozmoUnlockableTile>();
    _UnlockableTiles = new List<CozmoUnlockableTile>();
    _LockedTiles = new List<CozmoUnlockableTile>();
    LoadTiles();
    RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleRequestSetUnlockResult);

    // Show onboarding for this section if it's the first time here.
    if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Upgrades)) {
      // We've gotten into a state that there is no escape. Due to an old robot -> new app setting.
      // Just skip onboarding, they've figured it out on a previous phone...
      bool isOldRobot = UnlockablesManager.Instance.IsUnlocked(Anki.Cozmo.UnlockId.StackTwoCubes);
      if (!isOldRobot) {
        if (_UnlockableTiles.Count > 0) {
          OnboardingManager.Instance.SetOutlineRegion(_UnlockableTiles[0].transform);
        }
        OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.Upgrades);
      }
      else {
        OnboardingManager.Instance.CompletePhase(OnboardingManager.OnboardingPhases.Upgrades);
      }

    }
  }

  void OnDestroy() {
    if (_CoreUpgradeDetailsViewInstance != null) {
      _CoreUpgradeDetailsViewInstance.CloseView();
    }

    RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.RequestSetUnlockResult>(HandleRequestSetUnlockResult);
  }

  public void LoadTiles() {

    ClearTiles();

    string viewControllerName = "home_hub_cozmo_unlock_panel";
    int numTilesMade = 0;
    List<UnlockableInfo> unlockedUnlockData = UnlockablesManager.Instance.GetUnlocked();
    List<UnlockableInfo> unlockableUnlockData = UnlockablesManager.Instance.GetAvailableAndLocked();
    List<UnlockableInfo> lockedUnlockData = UnlockablesManager.Instance.GetUnavailable();
    List<UnlockableInfo> comingSoonUnlockData = UnlockablesManager.Instance.GetNeverAvailable();

    // Sort within themselves on "SortOrder" since locked doesn't show anything, no need to sort.
    unlockedUnlockData.Sort();
    unlockableUnlockData.Sort();
    lockedUnlockData.Sort();

    GameObject tileInstance;
    CozmoUnlockableTile unlockableTile;
    for (int i = 0; i < unlockedUnlockData.Count; ++i) {
      if (unlockedUnlockData[i].UnlockableType == UnlockableType.Action) {
        tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
        unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
        unlockableTile.Initialize(unlockedUnlockData[i], CozmoUnlockState.Unlocked, viewControllerName);
        unlockableTile.OnTapped += HandleTappedUnlocked;
        _UnlockedTiles.Add(unlockableTile);
        numTilesMade++;
      }
    }

    for (int i = 0; i < unlockableUnlockData.Count; ++i) {
      if (unlockableUnlockData[i].UnlockableType == UnlockableType.Action) {
        tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
        unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
        unlockableTile.Initialize(unlockableUnlockData[i], CozmoUnlockState.Unlockable, viewControllerName);
        unlockableTile.OnTapped += HandleTappedUnlockable;
        _UnlockableTiles.Add(unlockableTile);
        numTilesMade++;
      }
    }

    for (int i = 0; i < lockedUnlockData.Count; ++i) {
      if (lockedUnlockData[i].UnlockableType == UnlockableType.Action) {
        tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
        unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
        unlockableTile.Initialize(lockedUnlockData[i], CozmoUnlockState.Locked, viewControllerName);
        unlockableTile.OnTapped += HandleTappedUnavailable;
        _LockedTiles.Add(unlockableTile);
        numTilesMade++;
      }
    }

    for (int i = 0; i < comingSoonUnlockData.Count; ++i) {
      if (comingSoonUnlockData[i].UnlockableType == UnlockableType.Action) {
        tileInstance = UIManager.CreateUIElement(_UnlocksTilePrefab, _UnlocksContainer);
        unlockableTile = tileInstance.GetComponent<CozmoUnlockableTile>();
        unlockableTile.Initialize(comingSoonUnlockData[i], CozmoUnlockState.NeverAvailable, viewControllerName);
        unlockableTile.OnTapped += HandleTappedUnavailable;
        _LockedTiles.Add(unlockableTile);
        numTilesMade++;
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
    if (_CoreUpgradeDetailsViewInstance == null) {
      CoreUpgradeDetailsDialog detailView = UIManager.OpenView<CoreUpgradeDetailsDialog>(_CoreUpgradeDetailsViewPrefab);
      detailView.Initialize(unlockInfo, CozmoUnlockState.Unlocked, null);
      _CoreUpgradeDetailsViewInstance = detailView;
    }
  }

  private void HandleTappedUnlockable(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped available: " + unlockInfo.Id);
    if (_CoreUpgradeDetailsViewInstance == null) {
      CoreUpgradeDetailsDialog detailView = UIManager.OpenView<CoreUpgradeDetailsDialog>(_CoreUpgradeDetailsViewPrefab);
      detailView.Initialize(unlockInfo, CozmoUnlockState.Unlockable, HandleUnlockableUpgradeUnlocked);
      _CoreUpgradeDetailsViewInstance = detailView;
    }
  }

  private void HandleTappedUnavailable(UnlockableInfo unlockInfo) {
    DAS.Debug(this, "Tapped unavailable: " + unlockInfo.Id);
    if (_CoreUpgradeDetailsViewInstance == null) {

      UnlockableInfo preReqInfo = null;
      for (int i = 0; i < unlockInfo.Prerequisites.Length; i++) {
        // if available but we haven't unlocked it yet, then it is the upgrade that is blocking us
        if (UnlockablesManager.Instance.IsUnlockableAvailable(unlockInfo.Prerequisites[i].Value) && !UnlockablesManager.Instance.IsUnlocked(unlockInfo.Prerequisites[i].Value)) {
          preReqInfo = UnlockablesManager.Instance.GetUnlockableInfo(unlockInfo.Prerequisites[i].Value);
        }
      }
      // Create alert view with Icon
      AlertView alertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab, overrideCloseOnTouchOutside: true);
      alertView.SetPrimaryButton(LocalizationKeys.kButtonClose, null);
      alertView.TitleLocKey = unlockInfo.TitleKey;
      alertView.DescriptionLocKey = LocalizationKeys.kUnlockableUnavailableDescription;
      alertView.SetMessageArgs(new object[] { Localization.Get(unlockInfo.TitleKey) });


      if (unlockInfo.NeverAvailable) {
        alertView.DescriptionLocKey = LocalizationKeys.kUnlockableComingSoonDescription;
      }
      else if (preReqInfo == null) {
        alertView.DescriptionLocKey = LocalizationKeys.kUnlockableUnavailableDescription;
        alertView.SetMessageArgs(new object[] { Localization.Get(unlockInfo.TitleKey) });
      }
      else {
        alertView.DescriptionLocKey = LocalizationKeys.kUnlockablePreReqNeededDescription;
        alertView.SetMessageArgs(new object[] { Localization.Get(preReqInfo.TitleKey) });
      }

      _CoreUpgradeDetailsViewInstance = alertView;
    }

  }

  private void HandleUnlockableUpgradeUnlocked(UnlockableInfo unlockInfo) {
    UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);
  }

  private void HandleRequestSetUnlockResult(object message) {
    LoadTiles();
  }

}
