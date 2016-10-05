using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using System.Collections;

public class CozmoUnlockableTile : MonoBehaviour {
  private const float kLockedAlpha = 0.5f;

  public delegate void CozmoUnlockableTileTappedHandler(UnlockableInfo unlockData);
  public event CozmoUnlockableTileTappedHandler OnTapped;

  [SerializeField]
  public GameObject _LockedBackgroundContainer;

  [SerializeField]
  public GameObject _AvailableBackgroundContainer;

  [SerializeField]
  public GameObject _UnlockedBackgroundContainer;

  [SerializeField]
  public GameObject _ComingSoonContainer;

  [SerializeField]
  public Image _UnlockedIconSprite;

  [SerializeField]
  public GameObject _AffordableIndicator;

  [SerializeField]
  public CozmoButton _TileButton;

  private UnlockableInfo _UnlockData;

  public UnlockableInfo GetUnlockData() {
    return _UnlockData;
  }

  public void Initialize(UnlockableInfo unlockableData, CozmoUnlocksPanel.CozmoUnlockState unlockState, string dasViewController) {
    _UnlockData = unlockableData;

    gameObject.name = unlockableData.Id.Value.ToString();
    string dasButtonName = gameObject.name + " " + unlockState.ToString();

    _TileButton.Initialize(HandleButtonTapped, dasButtonName, dasViewController);

    _TileButton.Text = Localization.Get(unlockableData.TitleKey);

    _TileButton.onPress.AddListener(HandlePointerDown);
    _TileButton.onRelease.AddListener(HandlePointerUp);

    _LockedBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Locked || unlockState == CozmoUnlocksPanel.CozmoUnlockState.NeverAvailable);
    _AvailableBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable);
    _UnlockedBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlocked);
    _ComingSoonContainer.gameObject.SetActive(false);

    Cozmo.Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;

    _UnlockedIconSprite.sprite = unlockableData.CoreUpgradeIcon;
    switch (unlockState) {
    case CozmoUnlocksPanel.CozmoUnlockState.Locked:
      _UnlockedIconSprite.gameObject.SetActive(false);
      break;
    case CozmoUnlocksPanel.CozmoUnlockState.Unlockable:
      bool affordable = playerInventory.CanRemoveItemAmount(unlockableData.UpgradeCostItemId, unlockableData.UpgradeCostAmountNeeded);
      _AffordableIndicator.gameObject.SetActive(affordable);
      _UnlockedIconSprite.color = new Color(_UnlockedIconSprite.color.r, _UnlockedIconSprite.color.g, _UnlockedIconSprite.color.b, kLockedAlpha);
      break;
    case CozmoUnlocksPanel.CozmoUnlockState.Unlocked:
      _AffordableIndicator.gameObject.SetActive(false);
      _UnlockedIconSprite.color = new Color(_UnlockedIconSprite.color.r, _UnlockedIconSprite.color.g, _UnlockedIconSprite.color.b, 1.0f);
      break;
    case CozmoUnlocksPanel.CozmoUnlockState.NeverAvailable:
      _AffordableIndicator.gameObject.SetActive(false);
      _UnlockedIconSprite.gameObject.SetActive(false);
      _ComingSoonContainer.gameObject.SetActive(true);
      break;
    }

  }

  private void HandlePointerDown() {
    _TileButton.transform.localScale = new Vector3(0.9f, 0.9f, 1.0f);
  }

  private void HandlePointerUp() {
    _TileButton.transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
  }

  private void HandleButtonTapped() {
    if (OnTapped != null) {
      OnTapped(_UnlockData);
    }
  }
}
