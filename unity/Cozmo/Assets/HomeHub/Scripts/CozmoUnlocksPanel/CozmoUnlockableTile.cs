using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using System.Collections;

public class CozmoUnlockableTile : MonoBehaviour {

  public delegate void CozmoUnlockableTileTappedHandler(UnlockableInfo unlockData);
  public event CozmoUnlockableTileTappedHandler OnTapped;

  [SerializeField]
  public GameObject _LockedBackgroundContainer;

  [SerializeField]
  public GameObject _AvailableBackgroundContainer;

  [SerializeField]
  public GameObject _UnlockedBackgroundContainer;

  [SerializeField]
  public Image _UnlockedTintBackground;

  [SerializeField]
  public Image _UnlockedIconSprite;

  [SerializeField]
  public Image _BeginningCircuitySprite;

  [SerializeField]
  public Image _EndCircuitrySprite;

  [SerializeField]
  public Image _ActionIndicator;

  [SerializeField]
  public GameObject _AffordableIndicator;

  [SerializeField]
  public CozmoButton _TileButton;

  private UnlockableInfo _UnlockData;

  public void Initialize(UnlockableInfo unlockableData, CozmoUnlocksPanel.CozmoUnlockState unlockState, string dasViewController,
                        bool isBeginningTile, Sprite beginningSprite, bool isEndTile, Sprite endSprite) {
    _UnlockData = unlockableData;

    gameObject.name = unlockableData.Id.Value.ToString();
    string dasButtonName = gameObject.name + " " + unlockState.ToString();

    _TileButton.Initialize(HandleButtonTapped, dasButtonName, dasViewController);

    _TileButton.Text = Localization.Get(unlockableData.TitleKey);

    _LockedBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Locked);
    _AvailableBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable);
    _UnlockedBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlocked);

    _BeginningCircuitySprite.gameObject.SetActive(isBeginningTile);
    _BeginningCircuitySprite.overrideSprite = beginningSprite;
    _EndCircuitrySprite.gameObject.SetActive(!isEndTile);
    _EndCircuitrySprite.overrideSprite = endSprite;

    _UnlockedIconSprite.sprite = unlockableData.CoreUpgradeIcon;
    _UnlockedTintBackground.color = UIColorPalette.GetUpgradeTintData(unlockableData.CoreUpgradeTintColorName).TintColor;

    _ActionIndicator.gameObject.SetActive(unlockableData.UnlockableType == UnlockableType.Action);

    if (unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable) {
      Cozmo.Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      _AffordableIndicator.gameObject.SetActive(
        playerInventory.CanRemoveItemAmount(unlockableData.UpgradeCostItemId, unlockableData.UpgradeCostAmountNeeded));
    }
    else {
      _AffordableIndicator.gameObject.SetActive(false);
    }
  }

  private void HandleButtonTapped() {
    if (OnTapped != null) {
      OnTapped(_UnlockData);
    }
  }
}
