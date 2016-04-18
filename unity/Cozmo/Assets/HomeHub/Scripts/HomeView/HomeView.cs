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
    private UnityEngine.UI.Text _GreenPointsText;

    [SerializeField]
    private UnityEngine.UI.Text _GreenPointsTotal;

    [SerializeField]
    private UnityEngine.UI.ScrollRect _ScrollRect;

    private HomeHub _HomeHubInstance;

    public HomeHub HomeHubInstance {
      get { return _HomeHubInstance; }
      private set { _HomeHubInstance = value; }
    }

    public delegate void ButtonClickedHandler(string challengeClicked,Transform buttonTransform);

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

      _CozmoTabButton.Initialize(HandleCozmoTabButton, "switch_to_cozmo_tab_button", DASEventViewName);
      _PlayTabButton.Initialize(HandlePlayTabButton, "switch_to_play_tab_button", DASEventViewName);
      _ProfileTabButton.Initialize(HandleProfileTabButton, "switch_to_profile_tab_button", DASEventViewName);

      // TODO: Listen for inventory changing instead
      PlayerManager.Instance.GreenPointsUpdate += HandleGreenPointsGained;
      PlayerManager.Instance.ChestGained += HandleChestGained;
      UpdateGreenBar(DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints, PlayerManager.Instance.GetGreenPointsLadderMax());

    }

    private void HandleChestGained(int treatsGained, int hexGained) {
      _GreenPointsProgressBar.ResetProgress();
    }

    private void HandleGreenPointsGained(int greenPoints, int maxPoints) {
      UpdateGreenBar(greenPoints, maxPoints);
    }

    private void UpdateGreenBar(int greenPoints, int maxPoints) {
      _GreenPointsProgressBar.SetProgress((float)greenPoints / (float)maxPoints);
      _GreenPointsText.text = greenPoints.ToString();
      _GreenPointsTotal.text = maxPoints.ToString();
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
      PlayerManager.Instance.GreenPointsUpdate -= HandleGreenPointsGained;
      PlayerManager.Instance.ChestGained -= HandleChestGained;
    }

  }
}