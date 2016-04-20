using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class ItemDataLabel : MonoBehaviour {
      [SerializeField]
      private Anki.UI.AnkiTextLabel _CountLabel;

      [SerializeField, Cozmo.ItemId]
      private string _ItemId;

      private void Start() {
        Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.ItemAdded += HandleItemValueChanged;
        playerInventory.ItemRemoved += HandleItemValueChanged;
        playerInventory.ItemCountSet += HandleItemValueChanged;
        _CountLabel.FormattingArgs = new object[] { GetItemNamePlural(), playerInventory.GetItemAmount(_ItemId) };
      }

      private void OnDestroy() {
        Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.ItemAdded -= HandleItemValueChanged;
        playerInventory.ItemRemoved -= HandleItemValueChanged;
        playerInventory.ItemCountSet -= HandleItemValueChanged;
      }

      private void HandleItemValueChanged(string itemId, int delta, int newCount) {
        if (itemId == _ItemId) {
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
