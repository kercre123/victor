using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class ItemDataLabel : MonoBehaviour {
      [SerializeField, Cozmo.ItemId]
      private string _ItemId;

      [SerializeField]
      private CozmoText _CountLabel;

      [SerializeField]
      private UnityEngine.UI.Image _ItemIcon;

      [SerializeField]
      private GameObject _ItemIconAnimatorPrefab;

      [SerializeField]
      private Transform _ItemIconAnimatorContainer;

      [SerializeField]
      private float _ItemIconAnimatorLocalScale = 1f;

      [SerializeField]
      private string _ItemIconBurstAnimStateName;

      private int _ItemIconBurstAnimHash;
      private Animator _ItemIconAnimatorInstance;

      private void Start() {
        Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
        Inventory.ItemAdded += HandleItemValueChanged;
        Inventory.ItemRemoved += HandleItemValueChanged;
        Inventory.ItemCountSet += HandleItemValueChanged;
        Inventory.ItemCountUpdated += HandleItemValueChanged;

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

        if (_ItemIconAnimatorPrefab != null
            && _ItemIconAnimatorContainer != null
            && !string.IsNullOrEmpty(_ItemIconBurstAnimStateName)) {
          GameObject newIconAnimator = UIManager.CreateUIElement(_ItemIconAnimatorPrefab, _ItemIconAnimatorContainer);
          _ItemIconAnimatorInstance = newIconAnimator.GetComponent<Animator>();
          _ItemIconBurstAnimHash = Animator.StringToHash(_ItemIconBurstAnimStateName);
          if (_ItemIconAnimatorInstance != null) {
            _ItemIconAnimatorInstance.transform.localScale = Vector3.one * _ItemIconAnimatorLocalScale;
          }
          else {
            DAS.Error("ItemDataLabel.Start.AnimatorNotFound",
                      "_ItemIconAnimatorPrefab needs to have an Animator component!",
                      new System.Collections.Generic.Dictionary<string, string>() { { "gameObject", gameObject.name } });
          }
        }
        else if (!(_ItemIconAnimatorPrefab == null
                   && _ItemIconAnimatorContainer == null
                   && string.IsNullOrEmpty(_ItemIconBurstAnimStateName))) {
          DAS.Error("ItemDataLabel.Start.AnimatorCreatePartNull",
                    "Animator prefab, Transform container, and Anim State name need to be assigned for Animation!",
                    new System.Collections.Generic.Dictionary<string, string>() { { "gameObject", gameObject.name } });
        }
      }

      private void OnDestroy() {
        Inventory.ItemAdded -= HandleItemValueChanged;
        Inventory.ItemRemoved -= HandleItemValueChanged;
        Inventory.ItemCountSet -= HandleItemValueChanged;
        Inventory.ItemCountUpdated -= HandleItemValueChanged;
      }

      private void HandleItemValueChanged(string itemId, int delta, int newCount) {
        if (itemId == _ItemId) {
          if (delta > 0) {
            if (_ItemIconAnimatorInstance != null && _ItemIconBurstAnimHash != 0) {
              _ItemIconAnimatorInstance.Play(_ItemIconBurstAnimHash);
            }
            else {
              DAS.Error("ItemDataLabel.HandleItemValueChanged",
                        string.Format("Missing animator for {0}, item id: {1}, count: {2}, animName: {3}",
                        name, itemId, newCount, _ItemIconBurstAnimStateName));
            }
          }
          SetCountText(newCount);
        }
      }

      private void SetCountText(int newCount) {
        if (_CountLabel != null) {
          _CountLabel.text = Localization.GetNumber(newCount);

          // Force the rect transform to update for use with ContentSizeFitters
          _CountLabel.gameObject.SetActive(false);
          _CountLabel.gameObject.SetActive(true);
        }
        else {
          DAS.Error(this, string.Format("Missing Count Label for {0}, item id: {1}, count: {2}, animName: {3}",
                                        name, _ItemId, newCount, _ItemIconBurstAnimStateName));
        }
      }
    }
  }
}
