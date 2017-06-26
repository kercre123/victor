using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class ItemDataLabel : MonoBehaviour {
      [SerializeField]
      private CozmoText _CountLabel;

      [SerializeField]
      private UnityEngine.UI.Image _ItemIcon;

      [SerializeField, Cozmo.ItemId]
      private string _ItemId;

      private void Start() {
        Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.ItemAdded += HandleItemValueChanged;
        playerInventory.ItemRemoved += HandleItemValueChanged;
        playerInventory.ItemCountSet += HandleItemValueChanged;
        playerInventory.ItemCountUpdated += HandleItemValueChanged;
        SetCountText(playerInventory.GetItemAmount(_ItemId));

        if (_ItemIcon != null) {
          ItemData data = ItemDataConfig.GetData(_ItemId);
          if (data != null) {
            _ItemIcon.overrideSprite = data.Icon;
          }
          else {
            _ItemIcon.enabled = false;
          }
        }
      }

      private void OnDestroy() {
        Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.ItemAdded -= HandleItemValueChanged;
        playerInventory.ItemRemoved -= HandleItemValueChanged;
        playerInventory.ItemCountSet -= HandleItemValueChanged;
        playerInventory.ItemCountUpdated -= HandleItemValueChanged;
      }

      private void HandleItemValueChanged(string itemId, int delta, int newCount) {
        if (itemId == _ItemId) {
          SetCountText(newCount);
        }
      }

      private void SetCountText(int newCount) {
		_CountLabel.text = Localization.GetNumber(newCount);

		// Force the rect transform to update for use with ContentSizeFitters
		_CountLabel.gameObject.SetActive(false);
		_CountLabel.gameObject.SetActive(true);
      }

      private string GetItemNamePlural() {
        ItemData itemData = ItemDataConfig.GetData(_ItemId);
        string itemName = "(null)";
        if (itemData != null) {
          itemName = itemData.GetPluralName();
        }
        return itemName;
      }
    }
  }
}
