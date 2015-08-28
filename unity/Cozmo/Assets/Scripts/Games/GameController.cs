using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki;
using Anki.Cozmo;

/// <summary>
/// this component acts as a simple state machine using a GameState enumeration to represent the common features of all cozmo minigames
///    specific games inherit from this class to enact their own unique features
/// </summary>
public class GameController : MonoBehaviour {

  #region NESTED DEFINITIONS

  //add states here only if they belong to ALL games
  public enum GameState {
    BUILDING,
    PRE_GAME,
    PLAYING,
    RESULTS,
    Count
  }

  //associate a time value in seconds with the voice-over audio clip describing it
  [Serializable]
  public struct TimerAudio {
    public int time;
    public AudioClip sound;
  }

  #endregion

  #region INSPECTOR FIELDS

  [SerializeField] protected int currentLevelOverride = -1;
  [SerializeField] protected Text countdownText = null;
  [SerializeField] protected float countdownToStart = 0f;
  [SerializeField] protected AudioClip countdownTickSound;
  [SerializeField] protected TimerAudio[] timerSounds;
  [SerializeField] protected AudioClip gameStartingIn;
  [SerializeField] protected AudioClip starPop;
  [SerializeField] protected Text textScore = null;
  [SerializeField] protected Text textError = null;
  [SerializeField] protected Text textState = null;
  [SerializeField] protected Text textTime = null;
  [SerializeField] protected Text textAddaboy = null;
  [SerializeField] protected bool autoPlay = false;
  [SerializeField] protected Button playButton = null;
  [SerializeField] protected Button playAgainButton = null;
  [SerializeField] protected Image resultsPanel = null;
  [SerializeField] protected AudioClip instructionsSound;
  [SerializeField] protected AudioClip gameStartSound;
  [SerializeField] protected AudioClip playerScoreSound;
  [SerializeField] protected AudioClip playingLoopSound;
  [SerializeField] protected GameActions[] stateActions = new GameActions[(int)GameState.Count];
  [SerializeField] protected AudioClip[] winSounds = new AudioClip[STAR_COUNT + 1];
  [SerializeField] protected AudioClip[] winLoopSounds = new AudioClip[STAR_COUNT + 1];
  [SerializeField] protected AudioClip loseSound;
  [SerializeField] protected AudioClip loseLoopSound;
  [SerializeField] protected Image[] starImages = new Image[STAR_COUNT];
  [SerializeField] protected float starPopInTime = .5f;
  [SerializeField] protected float maxStarPop = 1.5f;
  [SerializeField] protected float minStarPop = 0.3f;
  [SerializeField] protected bool robotBusyDuringResults = true;
  [SerializeField] protected float maxEmotionWaitTime = 20f;

  #endregion

  #region PUBLIC MEMBERS

  public const int MAX_PLAYERS = 4;
  //four plus cozmo?
  public const int STAR_COUNT = 3;

  public static float MessageDelay { get { return _MessageDelay; } protected set { _MessageDelay = value; } }

  public GameState state { get; private set; }

  public const string STARS_END = "_stars";

  public string SAVED_STARS { get { return currentGameName + currentLevelNumber + STARS_END; } }

  public int savedStars { get { return PlayerPrefs.GetInt(SAVED_STARS, 0); } set { PlayerPrefs.SetInt(SAVED_STARS, value); } }

  protected Animation playAgainSheen = null;

  #endregion

  #region PROTECTED MEMBERS

  protected AudioClip gameOverSound {
    get {
      if (win)
        return stars < winSounds.Length ? winSounds[stars] : null;
      return loseSound;
    }
  }

  protected AudioClip resultsLoopSound {
    get {
      if (win)
        return stars < winLoopSounds.Length ? winLoopSounds[stars] : null;
      return loseLoopSound;
    }
  }

  protected bool playRequested = false;
  protected bool buildRequested = false;

  protected GameState _state = GameState.BUILDING;

  protected float stateTimer = 0f;
  protected bool countdownAnnounced = false;
  protected float lastPlayTime = 0;
  protected int timerEventIndex = 0;
  protected float bonusTime = 0;
  // bonus time is awarded each time the player numDropsForBonusTime drop offs

  //supporting four human players plus cozmo for now
  protected List<int> scores = new List<int>();

  protected int score {
    get { return scores[0]; }
    set { scores[0] = value; }
  }

  protected int lastLeader = -1;

  protected int stars = 0;
  protected bool win = false;

  protected string currentGameName { get; private set; }

  protected int currentLevelNumber { get; private set; }

  protected bool firstFrame = true;

  protected float errorMsgTimer = 0f;

  protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  protected float gameStartingInDelay { get { return gameStartingIn != null ? gameStartingIn.length + 0.1f : 0f; } }

  protected float instructionsDelay { get { return instructionsSound != null ? instructionsSound.length + 0.5f : 0f; } }

  protected int lastTimerSeconds = 0;
  protected float coundownTimer = 0f;

  protected static float _MessageDelay = 0.5f;

  protected GameData.LevelData levelData { get; private set; }

  protected bool waitingForEmoteToFinish = false;

  protected Coroutine emotionWaitLimiterRoutine = null;

  #endregion

  #region MONOBEHAVIOR CALLBACKS

  protected virtual void Awake() {
    for (int i = 0; i < stateActions.Length; i++) {
      if (stateActions[i] == null)
        continue;
      stateActions[i].gameObject.SetActive(false);
    }

    while (scores.Count < MAX_PLAYERS)
      scores.Add(0);

    if (playAgainButton != null) {
      playAgainSheen = playAgainButton.GetComponent<Animation>();
    }
  }

  protected virtual void OnEnable() {
    state = GameState.BUILDING;
    stateTimer = 0f;
    errorMsgTimer = 0f;
    firstFrame = true;
    playRequested = false;
    buildRequested = false;

    currentGameName = PlayerPrefs.GetString("CurrentGame", "Unknown");
    currentLevelNumber = PlayerPrefs.GetInt(currentGameName + "_CurrentLevel", 1);

    if (currentLevelOverride > 0)
      currentLevelNumber = currentLevelOverride;

    if (GameData.instance != null) {
      if (GameData.instance.levelData.ContainsKey(currentGameName + currentLevelNumber)) {
        levelData = GameData.instance.levelData[currentGameName + currentLevelNumber];
      }
      else {
        Debug.LogError("GameData does not contain level: " + currentGameName + currentLevelNumber);
      }
    }
    else {
      Debug.LogError("GameData is not in scene");
    }

    if (textError != null)
      textError.gameObject.SetActive(false);
    if (playButton != null)
      playButton.gameObject.SetActive(false);
    if (resultsPanel != null)
      resultsPanel.gameObject.SetActive(false);
    if (countdownText != null)
      countdownText.gameObject.SetActive(false);
    if (textScore != null)
      textScore.gameObject.SetActive(false);
    if (robot != null)
      robot.ClearData();

    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.RobotCompletedAnimation += RobotCompletedAnimation;
      RobotEngineManager.instance.RobotCompletedCompoundAction += RobotCompletedCompoundAction;
    }
  }

  void Update() {
    stateTimer += Time.deltaTime;

    //force our initial state to enter in case there is set up code there
    if (firstFrame) {
      EnterState();
    }
    else {

      GameState nextState = GetNextState();

      if (nextState != state) {
        ExitState();
        state = nextState;
        EnterState();
      }
      else {
        UpdateState();
      }

    }

    RefreshGameActionsForState();

    RefreshHUD();
    firstFrame = false;
    playRequested = false;
    buildRequested = false;
  }

  protected virtual void OnDisable() {
    if (textError != null)
      textError.gameObject.SetActive(false);
    if (playButton != null)
      playButton.gameObject.SetActive(false);
    if (resultsPanel != null)
      resultsPanel.gameObject.SetActive(false);
    AudioManager.Stop(); // stop all sounds
    if (robot != null)
      robot.ClearData();

    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.RobotCompletedAnimation -= RobotCompletedAnimation;
      RobotEngineManager.instance.RobotCompletedCompoundAction -= RobotCompletedCompoundAction;
    }

    if (emotionWaitLimiterRoutine != null)
      StopCoroutine(emotionWaitLimiterRoutine);
    emotionWaitLimiterRoutine = null;
    waitingForEmoteToFinish = false;
  }

  #endregion

  #region PRIVATE METHODS

  GameState GetNextState() {

    //consider switching states
    switch (state) {
    case GameState.BUILDING:
      if ((playRequested || autoPlay) && IsGameReady())
        return GameState.PRE_GAME; 
      break;
    case GameState.PRE_GAME:
      if (IsPreGameCompleted())
        return GameState.PLAYING; 
      break;
    case GameState.PLAYING:
      if (buildRequested)
        return GameState.BUILDING;
      if (IsGameOver())
        return GameState.RESULTS;
      break;
    case GameState.RESULTS:
      if (buildRequested)
        return GameState.BUILDING;
      if (playRequested && IsGameReady())
        return GameState.PRE_GAME;
      break;
    }

    //stay in current state
    return state;
  }

  void EnterState() {
    stateTimer = 0f;

    switch (state) {
    case GameState.BUILDING:
      Enter_BUILDING();
      break;
    case GameState.PRE_GAME:
      Enter_PRE_GAME();
      break;
    case GameState.PLAYING:
      Enter_PLAYING();
      break;
    case GameState.RESULTS:
      Enter_RESULTS();
      break;
    }
  }

  void UpdateState() {
    switch (state) {
    case GameState.BUILDING:
      Update_BUILDING();
      break;
    case GameState.PRE_GAME:
      Update_PRE_GAME();
      break;
    case GameState.PLAYING:
      Update_PLAYING();
      break;
    case GameState.RESULTS:
      Update_RESULTS();
      break;
    }
  }

  void ExitState() {
    switch (state) {
    case GameState.BUILDING:
      Exit_BUILDING();
      break;
    case GameState.PRE_GAME:
      Exit_PRE_GAME();
      break;
    case GameState.PLAYING:
      Exit_PLAYING();
      break;
    case GameState.RESULTS:
      Exit_RESULTS();
      break;
    }
  }

  void RefreshGameActionsForState() {
    GameActions currentActions = stateActions[(int)state];
    
    for (int i = 0; i < stateActions.Length; i++) {
      if (stateActions[i] != null && currentActions != stateActions[i])
        stateActions[i].gameObject.SetActive(false);
    }
    
    if (currentActions != null)
      currentActions.gameObject.SetActive(true);
  }

  IEnumerator PopInStars() {
    int num_pops = 0;
    float lastPopTime = -1;
    
    for (int i = 0; i < starImages.Length; ++i) {
      if (starImages[i] != null)
        starImages[i].gameObject.SetActive(false);
    }
    
    
    while (num_pops < stars + 1) {
      if (lastPopTime + starPopInTime < Time.realtimeSinceStartup) {
        // time to pop in a star
        if (num_pops < starImages.Length) {
          starImages[num_pops].gameObject.SetActive(win && num_pops < stars);
          starImages[num_pops].transform.localScale = new Vector3(minStarPop, minStarPop, 0);
          if (win && num_pops < stars) {
            AudioManager.PlayOneShot(starPop);
          }
        }
        lastPopTime = Time.realtimeSinceStartup;
        num_pops++;
      }
      else {
        // lerp it in three phases:
        /// 1) from minStarPop to maxStarPop
        /// 2) from maxStarPop back to 1
        float pop_time = Time.realtimeSinceStartup - lastPopTime;
        float scale = 1;
        if (pop_time < starPopInTime / 2) {
          // first half
          scale = Mathf.Lerp(minStarPop, maxStarPop, pop_time / (starPopInTime / 2));
        }
        else {
          // second half
          scale = Mathf.Lerp(maxStarPop, 1, (pop_time - (starPopInTime / 2)) / (starPopInTime / 2));
        }
        starImages[num_pops - 1].transform.localScale = new Vector3(scale, scale, 0);
        
      }
      yield return 0;
    }
    
    yield return 0;
  }

  IEnumerator StopWaitingForEmotion(float delay) {
    yield return new WaitForSeconds(delay);
    
    waitingForEmoteToFinish = false;
  }

  void RobotCompletedAnimation(bool success, string animName) {
    Debug.Log("RobotCompletedAnimation(" + success + ", " + animName + ")");
    if (emotionWaitLimiterRoutine != null)
      StopCoroutine(emotionWaitLimiterRoutine);
    emotionWaitLimiterRoutine = null;

    waitingForEmoteToFinish = false;
  }

  void RobotCompletedCompoundAction(bool success, uint tagId) {
    Debug.Log("RobotCompletedAnimation(" + success + ", " + tagId + ")");
    if (emotionWaitLimiterRoutine != null)
      StopCoroutine(emotionWaitLimiterRoutine);
    emotionWaitLimiterRoutine = null;

    waitingForEmoteToFinish = false;
  }

  #endregion

  #region PROTECTED METHODS

  protected const string suffix_seconds = "s";

  protected virtual void RefreshHUD() {
    if (textScore != null) {
      textScore.text = score.ToString();
    }

    if (textState != null) {
      textState.text = state.ToString();
    }

    if (textTime != null && state == GameState.PLAYING) {
      if (levelData.maxPlayTime > 0f) {
        textTime.text = Mathf.CeilToInt(levelData.maxPlayTime - stateTimer).ToString() + suffix_seconds;
      }
      else {
        textTime.text = Mathf.CeilToInt(stateTimer).ToString() + suffix_seconds;
      }
    }
    else if (textTime != null) {
      textTime.text = string.Empty;
    }

    if (state != GameState.PLAYING && playRequested && !IsGameReady()) {
      if (textError != null) {
        //set specific text once we have analyzer
        textError.gameObject.SetActive(true);
      }
      errorMsgTimer = 5f;
    }
    
    if (errorMsgTimer > 0f) {
      errorMsgTimer -= Time.deltaTime;
      if (errorMsgTimer <= 0f) {
        if (textError != null) {
          textError.gameObject.SetActive(false);
        }
      }
      else {
        Color color = textError.color;
        color.a = Mathf.Lerp(0f, 1f, errorMsgTimer);
        textError.color = color;
      }
    }
  }

  protected virtual void Enter_BUILDING() {
    //Debug.Log(gameObject.name + " Enter_BUILDING");

    if (playButton != null)
      playButton.gameObject.SetActive(true);
    if (robot != null)
      robot.SetObjectAdditionAndDeletion(true, false);
  }

  protected virtual void Update_BUILDING() {
    //Debug.Log(gameObject.name + " Update_BUILDING");
  }

  protected virtual void Exit_BUILDING() {
    //Debug.Log(gameObject.name + " Exit_BUILDING");

    if (playButton != null)
      playButton.gameObject.SetActive(false);
    if (robot != null)
      robot.SetObjectAdditionAndDeletion(false, false);
  }

  protected virtual void Enter_PRE_GAME() {
    //Debug.Log(gameObject.name + " Enter_PRE_GAME");

    if (countdownText != null) {
      countdownText.gameObject.SetActive(false);
    }

    coundownTimer = 0f;
    countdownAnnounced = false;

    for (int i = 0; i < scores.Count; i++) {
      scores[i] = 0;
    }

    winners.Clear();

    //if(robot != null) robot.SetObjectAdditionAndDeletion(false, false);
  }

  protected virtual void Update_PRE_GAME() {
    //Debug.Log(gameObject.name + " Update_PRE_GAME");
    //add game specific intro messaging in an override of this method
    // for instance, if this game requires a certain action to start, tell them here: 'pick up X to begin!'
    //  or if this game has a count-down before start, we display that count down here
    if (countdownToStart > 0f) {

      if (coundownTimer == 0f) {
        if (instructionsSound != null)
          AudioManager.PlayOneShot(instructionsSound);
        //Debug.Log("gameStartingIn stateTimer("+stateTimer+")");
        if (gameStartingIn != null)
          AudioManager.PlayAudioClip(gameStartingIn, instructionsDelay, AudioManager.Source.Notification);
        lastTimerSeconds = 0;
        countdownAnnounced = true;
      }

      coundownTimer += Time.deltaTime;

      if (coundownTimer >= gameStartingInDelay + instructionsDelay) {

        int remaining = Mathf.CeilToInt(Mathf.Clamp((countdownToStart + gameStartingInDelay + instructionsDelay) - coundownTimer, 0f, countdownToStart));

        //Debug.Log("PlayCountdownAudio stateTimer("+stateTimer+")");
        PlayCountdownAudio(remaining);
        
        if (countdownText != null) {
          countdownText.text = remaining.ToString();
          countdownText.gameObject.SetActive(true);
        }
      }
    }
  }

  protected virtual void Exit_PRE_GAME() {
    //Debug.Log(gameObject.name + " Exit_PRE_GAME");
    if (countdownText != null) {
      countdownText.gameObject.SetActive(false);
    }
  }

  protected virtual void Enter_PLAYING() {

    timerEventIndex = 0;
    bonusTime = 0;

    //Debug.Log(gameObject.name + " Enter_PLAYING");
    if (gameStartSound != null)
      AudioManager.PlayOneShot(gameStartSound);

    if (textScore != null)
      textScore.gameObject.SetActive(true);
    if (textError != null)
      textError.gameObject.SetActive(false);

    score = 0;
  }

  protected virtual void Update_PLAYING() {
    //Debug.Log(gameObject.name + " Update_PLAYING");
  }

  protected virtual void Exit_PLAYING(bool overrideStars = false) {
    win = false;
    
    if (levelData.scoreToWin > 0) {
      win = score >= levelData.scoreToWin;
    }
    else {
      win = true;
    }

    if (!overrideStars) {
      stars = 0;
      int[] starRequirements = levelData.stars;
      for (int i = 0; i < starRequirements.Length; ++i) {
        if (score >= starRequirements[i] && starRequirements[i] > 0)
          stars = i + 1;
      }

      if (stars >= savedStars)
        savedStars = stars;
    }

    //Debug.Log(gameObject.name + " Exit_PLAYING");
    if (textScore != null)
      textScore.gameObject.SetActive(false);

    if (gameOverSound != null)
      AudioManager.PlayOneShot(gameOverSound);
  }

  protected virtual void Enter_RESULTS() {

    //Debug.Log(gameObject.name + " Enter_RESULTS");
    StartCoroutine(PopInStars());
    if (resultsPanel != null)
      resultsPanel.gameObject.SetActive(true);
    if (textScore != null)
      textScore.gameObject.SetActive(true);
    if (resultsLoopSound != null)
      AudioManager.PlayAudioClipLooping(resultsLoopSound, gameOverSound != null ? gameOverSound.length + 0.5f : 0.5f, AudioManager.Source.Gameplay);
    if (textAddaboy != null) {
      switch (stars) {
      case 0: 
        textAddaboy.text = "Better luck next time!";
        break;
      case 1: 
        textAddaboy.text = "Not bad!";
        break;
      case 2: 
        textAddaboy.text = "Good job!";
        break;
      case 3: 
        textAddaboy.text = "Fantastic!";
        break;
      }
    }

    if (robotBusyDuringResults && robot != null) {
      robot.isBusy = true;
    }

    if (playAgainButton != null) {
      playAgainButton.interactable = IsGameReady();
    }

    if (playAgainSheen != null) {
      playAgainSheen.enabled = IsGameReady();
    }
  }

  protected virtual void Update_RESULTS() {
    //Debug.Log(gameObject.name + " Update_RESULTS");

    if (playAgainButton != null) {
      playAgainButton.interactable = IsGameReady();
    }

    if (playAgainSheen != null) {
      playAgainSheen.enabled = IsGameReady();
    }
  }

  protected virtual void Exit_RESULTS() {
    //Debug.Log(gameObject.name + " Exit_RESULTS");

    if (resultsPanel != null)
      resultsPanel.gameObject.SetActive(false);
    if (textScore != null)
      textScore.gameObject.SetActive(false);
    AudioManager.Stop(resultsLoopSound);
    if (robot != null) {
      robot.TurnOffAllLights();
      if (robotBusyDuringResults)
        robot.isBusy = false;
    }
  }

  protected virtual bool IsGameReady() {
    if (robot == null || robot.isBusy)
      return false;
    if (GameLayoutTracker.instance == null)
      return false;

    //layout tracker dominates unless it is disabled
    return GameLayoutTracker.instance.Phase == LayoutTrackerPhase.DISABLED;
  }

  protected virtual bool IsPreGameCompleted() {
    if (robot == null)
      return false;
    if (GameLayoutTracker.instance == null)
      return false;

    //add game specific gating in an override of this method
    // for instance, if this game requires a certain action to start: 'pick up X to begin!'
    //  or if this game has a count-down before start, handle that check here
    if (countdownToStart > 0f && ((countdownToStart + gameStartingInDelay + instructionsDelay) - coundownTimer) > 0f)
      return false;

    return true;
  }

  protected List<int> winners = new List<int>();

  protected virtual bool IsGameOver() {
    winners.Clear();

    if (levelData.scoreToWin > 0) {
      for (int i = 0; i < scores.Count; i++) {
        if (scores[i] >= levelData.scoreToWin) {
          winners.Add(i);
        }
      }
    }

    if (winners.Count > 0)
      return true;

    if (levelData.maxPlayTime > 0f && stateTimer >= levelData.maxPlayTime)
      return true;
    
    return false;
  }

  protected void PlayCountdownAudio(int secondsLeft) {
    bool played = false;
    for (int i = 0; i < timerSounds.Length; i++) {
      if (secondsLeft == timerSounds[i].time && lastTimerSeconds != timerSounds[i].time) {
        AudioManager.PlayAudioClipDeferred(timerSounds[i].sound, 0, AudioManager.Source.Notification);
        played = true;
        break;
      }
    }
    
    if (!played && lastTimerSeconds != secondsLeft && countdownTickSound != null)
      AudioManager.PlayOneShot(countdownTickSound);
    
    lastTimerSeconds = secondsLeft;
  }

  protected void SetRobotEmotion(string animName, bool wait = true, bool stopPriorAnim = true) {
    if (robot == null)
      return;

    //Debug.Log("SetRobotEmotion("+animName+")");
    CozmoEmotionManager.SetEmotion(animName, stopPriorAnim);

    if (emotionWaitLimiterRoutine != null)
      StopCoroutine(emotionWaitLimiterRoutine);
    waitingForEmoteToFinish = wait;

    if (!wait)
      return;

    emotionWaitLimiterRoutine = StartCoroutine(StopWaitingForEmotion(maxEmotionWaitTime));
  }


  #endregion

  #region PUBLIC METHODS

  public void PlayRequested() {
    //Debug.Log ("PlayRequested");
    playRequested = true;
  }

  public void BuildRequested() {
    //Debug.Log ("BuildRequested");
    buildRequested = true;
    if (GameLayoutTracker.instance != null) {
      GameLayoutTracker.instance.StartBuild();
    }
  }

  #endregion

}
