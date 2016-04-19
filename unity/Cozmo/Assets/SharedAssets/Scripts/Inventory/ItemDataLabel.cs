using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class ItemDataLabel : MonoBehaviour {
      [SerializeField]
      private Anki.UI.AnkiTextLabel _CountLabel;

      [SerializeField]
      private string _ItemId;

      private void Start() {
        Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        _CountLabel.FormattingArgs = new object[] { playerInventory.GetItemAmount(_ItemId) };
        playerInventory.ItemAdded += HandleItemValueChanged;
        playerInventory.ItemRemoved += HandleItemValueChanged;
        playerInventory.ItemCountSet += HandleItemValueChanged;
      }

      private void OnDestroy() {
        Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        playerInventory.ItemAdded -= HandleItemValueChanged;
        playerInventory.ItemRemoved -= HandleItemValueChanged;
        playerInventory.ItemCountSet -= HandleItemValueChanged;
      }

      private void HandleItemValueChanged(string itemId, int delta, int newCount) {
        if (itemId == _ItemId) {
          _CountLabel.FormattingArgs = new object[] { newCount };
        }
      }
    }
  }
}
