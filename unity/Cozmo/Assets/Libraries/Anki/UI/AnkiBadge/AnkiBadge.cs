using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections.Generic;

namespace Anki {
  namespace UI {
    
    public class AnkiBadge : BaseBehaviour {

      [SerializeField]
      private string
        _badgeType;

      private int _badgeNumber;
      private GameObject _badgePrefab;
      private AnkiBadgeCell _badgeCell;
      private bool _hasUpdated;

      public event Action<int> OnNumberUpdate;

      void Start() {
        _badgePrefab = PrefabLoader.Instance.InstantiatePrefab("AnkiBadgePrefab");
        _badgePrefab.transform.SetParent(this.gameObject.transform, false);
        _badgeCell = _badgePrefab.GetComponent<AnkiBadgeCell>();
        OnUpdate(AnkiBadgeManager.GetBadgeNumber(_badgeType));

        // We still want the listener active if this is disabled, so this here instead of RegisterCallbacks()
        AnkiBadgeManager.AddListener(_badgeType, OnUpdate);
      }

      void OnDestroy() {
        // We still want the listener active if this is disabled, so this here instead of UnregisterCallbacks()
        AnkiBadgeManager.RemoveListener(_badgeType, OnUpdate);
      }

      public void OnUpdate(int count) {
        int oldNumber = _badgeNumber;
        
        if( oldNumber != count ) {
          DAS.Event("ui.badge", _badgeType, new Dictionary<string,string>{{"$data", string.Format("{0}", count)}});
        }
        
        _badgeNumber = count;
        if (_badgeNumber == 0) {
          _badgePrefab.SetActive(false);
        }
        else {
          _badgePrefab.SetActive(true);
          _badgeCell.Number.text = _badgeNumber.ToString();
        }
        bool invokeCallback = !_hasUpdated || oldNumber != _badgeNumber;
        if (invokeCallback && OnNumberUpdate != null) {
          OnNumberUpdate(_badgeNumber);
        }
        _hasUpdated = true;
      }

    }
  }
}
