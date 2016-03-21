using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using Cozmo.UI;
using Cozmo.HubWorld;

namespace Cozmo.HomeHub {
  public class HomeHubChallengeListView : MonoBehaviour {

    [SerializeField]
    private HubWorldButton _UnlockedChallengeButtonPrefab;

    [SerializeField]
    private HubWorldButton _LockedButtonPrefab;

    [SerializeField]
    private RectTransform _TopChallengeContainer;

    [SerializeField]
    private RectTransform _BottomChallengeContainer;

    public event TimelineView.ButtonClickedHandler OnLockedChallengeClicked;
    public event TimelineView.ButtonClickedHandler OnUnlockedChallengeClicked;

    private readonly Dictionary<string, GameObject> _ChallengeButtons = new Dictionary<string, GameObject>();

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById, string dasParentViewName) {
      foreach (KeyValuePair<string, ChallengeStatePacket> kvp in challengeStatesById) {
        if (kvp.Value.currentState == ChallengeState.Locked) {
          _ChallengeButtons.Add(kvp.Value.data.ChallengeID, 
            CreateChallengeButton(kvp.Value.data, _LockedButtonPrefab.gameObject, 
              HandleLockedChallengeClicked, dasParentViewName));
        }
        else {
          _ChallengeButtons.Add(kvp.Value.data.ChallengeID, 
            CreateChallengeButton(kvp.Value.data, _UnlockedChallengeButtonPrefab.gameObject, 
              HandleUnlockedChallengeClicked, dasParentViewName));
        }
      }
    }

    private GameObject CreateChallengeButton(ChallengeData challengeData, GameObject prefab, 
                                             HubWorldButton.ButtonClickedHandler handler, string dasParentViewName) {
      RectTransform container;
      if (_TopChallengeContainer.childCount < _BottomChallengeContainer.childCount) {
        container = _TopChallengeContainer;
      }
      else {
        container = _BottomChallengeContainer;
      }
      GameObject newButton = UIManager.CreateUIElement(prefab, container);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData, dasParentViewName);
      buttonScript.OnButtonClicked += handler;
      return newButton;
    }

    private void HandleLockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnLockedChallengeClicked != null) {
        OnLockedChallengeClicked(challengeClicked, buttonTransform);
      }
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnUnlockedChallengeClicked != null) {
        OnUnlockedChallengeClicked(challengeClicked, buttonTransform);
      }
    }

  }

}