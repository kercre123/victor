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

      [SerializeField]
      private bool _CountOnly = false;

      private void Start() {
        Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.ItemAdded += HandleItemValueChanged;
        playerInventory.ItemRemoved += HandleItemValueChanged;
        playerInventory.ItemCountSet += HandleItemValueChanged;
        SetFormattingArgs(playerInventory.GetItemAmount(_ItemId));

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
      }

      private void HandleItemValueChanged(string itemId, int delta, int newCount) {
        if (itemId == _ItemId) {
          SetFormattingArgs(newCount);
        }
      }

      private void SetFormattingArgs(int newCount) {
        if (_CountOnly) {
          _CountLabel.FormattingArgs = new object[] { newCount };
        }
        else {
          _CountLabel.FormattingArgs = new object[] { GetItemNamePlural(), newCount };
        }
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
