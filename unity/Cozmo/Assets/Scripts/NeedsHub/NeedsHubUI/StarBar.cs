using Cozmo.UI;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using DataPersistence;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo.Needs.UI {
  public class StarBar : MonoBehaviour {

    #region Nested Definitions

    public enum RewardState {
      IDLE,   //no change in stars or star level rewards detected
      BLING,  //newly earned stars are about to be display, spawn bling effect over needs meter
      STARS,  //spawn or despawn stars to match the current count from engine
      CRATE,  //spawn a crate icon
      MODAL,   //spawn the reward modal and hand it our accumulated star level rewards to display to user
      ONBOARDING_WAIT_FOR_FILL, // before bling in onboarding, show an explination, wait for click to continue
    }

    #endregion //Nested Definitions

    #region Serialized Fields

    [SerializeField]
    private RectTransform _BlingAnchor = null;

    [SerializeField]
    private GameObject _BlingPrefab = null;

    [SerializeField]
    private float _BlingDuration_sec = 2f;

    [SerializeField]
    private RectTransform _StarAnchor = null;

    [SerializeField]
    private GameObject _StarPrefab = null;

    [SerializeField]
    private float _StarDuration_sec = 2f;

    [SerializeField]
    private RectTransform _CrateAnchor = null;

    [SerializeField]
    private GameObject _CratePrefab = null;

    [SerializeField]
    private float _CrateDuration_sec = 2f;

    [SerializeField]
    private NeedsRewardModal _RewardModalPrefab;

    [SerializeField]
    private Button _AddStarDebugButton = null;

    #endregion //Serialized Fields

    #region Non-serialized Fields

    private RewardState _CurrentState = RewardState.IDLE;
    private float _TimeInState_sec = 0f;

    private List<GameObject> _DisplayedStars = new List<GameObject>();

    private GameObject _BlingEffect = null;
    private GameObject _CrateIcon = null;
    private NeedsRewardModal _NeedsModal = null;

    #endregion //Non-serialized Fields

    #region Life Span

    // Some events will go here on clicking, etc...
    private void Start() {
      if (_AddStarDebugButton != null) {
#if UNITY_EDITOR
        _AddStarDebugButton.gameObject.SetActive(true);
#else
        _AddStarDebugButton.gameObject.SetActive(false);
#endif
      }

      UpdateStars(DataPersistenceManager.Instance.DisplayedStars);

      // Make really sure we have up to date info
      RobotEngineManager.Instance.Message.GetNeedsState = Singleton<GetNeedsState>.Instance;
      RobotEngineManager.Instance.SendMessage();

      EnterState();
    }

    private void Update() {

      bool pause = UIManager.Instance.AreAnyModalsOpen;

#if ANKI_DEV_CHEATS
      pause |= (DebugMenuManager.Instance != null && DebugMenuManager.Instance.IsDialogOpen());
#endif

      if (!pause) {
        RefreshStateLogic(Time.deltaTime);
      }
    }

    private void OnDestroy() {
      ExitState();

      if (_BlingEffect != null) {
        GameObject.Destroy(_BlingEffect);
      }
      _BlingEffect = null;

      if (_CrateIcon != null) {
        GameObject.Destroy(_CrateIcon);
      }
      _CrateIcon = null;
    }

    #endregion //Life Span

    #region FSM

    private void RefreshStateLogic(float deltaTime) {
      if (!StateTransition()) {
        UpdateState(deltaTime);
      }
    }

    //transitions can ONLY happen within this method
    private bool StateTransition() {
      switch (_CurrentState) {
      case RewardState.IDLE:
        // start onboarding if we have one token and have completed the previous section of onboarding, but dont restart if already in it.
        if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.RewardBox) &&
            NeedsStateManager.Instance.GetLatestStarAwardedFromEngine() > 0 &&
            !OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.PlayIntro) &&
             OnboardingManager.Instance.GetCurrentPhase() != OnboardingManager.OnboardingPhases.RewardBox) {
          OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.RewardBox);
          return ChangeState(RewardState.ONBOARDING_WAIT_FOR_FILL);
        }
        if (DataPersistenceManager.Instance.StarLevelToDisplay) {
          return ChangeState(RewardState.BLING);
        }
        if (NeedsStateManager.Instance.GetLatestStarAwardedFromEngine() > _DisplayedStars.Count) {
          return ChangeState(RewardState.BLING);
        }
        if (NeedsStateManager.Instance.GetLatestStarAwardedFromEngine() < _DisplayedStars.Count) {
          return ChangeState(RewardState.STARS);
        }
        break;
      case RewardState.BLING:
        if (_TimeInState_sec >= _BlingDuration_sec) {
          return ChangeState(RewardState.STARS);
        }
        break;
      case RewardState.STARS:
        if (_TimeInState_sec >= _StarDuration_sec) {
          if (DataPersistenceManager.Instance.StarLevelToDisplay) {
            return ChangeState(RewardState.CRATE);
          }
          return ChangeState(RewardState.IDLE);
        }
        break;
      case RewardState.CRATE:
        if (_TimeInState_sec >= _CrateDuration_sec) {
          return ChangeState(RewardState.MODAL);
        }
        break;
      case RewardState.MODAL:
        if (_NeedsModal == null) {
          return ChangeState(RewardState.IDLE);
        }
        break;
      case RewardState.ONBOARDING_WAIT_FOR_FILL:
        // iff onboarding says we've reached stage 2, show the bling animation
        if (OnboardingManager.Instance.GetCurrentPhase() == OnboardingManager.OnboardingPhases.RewardBox) {
          if (OnboardingManager.Instance.GetCurrStageInPhase(OnboardingManager.OnboardingPhases.RewardBox) > 0) {
            return ChangeState(RewardState.BLING);
          }
        }
        else {
          // they've debug skipped past the phase
          return ChangeState(RewardState.BLING);
        }
        break;
      }
      return false;
    }

    private bool ChangeState(RewardState newState) {
      if (newState == _CurrentState) {
        return false;
      }

      ExitState();
      _CurrentState = newState;
      EnterState();

      return true;
    }

    private void EnterState() {
      _TimeInState_sec = 0f;

      switch (_CurrentState) {
      case RewardState.BLING:
        _BlingEffect = GameObject.Instantiate(_BlingPrefab, _BlingAnchor);
        break;
      case RewardState.STARS:
        int starToDisplay = NeedsStateManager.Instance.GetLatestStarAwardedFromEngine();
        if (DataPersistenceManager.Instance.StarLevelToDisplay) {
          starToDisplay = 3; //fake this if stars and rewards are out of sync
        }
        if (starToDisplay > _DisplayedStars.Count) {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Earn_Token);
        }
        UpdateStars(starToDisplay);
        break;
      case RewardState.CRATE:
        _CrateIcon = GameObject.Instantiate(_CratePrefab, _CrateAnchor);
        break;
      case RewardState.MODAL:
        UIManager.OpenModal(_RewardModalPrefab,
            new ModalPriorityData(),
            (BaseModal newModal) => {
              _NeedsModal = (NeedsRewardModal)newModal;
              _NeedsModal.Init(DataPersistenceManager.Instance.GetNeedsRewardsFromNewStarLevels());
              //empty our accrued rewards from the record once we show the modal
              DataPersistenceManager.Instance.ClearNewStarLevels();
            });
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Earn_Bonus_Box);

        // Make really sure we have up to date info
        RobotEngineManager.Instance.Message.GetNeedsState = Singleton<GetNeedsState>.Instance;
        RobotEngineManager.Instance.SendMessage();
        break;
      }
    }

    private void UpdateState(float deltaTime) {
      _TimeInState_sec += deltaTime;
    }

    private void ExitState() {
      switch (_CurrentState) {
      case RewardState.BLING:
        if (_BlingEffect != null) {
          GameObject.Destroy(_BlingEffect);
        }
        _BlingEffect = null;
        break;
      case RewardState.MODAL:
        if (_CrateIcon != null) {
          GameObject.Destroy(_CrateIcon);
        }
        _CrateIcon = null;
        //DataPersistenceManager.Instance.NewStars = 0;
        UpdateStars(0);
        break;
      }
    }

    #endregion //FSM

    #region Misc Helper Methods

    private void UpdateStars(int count) {
      while (_DisplayedStars.Count < count) {
        GameObject star = GameObject.Instantiate(_StarPrefab, _StarAnchor);
        _DisplayedStars.Add(star);
      }

      while (_DisplayedStars.Count > count && _DisplayedStars.Count > 0) {
        GameObject star = _DisplayedStars[_DisplayedStars.Count - 1];
        if (star != null) {
          GameObject.Destroy(star);
        }
        _DisplayedStars.RemoveAt(_DisplayedStars.Count - 1);
      }

      //storing the latest displayed value persistently
      DataPersistenceManager.Instance.DisplayedStars = count;
    }

    public void HackToTestStarWhileWatchingPlayWindow() {
      RobotEngineManager.Instance.RunDebugConsoleFuncMessage("DebugGiveStar", "");
    }

    #endregion //Misc Helper Methods
  }
}