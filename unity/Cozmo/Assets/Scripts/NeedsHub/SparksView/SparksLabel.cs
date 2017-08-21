using UnityEngine;
using DataPersistence;
using Anki.Cozmo.ExternalInterface;

namespace Cozmo {
  namespace UI {

    /// <summary>
    /// Sparks label has some specialized functionality as compared to ItemDataLabel
    ///   initializes count to last displayed value from persistent data
    ///   does not refresh count while there is a reward crate waiting to be displayed
    /// </summary>
    public class SparksLabel : MonoBehaviour {

      [Cozmo.ItemId]
      private string _SparkItemId = "spark";

      [SerializeField]
      private CozmoText _CountLabel;

      [SerializeField]
      private GameObject _ItemIconAnimatorPrefab;

      [SerializeField]
      private Transform _ItemIconAnimatorContainer;

      [SerializeField]
      private float _ItemIconAnimatorLocalScale = 1f;

      [SerializeField]
      private string _ItemIconBurstAnimStateName = "SparksBurstAnimation";

      private int _ItemIconBurstAnimHash;
      private Animator _ItemIconAnimatorInstance;

      private bool BlockInventoryRefresh {
        get {
          return (DataPersistenceManager.Instance != null && DataPersistenceManager.Instance.StarLevelToDisplay);
        }
      }

      private void Start() {
        Inventory.ItemCountUpdated += HandleItemValueChanged;
        Inventory.ItemAdded += HandleItemValueChanged;
        Inventory.ItemRemoved += HandleItemValueChanged;
        Inventory.ItemCountSet += HandleItemValueChanged;

        RobotEngineManager.Instance.AddCallback<FreeplaySparksAwarded>(AddFreeplaySparks);

        Cozmo.Needs.UI.NeedsRewardModal.CrateSparksRewardDisplayed += SetToActualCount;
        //if we are waiting on a crate, just show the last displayed spark count
        if (DataPersistenceManager.Instance.StarLevelToDisplay) {
          SetCountText(DataPersistenceManager.Instance.DisplayedSparks);
        }
        else {
          SetToActualCount();
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
        Inventory.ItemCountUpdated -= HandleItemValueChanged;
        Inventory.ItemAdded -= HandleItemValueChanged;
        Inventory.ItemRemoved -= HandleItemValueChanged;
        Inventory.ItemCountSet -= HandleItemValueChanged;
        if (RobotEngineManager.Instance != null) {
          RobotEngineManager.Instance.RemoveCallback<FreeplaySparksAwarded>(AddFreeplaySparks);
        }
        Cozmo.Needs.UI.NeedsRewardModal.CrateSparksRewardDisplayed -= SetToActualCount;
      }

      private void HandleItemValueChanged(string itemId, int delta, int newCount) {
        if (!BlockInventoryRefresh) {
          if (itemId == _SparkItemId) {
            if (delta > 0) {
              if (_ItemIconAnimatorInstance != null && _ItemIconBurstAnimHash != 0) {
                _ItemIconAnimatorInstance.Play(_ItemIconBurstAnimHash);
              }
              else {
                DAS.Error("SparksLabel.HandleItemValueChanged.NoAnimator", string.Format("Missing animator for {0}, item id: {1}, count: {2}, animName: {3}",
                                name, itemId, newCount, _ItemIconBurstAnimStateName));
              }
            }
            SetCountText(newCount);
          }
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
          DAS.Error("SparksLabel.SetCountText", string.Format("Missing Count Label for {0}, item id: {1}, count: {2}, animName: {3}",
                          name, _SparkItemId, newCount, _ItemIconBurstAnimStateName));
        }

        DataPersistenceManager.Instance.DisplayedSparks = newCount;
      }

      private void SetToActualCount() {
        int actualCount = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(_SparkItemId);
        SetCountText(actualCount);
      }

      private void AddFreeplaySparks(FreeplaySparksAwarded msg) {
        //if we're refreshing from inventory, then we don't need to accrue freeplay sparks here
        if (BlockInventoryRefresh) {
          SetCountText(DataPersistenceManager.Instance.DisplayedSparks + msg.sparksAwarded);
        }
      }
    }
  }
}
