using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using Cozmo.UI;
using Cozmo.HubWorld;

namespace Cozmo.HomeHub {
  public class HomeHubChallengeListView : BaseView {

    [SerializeField]
    private HubWorldButton _UnlockedChallengeButtonPrefab;

    [SerializeField]
    private HubWorldButton _LockedButtonPrefab;

    [SerializeField]
    private RectTransform _TopChallengeContainer;

    [SerializeField]
    private RectTransform _BottomChallengeContainer;

    public event HubWorldButton.ButtonClickedHandler OnLockedChallengeClicked;
    public event HubWorldButton.ButtonClickedHandler OnUnlockedChallengeClicked;

    private readonly Dictionary<string, GameObject> _ChallengeButtons = new Dictionary<string, GameObject>();

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      foreach (KeyValuePair<string, ChallengeStatePacket> kvp in challengeStatesById) {
        if (kvp.Value.currentState == ChallengeState.UNLOCKED) {
          _ChallengeButtons.Add(kvp.Value.data.ChallengeID, CreateChallengeButton(kvp.Value.data, _UnlockedChallengeButtonPrefab.gameObject, HandleUnlockedChallengeClicked));
        }
        else if (kvp.Value.currentState == ChallengeState.LOCKED) {
          _ChallengeButtons.Add(kvp.Value.data.ChallengeID, CreateChallengeButton(kvp.Value.data, _LockedButtonPrefab.gameObject, HandleLockedChallengeClicked));
        }
      }
    }

    private GameObject CreateChallengeButton(ChallengeData challengeData, GameObject prefab, HubWorldButton.ButtonClickedHandler handler) {
      RectTransform container;
      if (_TopChallengeContainer.childCount > _BottomChallengeContainer.childCount) {
        container = _BottomChallengeContainer;
      }
      else {
        container = _TopChallengeContainer;
      }
      GameObject newButton = UIManager.CreateUIElement(prefab, container);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      buttonScript.Initialize(challengeData);
      buttonScript.OnButtonClicked += handler;
      return newButton;
    }

    protected override void CleanUp() {
      foreach (KeyValuePair<string, GameObject> kvp in _ChallengeButtons) {
        GameObject.Destroy(kvp.Value);
      }
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