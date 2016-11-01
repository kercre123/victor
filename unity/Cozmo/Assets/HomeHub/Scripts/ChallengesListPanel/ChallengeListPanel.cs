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
      LoadTiles();
      UnlockablesManager.Instance.OnUnlockComplete += HandleUnlockCompleted;
    }

    void OnDestroy() {
      UnlockablesManager.Instance.OnUnlockComplete -= HandleUnlockCompleted;
    }

    private void LoadTiles() {
      ClearTiles();

      List<KeyValuePair<string, ChallengeStatePacket>> sortedDict = new List<KeyValuePair<string, ChallengeStatePacket>>();
      sortedDict.AddRange(GetHomeViewInstance().GetChallengeStates());

      // Sort by unlocked first, then by designer order
      sortedDict.Sort((KeyValuePair<string, ChallengeStatePacket> a, KeyValuePair<string, ChallengeStatePacket> b) => {
        UnlockableInfo aInfo = UnlockablesManager.Instance.GetUnlockableInfo(a.Value.Data.UnlockId.Value);
        UnlockableInfo bInfo = UnlockablesManager.Instance.GetUnlockableInfo(b.Value.Data.UnlockId.Value);
        return aInfo.SortOrder - bInfo.SortOrder;
      });

      for (int i = 0; i < sortedDict.Count; i++) {
        KeyValuePair<string, ChallengeStatePacket> kvp = sortedDict[i];
        _ChallengeButtons.Add(kvp.Value.Data.ChallengeID,
          CreateChallengeButton(kvp.Value.Data, _UnlockedChallengeButtonPrefab.gameObject,
            HandleUnlockedChallengeClicked, "home_hub_challenge_list_panel"));
      }
    }

    private void ClearTiles() {
      foreach (GameObject button in _ChallengeButtons.Values) {
        button.GetComponent<HubWorldButton>().OnButtonClicked -= HandleUnlockedChallengeClicked;
        // this is necessary because CreateChallengeButton relies on the childCount to be correct
        GameObject.DestroyImmediate(button);
      }
      _ChallengeButtons.Clear();
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
      bool isAvailable = UnlockablesManager.Instance.IsUnlockableAvailable(challengeData.UnlockId.Value);
      buttonScript.Initialize(challengeData, dasParentViewName, true, isNew, UnlockablesManager.Instance.IsUnlocked(challengeData.UnlockId.Value), isAvailable);
      buttonScript.OnButtonClicked += handler;
      return newButton;
    }

    private void HandleUnlockCompleted(Anki.Cozmo.UnlockId unlockID) {
      LoadTiles();
    }

    private void HandleNewChallengeUnlock(UnlockableInfo unlockInfo) {
      UnlockablesManager.Instance.TrySetUnlocked(unlockInfo.Id.Value, true);
    }

    private void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      base.GetHomeViewInstance().HandleUnlockedChallengeClicked(challengeClicked, buttonTransform);
    }

  }

}