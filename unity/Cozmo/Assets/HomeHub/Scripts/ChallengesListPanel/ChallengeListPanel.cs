using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using Cozmo.UI;
using Cozmo.HubWorld;

namespace Cozmo.HomeHub {
  public class ChallengeListPanel : TabPanel {

    [SerializeField]
    private HubWorldButton _UnlockedChallengeButtonPrefab;

    [SerializeField]
    private RectTransform _TopChallengeContainer;

    [SerializeField]
    private RectTransform _BottomChallengeContainer;

    private readonly Dictionary<string, GameObject> _ChallengeButtons = new Dictionary<string, GameObject>();

    public override void Initialize(HomeView homeViewInstance) {
      base.Initialize(homeViewInstance);
      foreach (KeyValuePair<string, ChallengeStatePacket> kvp in homeViewInstance.GetChallengeStates()) {
        if (kvp.Value.ChallengeUnlocked) {
          _ChallengeButtons.Add(kvp.Value.Data.ChallengeID,
            CreateChallengeButton(kvp.Value.Data, _UnlockedChallengeButtonPrefab.gameObject,
              HandleUnlockedChallengeClicked, "home_hub_challenge_list_panel"));
        }
      }
      UnlockablesManager.Instance.ResolveNewUnlocks();
    }

    private GameObject CreateChallengeButton(ChallengeData challengeData, GameObject prefab,
                                             HubWorldButton.ButtonClickedHandler handler, string dasParentViewName) {
      RectTransform container;
      if (_BottomChallengeContainer.childCount < _TopChallengeContainer.childCount) {
        container = _BottomChallengeContainer;
      }
      else {
        container = _TopChallengeContainer;
      }
      GameObject newButton = UIManager.CreateUIElement(prefab, container);
      HubWorldButton buttonScript = newButton.GetComponent<HubWorldButton>();
      bool isNew = UnlockablesManager.Instance.IsNewUnlock(challengeData.UnlockId.Value);
      buttonScript.Initialize(challengeData, dasParentViewName, true, isNew);
      buttonScript.OnButtonClicked += handler;
      return newButton;
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      base.GetHomeViewInstance().HandleUnlockedChallengeClicked(challengeClicked, buttonTransform);
    }

  }

}