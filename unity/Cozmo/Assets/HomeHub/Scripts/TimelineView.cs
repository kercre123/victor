using System;
using UnityEngine;
using System.Collections.Generic;

namespace Cozmo.HomeHub {
  public class TimelineView : MonoBehaviour {
    
    public delegate void ButtonClickedHandler(string challengeClicked, Transform buttonTransform);

    public event ButtonClickedHandler OnLockedChallengeClicked;
    public event ButtonClickedHandler OnUnlockedChallengeClicked;
    public event ButtonClickedHandler OnCompletedChallengeClicked;

    [SerializeField]
    HomeHubChallengeListView _ChallengeListViewPrefab;
    HomeHubChallengeListView _ChallengeListViewInstance;

    public void CloseView() {
      // TODO: Play some close animations before destroying view
      GameObject.Destroy(gameObject);
    }

    public void CloseViewImmediately() {
      GameObject.Destroy(gameObject);
    }

    public void OnDestroy() {
      if (_ChallengeListViewInstance != null) {
        GameObject.Destroy(_ChallengeListViewInstance.gameObject);
      }
    }

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeListViewInstance = (GameObject.Instantiate(_ChallengeListViewPrefab.gameObject) as GameObject).GetComponent<HomeHubChallengeListView>();
      _ChallengeListViewInstance.Initialize(challengeStatesById);
    }
  }
}

