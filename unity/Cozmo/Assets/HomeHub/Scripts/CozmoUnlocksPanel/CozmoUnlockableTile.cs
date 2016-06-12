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
  public CozmoButton _TileButton;

  private UnlockableInfo _UnlockData;

  public void Initialize(UnlockableInfo unlockableData, CozmoUnlocksPanel.CozmoUnlockState unlockState, string dasViewController,
                        bool isBeginningTile, Sprite beginningSprite, bool isEndTile, Sprite endSprite) {
    _UnlockData = unlockableData;

    gameObject.name = unlockableData.Id.Value.ToString();
    string dasButtonName = gameObject.name + " " + unlockState.ToString();

    _TileButton.Initialize(HandleButtonTapped, dasButtonName, dasViewController);

    if (unlockState != CozmoUnlocksPanel.CozmoUnlockState.Locked) {
      _TileButton.Text = Localization.Get(unlockableData.TitleKey);
    }
    else {
      _TileButton.Text = null;
    }

    _LockedBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Locked);
    _AvailableBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlockable);
    _UnlockedBackgroundContainer.SetActive(unlockState == CozmoUnlocksPanel.CozmoUnlockState.Unlocked);

    _BeginningCircuitySprite.gameObject.SetActive(isBeginningTile);
    _BeginningCircuitySprite.overrideSprite = beginningSprite;
    _EndCircuitrySprite.gameObject.SetActive(!isEndTile);
    _EndCircuitrySprite.overrideSprite = endSprite;

    // TODO set tint of the icon
    _UnlockedIconSprite.sprite = unlockableData.CoreUpgradeIcon;
    _UnlockedTintBackground.color = UIColorPalette.GetUpgradeTintData(unlockableData.CoreUpgradeTintColorName).TintColor;
  }

  private void HandleButtonTapped() {
    if (OnTapped != null) {
      OnTapped(_UnlockData);
    }
  }
}
