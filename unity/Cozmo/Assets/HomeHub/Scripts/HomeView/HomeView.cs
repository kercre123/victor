using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.UI;
using Cozmo.UI;
using DataPersistence;
using System.Linq;
using System;

namespace Cozmo.HomeHub {
  public class HomeView : BaseView {

    public System.Action<StatContainer, StatContainer, Transform[]> DailyGoalsSet;

    [SerializeField]
    private HomeViewTab _CozmoTabPrefab;

    [SerializeField]
    private HomeViewTab _PlayTabPrefab;

    [SerializeField]
    private HomeViewTab _ProfileTabPrefab;

    private HomeViewTab _CurrentTab;

    [SerializeField]
    private CozmoButton _CozmoTabButton;

    [SerializeField]
    private CozmoButton _PlayTabButton;

    [SerializeField]
    private CozmoButton _ProfileTabButton;

    [SerializeField]
    private RectTransform _ScrollRectContent;

    [SerializeField]
    private Cozmo.UI.ProgressBar _GreenPointsProgressBar;

    [SerializeField]
    private UnityEngine.UI.ScrollRect _ScrollRect;

    private HomeHub _HomeHubInstance;

    public HomeHub HomeHubInstance {
      get { return _HomeHubInstance; }
      private set { _HomeHubInstance = value; }
    }

    public delegate void ButtonClickedHandler(string challengeClicked, Transform buttonTransform);

    public event ButtonClickedHandler OnLockedChallengeClicked;
    public event ButtonClickedHandler OnUnlockedChallengeClicked;
    public event ButtonClickedHandler OnCompletedChallengeClicked;
    public event Action OnEndSessionClicked;

    private Dictionary<string, ChallengeStatePacket> _ChallengeStates;

    public void Initialize(Dictionary<string, ChallengeStatePacket> challengeStatesById, HomeHub homeHubInstance) {

      _HomeHubInstance = homeHubInstance;

      DASEventViewName = "home_view";

      _ChallengeStates = challengeStatesById;

      _CurrentTab = GameObject.Instantiate(_PlayTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);

      _CozmoTabButton.DASEventButtonName = "cozmo_tab_botton";
      _PlayTabButton.DASEventButtonName = "play_tab_button";
      _ProfileTabButton.DASEventButtonName = "profile_tab_button";

      _CozmoTabButton.DASEventViewController = "home_view";
      _PlayTabButton.DASEventViewController = "home_view";
      _ProfileTabButton.DASEventViewController = "home_view";

      _CozmoTabButton.onClick.AddListener(HandleCozmoTabButton);
      _PlayTabButton.onClick.AddListener(HandlePlayTabButton);
      _ProfileTabButton.onClick.AddListener(HandleProfileTabButton);

      PlayerManager.Instance.GreenPointsUpdate += HandleGreenPointsGained;
      PlayerManager.Instance.ChestGained += HandleChestGained;
      _GreenPointsProgressBar.SetProgress((float)DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints / (float)PlayerManager.Instance.GetGreenPointsLadderMax());

    }

    private void HandleChestGained() {
      _GreenPointsProgressBar.ResetProgress();
    }

    private void HandleGreenPointsGained(int greenPoints, int maxPoints) {
      _GreenPointsProgressBar.SetProgress((float)greenPoints / (float)maxPoints);
    }

    public void SetChallengeStates(Dictionary<string, ChallengeStatePacket> challengeStatesById) {
      _ChallengeStates = challengeStatesById;
    }

    private void HandleCozmoTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTab = GameObject.Instantiate(_CozmoTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);
    }

    private void HandlePlayTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTab = GameObject.Instantiate(_PlayTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);
    }

    private void HandleProfileTabButton() {
      if (_CurrentTab != null) {
        GameObject.Destroy(_CurrentTab.gameObject);
      }
      _ScrollRect.horizontalNormalizedPosition = 0.0f;
      _CurrentTab = GameObject.Instantiate(_ProfileTabPrefab.gameObject).GetComponent<HomeViewTab>();
      _CurrentTab.transform.SetParent(_ScrollRectContent, false);
      _CurrentTab.Initialize(this);
    }

    public Dictionary<string, ChallengeStatePacket> GetChallengeStates() {
      return _ChallengeStates;
    }

    public void HandleLockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnLockedChallengeClicked != null) {
        OnLockedChallengeClicked(challengeClicked, buttonTransform);
      } 
    }

    public void HandleUnlockedChallengeClicked(string challengeClicked, Transform buttonTransform) {
      if (OnUnlockedChallengeClicked != null) {
        OnUnlockedChallengeClicked(challengeClicked, buttonTransform);
      } 
    }

    protected override void CleanUp() {
      
    }

  }
}