using UnityEngine;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.Cozmo;
using System;
using DigitalRuby.ThunderAndLightning;

/// <summary>
/// game controller for manageing cozmo's vortex minigame
///    wraps and manages a list of SpinWheels
/// </summary>
public class VortexController : GameController {

  #region NESTED DEFINITIONS

  public enum VortexState {
    INTRO,
    REQUEST_SPIN,
    SPINNING,
    SPIN_COMPLETE
  }

  class VortexInput {
    public List<float> stamps = new List<float>();

    public float FinalTime {
      get {
        return stamps.Count == 0 ? -1f : stamps[stamps.Count - 1];
      }
    }

    public float FirstTime {
      get {
        return stamps.Count == 0 ? Time.time : stamps[0];
      }
    }
  }

  struct ScoreBoardData {
    public int playerIndex;
    public Color color;
    public int score;
    public int place;
    //distinct from ordering to handle ties
  }

  [Serializable]
  public class VortexSettings {
    public int rings = 1;
    public int[] slicesPerRing = { 16 };
    public int[] maxSurvivePerRound = { 4 };
    public int roundsPerRing = 5;
    public bool inward = true;
    public bool showTokens = true;
    public bool elimination = false;
    public bool winnerEliminated = false;
    public int pointsFirstPlace = 30;
    public int pointsSecondPlace = 20;
    public int pointsThirdPlace = 10;
    public int pointsFourthPlace = 5;
    public int pointsIncorrectPenalty = -5;
  }

  #endregion

  #region INSPECTOR FIELDS

  [SerializeField] List<SpinWheel> wheels;
  [SerializeField] Text textPlayState;
  [SerializeField] Image imageHub;
  [SerializeField] Text textCurrentNumber;
  [SerializeField] Text textRoundNumber;
  [SerializeField] Text[] textPlayerCurrentNumbers;
  // only use for single ring matches

  [SerializeField] Text[] textfinalPlayerScores;
  [SerializeField] Image[] imageFinalPlayerScoreBGs;

  [SerializeField] AudioClip numberChangedSound;
  [SerializeField] AudioClip buttonPressSound;
  [SerializeField] AudioClip roundCompleteWinner;
  [SerializeField] AudioClip roundCompleteNoWinner;

  [SerializeField] AudioClip timeUp;
  [SerializeField] AudioClip newHighScore;
  [SerializeField] AudioClip spinRequestSound;
  [SerializeField] AudioClip finalRound;

  [SerializeField] RectTransform[] playerPanels;
  [SerializeField] RectTransform[] playerTokens;
  [SerializeField] Button[] playerButtons;
  [SerializeField] LayoutBlock2d[] playerMockBlocks;
  [SerializeField] Image[] playerPanelFills;
  [SerializeField] Image[] imageInputLocked;
  [SerializeField] Image[] imagePlayerBidBGs;
  [SerializeField] Text[] textPlayerBids;
  [SerializeField] RectTransform[] textPlayerSpinNow;
  [SerializeField] RectTransform preGameAlert;
  [SerializeField] Animation[] playerBidFlashAnimations;
  [SerializeField] Text[] textPlayerScores;
  [SerializeField] Text[] textPlayerScoreDeltas;

  [SerializeField] VortexSettings[] settingsPerLevel;

  [SerializeField] LightningBoltShapeSphereScript lightingBall;
  [SerializeField] float[] wheelLightningRadii = { 3.5f, 2f, 1f };
  [SerializeField] int lightningMinCountAtSpeedMax = 8;
  [SerializeField] int lightningMaxCountAtSpeedMax = 16;
  [SerializeField] Image imageGear;
  [SerializeField] Image imageResultsGear;
  [SerializeField] Color gearDefaultColor = Color.white;
  [SerializeField] Color[] playerColors = { Color.blue, Color.green, Color.yellow, Color.red };

  [SerializeField] float maxPlayerInputTime = 3f;

  [SerializeField] float scoreDisplayFillFade = 0.5f;
  [SerializeField] float scoreDisplayFillAlpha = 0.5f;
  [SerializeField] float scoreDisplayEmptyAlpha = 0.1f;

  [SerializeField] Image imageCozmoDragHand;
  [SerializeField] Vector2 cozmoStartDragPos = Vector2.zero;
  [SerializeField] Vector2 cozmoEndDragPos = Vector2.zero;
  [SerializeField] float cozmoDragDelay = 2f;
  [SerializeField] float cozmoDragTime = 2f;
  [SerializeField] float cozmoFinalDragTime = 0.2f;
  
  [SerializeField] float scoreScaleBase = 0.75f;
  [SerializeField] float scoreScaleMax = 1f;
  [SerializeField] float cozmoPredictiveAccuracy = .75f;
  [SerializeField] float cozmoEarlyGuess = .1f;
  [SerializeField] float cozmoEarlyGuessTime = 3f;
  [SerializeField] float cozmoTimeFirstTap = 1.5f;
  [SerializeField] float cozmoTimePerTap = 1.25f;
  [SerializeField] float cozmoPredicitveLeadTime = 5f;
  [SerializeField] float cozmoExpectationDelay = 1f;
  public CozmoUtil.RobotPose[] robotPoses = new CozmoUtil.RobotPose[3];

  #endregion

  #region PRIVATE METHODS

  int currentWheelIndex {
    get {
      if (rings == 1)
        return 0;
      
      int index = 0;
      if (settings.inward) {
        index = Mathf.CeilToInt(round / roundsPerRing) - 1;
      }
      else {
        index = rings - Mathf.CeilToInt(round / roundsPerRing);
      }
      
      index = Mathf.Clamp(index, 0, wheels.Count - 1);
      return index;
    }
  }

  SpinWheel wheel { 
    get {
      return wheels[currentWheelIndex];
    }
  }

  RectTransform cozmoDragHandRectTransform;
  Vector2 startDragPos;
  Vector2 endDragPos;
  float dragTimer = 0f;
  int dragCount = 0;
  int dragCountMax = 2;
  int lightIndex = 0;

  List<VortexInput> playerInputs = new List<VortexInput>();

  CanvasGroup[] playerButtonCanvasGroups;

  int numPlayers = 4;
  int currentPlayerIndex = 0;
  int round = 0;

  VortexState playState = VortexState.INTRO;
  bool[] playersEliminated = { false, false, false, false };

  VortexSettings settings;

  List<ActiveBlock> playerInputBlocks = new List<ActiveBlock>();

  List<ScoreBoardData> sortedScoreData = new List<ScoreBoardData>();
  
  int rings = 1;
  int roundsPerRing = 5;
  int[] slicesPerRing;
  int lastNumber = 0;
  
  float playStateTimer = 0f;
  
  int cozmoTapsSubmitted = 0;
  int predictedNum = -1;
  float predictedDuration = -1f;
  float predictedTimeAfterLastPeg = -1f;
  float predictedTimeMovingBackwards = 0f;
  bool predictedEarly = false;

  List<int> playersThatAreCorrect = new List<int>();
  List<int> playersThatAreWrong = new List<int>();

  public enum CozmoExpectation {
    NONE,
    EXPECT_WIN_RIGHT,
    EXPECT_WIN_WRONG,
    KNOWS_WRONG
  }

  CozmoExpectation cozmoExpectation = CozmoExpectation.NONE;

  ObservedObject humanHead;

  float frequency = 0.1f;
  float headTimer = 0f;
  float turnStartAngle = 0f;

  float fadeTimer = 1f;
  int resultsDisplayIndex = 0;

  List<int> scoreDeltas = new List<int>();

  bool fakeCozmoTaps = true;
  int cozmoIndex = 1;
  bool atYourMark = false;
  float markx_mm = 0;
  float marky_mm = 50;
  float mark_rad = (3.0f * Mathf.PI) / 2.0f;

  void RobotCompletedAnimation(bool success, string animName) {
    switch (animName) {
    case "firstTap":
    case "tapTwoTimes":
    case "tapThreeTimes":
    case "tapFourTimes":
      DAS.Debug("Vortex", "Animation ended: " + animName);
      StartCoroutine(CozmoReaction());
      break;
    default:
      break;
    }
  }

  #endregion

  #region PROTECTED METHODS

  protected override void Awake() {
    base.Awake();

    playerButtonCanvasGroups = new CanvasGroup[playerButtons.Length];
    for (int i = 0; i < playerButtons.Length; i++) {
      playerButtonCanvasGroups[i] = playerButtons[i].gameObject.GetComponent<CanvasGroup>();
    }
  }

  protected override void OnEnable() {
    base.OnEnable();
    
    cozmoIndex = (PlayerPrefs.GetInt("VortexWithoutCozmo", 0) == 1) ? -1 : 1;

    numPlayers = PlayerPrefs.GetInt("NumberOfPlayers", 1);
    settings = settingsPerLevel[currentLevelNumber - 1];

    rings = settings.rings;
    roundsPerRing = settings.roundsPerRing;
    slicesPerRing = settings.slicesPerRing;

    for (int i = 0; i < playerPanels.Length; i++) {
      playerPanels[i].gameObject.SetActive(i < numPlayers);
    }

    for (int i = 0; i < slicesPerRing.Length && i < wheels.Count; i++) {
      wheels[i].SetNumSlices(slicesPerRing[i]);
    }

    for (int i = 0; i < wheels.Count; i++) {
      wheels[i].ResetWheel();
      wheels[i].Lock();
      wheels[i].Unfocus();
      wheels[i].gameObject.SetActive(i < rings);
    }
    
    PlaceTokens();

    lightingBall.CountRange = new RangeOfIntegers { Minimum = 0, Maximum = 0 };

    MessageDelay = .1f;

    for (int i = 0; i < playerButtonCanvasGroups.Length; i++) {
      playerButtonCanvasGroups[i].interactable = false;
      playerButtonCanvasGroups[i].blocksRaycasts = false;
    }

    for (int i = 0; i < textPlayerScores.Length; i++) {
      textPlayerScores[i].text = "SCORE: 0";
    }
    
    for (int i = 0; i < playerPanelFills.Length; i++) {
      Color col = playerPanelFills[i].color;
      col.a = 0f;
      playerPanelFills[i].color = col;
    }

    for (int i = 0; i < textPlayerBids.Length; i++) {
      textPlayerBids[i].text = "";
    }

    for (int i = 0; i < imagePlayerBidBGs.Length; i++) {
      imagePlayerBidBGs[i].gameObject.SetActive(false);
    }
    

    foreach (Image image in imageInputLocked)
      image.gameObject.SetActive(false);

    for (int i = 0; i < textPlayerSpinNow.Length; i++) {
      textPlayerSpinNow[i].gameObject.SetActive(false);
    }

    if (imageCozmoDragHand != null) {
      cozmoDragHandRectTransform = imageCozmoDragHand.transform.parent as RectTransform;
      imageCozmoDragHand.gameObject.SetActive(false);
    }

    if (imageHub != null) {
      imageHub.gameObject.SetActive(false);
    }

    if (preGameAlert != null)
      preGameAlert.gameObject.SetActive(false);

    ActiveBlock.TappedAction += BlockTapped;

    foreach (Text text in textPlayerScoreDeltas)
      text.gameObject.SetActive(false);
    
    imageGear.color = gearDefaultColor;

    while (playerInputs.Count < numPlayers)
      playerInputs.Add(new VortexInput());

    playAgainButton.gameObject.SetActive(false);
  }

  protected override void OnDisable() {
    base.OnDisable();

    ActiveBlock.TappedAction -= BlockTapped;

    //revert to single player incase this ever matters to other games
    PlayerPrefs.SetInt("NumberOfPlayers", 1);
    PlayerPrefs.SetInt("VortexWithoutCozmo", 0);
    
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
    if (GameLayoutTracker.instance != null)
      GameLayoutTracker.instance.CubeSpotted += CubeSpotted;
  }

  void CubeSpotted() {
    DAS.Debug("Vortex", "Setting robot position");
    SetRobotStartingPositions();
    if (GameLayoutTracker.instance != null)
      GameLayoutTracker.instance.CubeSpotted -= CubeSpotted;
  }

  protected override void Exit_BUILDING() {
    base.Exit_BUILDING();
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();
  }

  protected override void Enter_PRE_GAME() {

    if (playerMockBlocks != null) {
      for (int i = 0; i < numPlayers; i++) {
        playerMockBlocks[i].Validate(false);
        playerMockBlocks[i].Initialize(CubeType.LIGHT_CUBE);
        playerMockBlocks[i].SetLights(GetPlayerUIColor(i));
      }
    }

    for (int i = 0; i < playerButtonCanvasGroups.Length; i++) {
      if (i == cozmoIndex)
        continue; // player 2 is cozmo, no button
      playerButtonCanvasGroups[i].interactable = i < numPlayers;
      playerButtonCanvasGroups[i].blocksRaycasts = i < numPlayers;
    }

    if (preGameAlert != null)
      preGameAlert.gameObject.SetActive(true);

    ClearInputs();

    base.Enter_PRE_GAME();

    playerInputBlocks.Clear();
    
    humanHead = null;
    
    if (robot != null && PlayerPrefs.GetInt("DebugSkipLayoutTracker", 0) == 0) {

      for (int i = 0; i < robot.knownObjects.Count; i++) {
        if (!robot.knownObjects[i].isActive)
          continue;

        ActiveBlock block = robot.knownObjects[i] as ActiveBlock;

        block.SetMode(GetPlayerActiveCubeColorMode(playerInputBlocks.Count));

        playerInputBlocks.Add(block);
      }

      for (int i = 0; i < robot.knownObjects.Count; i++) {
        if (!robot.knownObjects[i].isFace)
          continue;
        humanHead = robot.knownObjects[i];
        break;
      }
      
      if (humanHead == null) {
        float height = 0.75f;
        robot.SetHeadAngle(height);
      }
      
    }

    headTimer = 0.1f;

    imageGear.color = gearDefaultColor;

    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.SuccessOrFailure += CheckForGotoStartCompletion;
      atYourMark = false;
    }
  }

  protected override void Update_PRE_GAME() {
    base.Update_PRE_GAME();

    #if UNITY_EDITOR
    if (Input.GetKeyDown(KeyCode.L)) {
      CozmoEmotionManager.SetEmotion("TAP_FOUR", true);
      StartCoroutine(SetCozmoTaps(4));
    }
    #endif

    if (cozmoIndex >= 0 && robot != null) {

      if (robot.carryingObject == null && !robot.isBusy) {
        robot.PickAndPlaceObject(playerInputBlocks[cozmoIndex]);
        if (CozmoBusyPanel.instance != null) {
          string desc = "Cozmo is attempting to pick-up\n a game cube.";
          CozmoBusyPanel.instance.SetDescription(desc);
        }
      }
      else if (!robot.isBusy && !playerMockBlocks[cozmoIndex].Validated) {
        //robot.TapBlockOnGround(1);
        //robot.isBusy = true;
        CozmoEmotionManager.instance.SetEmotionReturnToPose("TAP_ONE", markx_mm, marky_mm, mark_rad, true, true);

      }
      else if (humanHead == null) {
        
        for (int i = 0; i < robot.knownObjects.Count; i++) {
          if (!robot.knownObjects[i].isFace)
            continue;
          humanHead = robot.knownObjects[i];
          break;
        }
      }
      
    }
  }

  protected override void Exit_PRE_GAME() {
    base.Exit_PRE_GAME();

    if (preGameAlert != null)
      preGameAlert.gameObject.SetActive(false);

    for (int i = 0; i < playerButtonCanvasGroups.Length; i++) {
      playerButtonCanvasGroups[i].interactable = false;
      playerButtonCanvasGroups[i].blocksRaycasts = false;
    }
  }

  protected override void Enter_PLAYING() {
    for (int i = 0; i < textPlayerScores.Length; i++) {
      textPlayerScores[i].text = "SCORE: 0";
    }

    ClearInputs();
    playState = VortexState.INTRO;
    EnterPlayState();

    if (imageHub != null) {
      imageHub.gameObject.SetActive(true);
    }

    if (robot != null) {
      //turn back towards ipad
      robot.SetLiftHeight(1f);
      //robot.TrackToObject(null, false);
    }

    base.Enter_PLAYING();

    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.RobotCompletedAnimation += RobotCompletedAnimation;
    }

  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();

    VortexState nextPlayState = GetNextPlayState();

    if (playState != nextPlayState) {
      ExitPlayState();
      playState = nextPlayState;
      EnterPlayState();
    }
    else {
      UpdatePlayState();
    }

    if (robot != null) {
      if (humanHead == null) {
        
        for (int i = 0; i < robot.knownObjects.Count; i++) {
          if (!robot.knownObjects[i].isFace)
            continue;
          humanHead = robot.knownObjects[i];
          break;
        }
      }
    }


  }

  protected override void Exit_PLAYING(bool overrideStars = false) {
    base.Exit_PLAYING();

    ExitPlayState();

    if (imageHub != null) {
      imageHub.gameObject.SetActive(false);
    }

    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.RobotCompletedAnimation -= RobotCompletedAnimation;
    }

  }

  protected override void Enter_RESULTS() {
    base.Enter_RESULTS();

    //wheel.gameObject.SetActive(false);

    playAgainButton.gameObject.SetActive(true);

    sortedScoreData.Clear();

    for (int i = 0; i < scores.Count && i < numPlayers; i++) {
      int score = scores[i];

      int insertIndex = sortedScoreData.Count;

      for (int j = 0; j < sortedScoreData.Count; j++) {
        if (score < sortedScoreData[j].score)
          continue;
        insertIndex = j;
        break;
      }

      ScoreBoardData scoreData = new ScoreBoardData();
      scoreData.score = score;
      scoreData.playerIndex = i;
      scoreData.color = GetPlayerUIColor(i);

      if (insertIndex < sortedScoreData.Count) {
        sortedScoreData.Insert(insertIndex, scoreData);
      }
      else {
        sortedScoreData.Add(scoreData);
      }
      
    }

    List<Color> gearColors = new List<Color>();

    int placeCounter = 0;
    int lastScore = sortedScoreData[0].score;
    for (int i = 0; i < sortedScoreData.Count; i++) {
      if (sortedScoreData[i].score != lastScore)
        placeCounter++;

      if (placeCounter == 0) {
        gearColors.Add(sortedScoreData[i].color);
      } 

      ScoreBoardData data = sortedScoreData[i];
      data.place = placeCounter;
      lastScore = data.score;
      sortedScoreData[i] = data;
    }
    
    if (imageResultsGear != null) {
  
      float r = 0f;
      float g = 0f;
      float b = 0f;
      float a = 0f;
  
      for (int i = 0; i < gearColors.Count; i++) {
        r += gearColors[i].r;
        g += gearColors[i].g;
        b += gearColors[i].b;
        a += gearColors[i].a;
      }
  
      
      r /= gearColors.Count;
      g /= gearColors.Count;
      b /= gearColors.Count;
      a /= gearColors.Count;
      
      imageResultsGear.color = new Color(r, g, b, a);
    }

    for (int i = 0; i < textfinalPlayerScores.Length && i < imageFinalPlayerScoreBGs.Length; i++) {

      if (i >= sortedScoreData.Count) {
        textfinalPlayerScores[i].gameObject.SetActive(false);
        imageFinalPlayerScoreBGs[i].gameObject.SetActive(false);
        continue;
      }

      string scoreText = "";

      switch (sortedScoreData[i].place) {
      case 0:
        scoreText += "1st place: ";
        break;
      case 1:
        scoreText += "2nd place: ";
        break;
      case 2:
        scoreText += "3rd place: ";
        break;
      case 3:
        scoreText += "4th place: ";
        break;
      }

      scoreText += sortedScoreData[i].score;

      textfinalPlayerScores[i].text = scoreText;
      textfinalPlayerScores[i].color = sortedScoreData[i].color;
      imageFinalPlayerScoreBGs[i].color = sortedScoreData[i].color;
      textfinalPlayerScores[i].gameObject.SetActive(true);
      imageFinalPlayerScoreBGs[i].gameObject.SetActive(true);
    }
    
    if (robot != null) {
      if (humanHead == null) {
        
        for (int i = 0; i < robot.knownObjects.Count; i++) {
          if (!robot.knownObjects[i].isFace)
            continue;
          humanHead = robot.knownObjects[i];
          break;
        }
      }
      DAS.Debug("Vortex", "Enter_RESULTS robot.SetLiftHeight(0.1f);");
    }

    if (sortedScoreData[0].playerIndex == cozmoIndex) {
      // cozmo won
      SetRobotEmotion("WIN_MATCH");
    }
    else {
      // cozmo lost
      SetRobotEmotion("LOSE_MATCH");
    }

    for (int i = 0; i < playerMockBlocks.Length; i++) {
      playerMockBlocks[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < textPlayerScores.Length; i++) {
      textPlayerScores[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < imagePlayerBidBGs.Length; i++) {
      imagePlayerBidBGs[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < textPlayerBids.Length; i++) {
      textPlayerBids[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < playerPanelFills.Length; i++) {
      Color col = playerPanelFills[i].color;
      col.a = 0.25f;
      playerPanelFills[i].color = col;
    }
  }

  protected override void Update_RESULTS() {
    base.Update_RESULTS();

    
    if (robot != null) {
      if (humanHead == null) {
        
        for (int i = 0; i < robot.knownObjects.Count; i++) {
          if (!robot.knownObjects[i].isFace)
            continue;
          humanHead = robot.knownObjects[i];
          break;
        }
      }

      if (!robot.isBusy && humanHead != null && robot.headTrackingObjectID < 0) {
        robot.TrackToObject(humanHead, false);
      }
    }
  }

  protected override void Exit_RESULTS() {
    base.Exit_RESULTS();
    
    for (int i = 0; i < textPlayerScores.Length; i++) {
      textPlayerScores[i].text = "SCORE: 0";
    }
    
    for (int i = 0; i < playerMockBlocks.Length; i++) {
      playerMockBlocks[i].gameObject.SetActive(true);
    }
    
    for (int i = 0; i < textPlayerScores.Length; i++) {
      textPlayerScores[i].gameObject.SetActive(true);
    }
    
    for (int i = 0; i < playerPanelFills.Length; i++) {
      Color col = playerPanelFills[i].color;
      col.a = 0f;
      playerPanelFills[i].color = col;
    }
    
    for (int i = 0; i < imagePlayerBidBGs.Length; i++) {
      imagePlayerBidBGs[i].gameObject.SetActive(true);
    }
    
    for (int i = 0; i < textPlayerBids.Length; i++) {
      textPlayerBids[i].gameObject.SetActive(true);
    }

    playAgainButton.gameObject.SetActive(false);

  }

  protected override bool IsPreGameCompleted() {
    if (robot != null && PlayerPrefs.GetInt("DebugSkipLayoutTracker", 0) == 0) {
      
      if (cozmoIndex >= 0 && (robot.carryingObject == null || !robot.carryingObject.isActive || robot.isBusy || !atYourMark))
        return false;
      
      for (int i = 0; i < numPlayers; i++) {
        if (!playerMockBlocks[i].Validated)
          return false;
      }

      if (!base.IsPreGameCompleted())
        return false;
    }
    return true;
  }

  protected override bool IsGameReady() {
    if (robot != null && PlayerPrefs.GetInt("DebugSkipLayoutTracker", 0) == 0) {

      if (robot.activeBlocks.Count < numPlayers)
        return false;

      if (!base.IsGameReady())
        return false;
    }

    return true;
  }

  protected override bool IsGameOver() {
    //if(base.IsGameOver()) return true;

    if (round > (roundsPerRing * rings))
      return true;

    for (int i = 0; i < playersEliminated.Length; i++) {
      if (!playersEliminated[i])
        return false;
    }

    return true;
  }

  protected override void RefreshHUD() {
    base.RefreshHUD();

    textRoundNumber.text = "ROUND " + Mathf.Max(1, round).ToString();

    int newNumber = wheel.GetDisplayedNumber();
    textCurrentNumber.text = newNumber.ToString();
    textPlayState.text = state == GameState.PLAYING ? playState.ToString() : "";
  }

  #endregion

  #region PRIVATE METHODS

  VortexState GetNextPlayState() {

    switch (playState) {
    case VortexState.INTRO:
      if (waitingForEmoteToFinish)
        return playState;
      return VortexState.REQUEST_SPIN;
    case VortexState.REQUEST_SPIN:
      if (wheel.Spinning) {
        return VortexState.SPINNING;
      }
      break;
    case VortexState.SPINNING:
      if (wheel.Finished) {
        return VortexState.SPIN_COMPLETE;
      }
      break;
    case VortexState.SPIN_COMPLETE:
      if (waitingForEmoteToFinish)
        return playState;
      if (!IsGameOver() && playStateTimer > 2f + (playersThatAreCorrect.Count * scoreDisplayFillFade * 2f))
        return VortexState.REQUEST_SPIN;
      break;
    }

    return playState;
  }

  void EnterPlayState() {
    playStateTimer = 0f;
    switch (playState) {
    case VortexState.INTRO:
      Enter_INTRO();
      break;
    case VortexState.REQUEST_SPIN:
      Enter_REQUEST_SPIN();
      break;
    case VortexState.SPINNING:
      Enter_SPINNING();
      break;
    case VortexState.SPIN_COMPLETE:
      Enter_SPIN_COMPLETE();
      break;
    }
  }

  void UpdatePlayState() {
    playStateTimer += Time.deltaTime;
    switch (playState) {
    case VortexState.INTRO:
      Update_INTRO();
      break;
    case VortexState.REQUEST_SPIN:
      Update_REQUEST_SPIN();
      break;
    case VortexState.SPINNING:
      Update_SPINNING();
      break;
    case VortexState.SPIN_COMPLETE:
      Update_SPIN_COMPLETE();
      break;
    }
  }

  void ExitPlayState() {
    switch (playState) {
    case VortexState.INTRO:
      Exit_INTRO();
      break;
    case VortexState.REQUEST_SPIN:
      Exit_REQUEST_SPIN();
      break;
    case VortexState.SPINNING:
      Exit_SPINNING();
      break;
    case VortexState.SPIN_COMPLETE:
      Exit_SPIN_COMPLETE();
      break;
    }
  }

  void Enter_INTRO() {
    currentPlayerIndex = UnityEngine.Random.Range(0, numPlayers);
    round = 0;

    for (int i = 0; i < playersEliminated.Length; i++) {
      playersEliminated[i] = false;
    }

    PlaceTokens();

    //enable intro text
  }

  void Update_INTRO() {
    
  }

  void Exit_INTRO() {
    //disable intro text
  }

  void Enter_REQUEST_SPIN() {

    currentPlayerIndex = IncrementPlayerTurn(currentPlayerIndex);

    round++;

    if (round == (roundsPerRing * rings)) {
      AudioManager.PlayAudioClip(finalRound, 0, AudioManager.Source.Gameplay);
      DAS.Debug("Vortex", "final round");
    }

    for (int i = 0; i < wheels.Count; i++) {

      if (wheel == wheels[i])
        continue;

      //wheels[i].ResetWheel();
      wheels[i].Lock();
      wheels[i].Unfocus();
    
      bool show = rings > i;

      if (settings.inward) {
        show &= round <= (i + 1) * roundsPerRing;
      }
      else {
        show &= round <= (wheels.Count - i) * roundsPerRing;
      }

      wheels[i].gameObject.SetActive(show);
    }

    PlaceTokens();

    if (currentPlayerIndex == cozmoIndex) {
      wheel.AutomatedMode();
      CozmoEmotionManager.instance.SetEmotionTurnInPlace("SPIN_WHEEL", mark_rad, true);

      startDragPos = new Vector2(cozmoStartDragPos.x * Screen.width, cozmoStartDragPos.y * Screen.height);
      startDragPos += UnityEngine.Random.insideUnitCircle * Screen.height * 0.1f;

      endDragPos = new Vector2(cozmoEndDragPos.x * Screen.width, cozmoEndDragPos.y * Screen.height);
      endDragPos += UnityEngine.Random.insideUnitCircle * Screen.height * 0.1f;
      dragTimer = cozmoDragTime;
      dragCount = 0;
      dragCountMax = UnityEngine.Random.Range(2, 4);
      wheel.DragStart(startDragPos, Time.time);
    }
    else {
      wheel.Unlock();
      CozmoEmotionManager.instance.SetEmotionTurnInPlace("YOUR_TURN", GetPoseFromPlayerIndex(currentPlayerIndex).rad, true, true, true);
      // setting the head angle to ~35 degrees
      robot.SetHeadAngle(.61f, true);
    }

    wheel.Focus();
    wheel.gameObject.SetActive(true);

    lightingBall.Radius = wheelLightningRadii[currentWheelIndex];

    if (spinRequestSound != null)
      AudioManager.PlayOneShot(spinRequestSound);
    lastNumber = wheel.GetDisplayedNumber();

    foreach (Image image in imageInputLocked)
      image.gameObject.SetActive(false);
    foreach (LayoutBlock2d block in playerMockBlocks)
      block.Validate(false);
    
    for (int i = 0; i < textPlayerBids.Length; i++) {
      textPlayerBids[i].text = "";
    }

    for (int i = 0; i < imagePlayerBidBGs.Length; i++) {
      imagePlayerBidBGs[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < textPlayerSpinNow.Length; i++) {
      textPlayerSpinNow[i].gameObject.SetActive(currentPlayerIndex == i);
    }

    imageGear.color = GetPlayerUIColor(currentPlayerIndex);
  }

  int IncrementPlayerTurn(int index) {
    
    switch (index) {
    case 0:
      index = 2;
      break;
    case 1:
      index = 3;
      break;
    case 2:
      index = 1;
      break;
    case 3:
      index = 0;
      break;
    }
  
    if (index >= numPlayers)
      return IncrementPlayerTurn(index);

    return index;
  }

  int GetPoseIndex(int playerIndex) {
    int index = playerIndex;
    switch (playerIndex) {
    case 1:
      index = 0;
      break;
    case 2:
      index = 1;
      break;
    case 3:
      index = 2;
      break;
    default:
      break;
    }
    return index;
  }

  CozmoUtil.RobotPose GetPoseFromPlayerIndex(int playerIndex) {

    return robotPoses[GetPoseIndex(playerIndex)];
  }

  void Update_REQUEST_SPIN() {
    
    //cozmo's automated spinWheel dragging
    if (currentPlayerIndex == cozmoIndex && playStateTimer > cozmoDragDelay) {

      dragTimer -= Time.deltaTime;
    
      if (dragTimer <= 0f) {

        Vector2 temp = startDragPos;
        startDragPos = endDragPos;
        endDragPos = temp;
        dragTimer = cozmoDragTime;
        dragCount++;

        if (dragCount >= dragCountMax) {
          Vector2 delta = endDragPos - startDragPos;
          endDragPos += delta * 2f;
          dragTimer = cozmoFinalDragTime;
        }
      }

      //we drag back and forth some to make it obvious coz is spinning
      if (dragCount < dragCountMax) {
        float factor = 1f - Mathf.Clamp01(dragTimer / cozmoDragTime);
        Vector2 currentPos = Vector2.Lerp(startDragPos, endDragPos, factor);
        wheel.DragUpdate(currentPos, Time.time);
        PlaceCozmoTouch(currentPos);
      }
      else {
        float factor = 1f - Mathf.Clamp01(dragTimer / cozmoFinalDragTime);
        Vector2 currentPos = Vector2.Lerp(startDragPos, endDragPos, factor);
        wheel.DragUpdate(currentPos, Time.time);
        if (factor > 0.7f) {
          wheel.DragEnd();
          if (imageCozmoDragHand != null) {
            imageCozmoDragHand.gameObject.SetActive(false);
          }
        }
        else {  
          PlaceCozmoTouch(currentPos);
        }
      }
    }
    else {
      // face hunt
      // only note faces when within 45 degrees of desired rotation
      float current = robot.poseAngle_rad;
      float target = GetPoseFromPlayerIndex(currentPlayerIndex).rad;
      float angle_between = Mathf.Atan2(Mathf.Sin(current - target), Mathf.Cos(current - target));
      //DAS.Debug("Vortex", "angle_between " + angle_between);
      float diff = Mathf.Abs(current - target);
      angle_between = diff > Mathf.PI ? diff - 2 * Mathf.PI : diff;
      //DAS.Debug("Vortex", "angle_between2 " + angle_between);
      if (Mathf.Abs(angle_between) < (Mathf.PI / 4.0f)) {
        for (int i = 0; i < robot.markersVisibleObjects.Count; ++i) {
          if (robot.markersVisibleObjects[i].isFace) {
            //get angle to that and set robotpos
            Vector3 target_pos = new Vector3(robot.markersVisibleObjects[i].WorldPosition.x, robot.markersVisibleObjects[i].WorldPosition.y, 0);
            Vector3 robot_pos = new Vector3(robot.WorldPosition.x, robot.WorldPosition.y, 0);
            Vector3 to_target = target_pos - robot_pos;
            to_target = to_target.normalized;
            robotPoses[GetPoseIndex(currentPlayerIndex)].rad = Mathf.Acos(Vector3.Dot(Vector3.right, to_target));
            DAS.Debug("Vortex", "Setting desired rad to " + robotPoses[GetPoseIndex(currentPlayerIndex)].rad);
          }
        }
      }

    }

    int newNumber = wheel.GetDisplayedNumber();

    if (newNumber != lastNumber) {
      lightingBall.SingleLightningBolt();
    }
    lastNumber = newNumber;
  }

  void Exit_REQUEST_SPIN() {
    ClearInputs();
    for (int i = 0; i < textPlayerSpinNow.Length; i++) {
      textPlayerSpinNow[i].gameObject.SetActive(false);
    }
    if (imageCozmoDragHand != null) {
      imageCozmoDragHand.gameObject.SetActive(false);
    }

    for (int i = 0; i < textPlayerBids.Length; i++) {
      textPlayerBids[i].text = "?";
    }
    
    for (int i = 0; i < imagePlayerBidBGs.Length; i++) {
      imagePlayerBidBGs[i].gameObject.SetActive(true);
    }

  }

  void Enter_SPINNING() {

    if (cozmoIndex == currentPlayerIndex) {
      SetRobotEmotion("WATCH_SPIN", false, false);
    }
    else {
      CozmoEmotionManager.instance.SetEmotionTurnInPlace("WATCH_SPIN", robotPoses[0].rad, true, false, true);
    }

    lightingBall.Radius = wheelLightningRadii[currentWheelIndex];

    for (int i = 0; i < playerButtonCanvasGroups.Length; i++) {
      if (i == cozmoIndex)
        continue; // player 2 is cozmo, no button
      playerButtonCanvasGroups[i].interactable = i < numPlayers;
      playerButtonCanvasGroups[i].blocksRaycasts = i < numPlayers;
    }
    predictedNum = -1;
    predictedDuration = -1f;
    predictedTimeAfterLastPeg = -1f;
    predictedTimeMovingBackwards = 0f;
    cozmoTapsSubmitted = -1;
    lightIndex = 0;

    RefreshSpinningLights();
  }

  void RefreshSpinningLights() {
    
    for (int i = 0; i < numPlayers; i++) {
      if (i >= playerInputs.Count)
        break;

      //only do spinning lights until bidding starts
      if (playerInputs[i].stamps.Count > 0)
        continue;
      
      if (playerInputBlocks.Count > i) {
        uint c1 = CozmoPalette.ColorToUInt(lightIndex == 0 ? Color.white : Color.clear);
        uint c2 = CozmoPalette.ColorToUInt(lightIndex == 1 ? Color.white : Color.clear);
        uint c3 = CozmoPalette.ColorToUInt(lightIndex == 2 ? Color.white : Color.clear);
        uint c4 = CozmoPalette.ColorToUInt(lightIndex == 3 ? Color.white : Color.clear);

        playerInputBlocks[i].SetLEDs(c1, 0, (byte)ActiveBlock.Light.IndexToPosition(0), Robot.Light.FOREVER, 0, 0, 0, 0);
        playerInputBlocks[i].SetLEDs(c2, 0, (byte)ActiveBlock.Light.IndexToPosition(1), Robot.Light.FOREVER, 0, 0, 0, 0);
        playerInputBlocks[i].SetLEDs(c3, 0, (byte)ActiveBlock.Light.IndexToPosition(2), Robot.Light.FOREVER, 0, 0, 0, 0);
        playerInputBlocks[i].SetLEDs(c4, 0, (byte)ActiveBlock.Light.IndexToPosition(3), Robot.Light.FOREVER, 0, 0, 0, 0);
      }

      if (playerMockBlocks.Length > i) {

        Color col1 = lightIndex == 0 ? Color.white : Color.black;
        Color col2 = lightIndex == 1 ? Color.white : Color.black;
        Color col3 = lightIndex == 2 ? Color.white : Color.black;
        Color col4 = lightIndex == 3 ? Color.white : Color.black;

        playerMockBlocks[i].SetLights(col1, col2, col3, col4);
      }
    }

  }

  void Update_SPINNING() {
    int lightingMin = Mathf.FloorToInt(Mathf.Lerp(0, lightningMinCountAtSpeedMax, (wheel.Speed - 1f) * 0.1f));
    int lightingMax = Mathf.FloorToInt(Mathf.Lerp(0, lightningMaxCountAtSpeedMax, (wheel.Speed - 1f) * 0.1f));
    lightingBall.CountRange = new RangeOfIntegers { Minimum = lightingMin, Maximum = lightingMax };
    
    int newNumber = wheel.GetDisplayedNumber();

    if (newNumber != lastNumber) {

      if (lightingMax == 0) {
        lightingBall.SingleLightningBolt();
      }
      if (wheel.SpinClockWise) {
        lightIndex--;
        if (lightIndex < 0)
          lightIndex = 3;
      }
      else {
        lightIndex++;
        if (lightIndex >= 4)
          lightIndex = 0;
      }
    }


    lastNumber = newNumber;

    if (cozmoIndex >= 0) {
  
      if (predictedNum < 0) {
  
        int numCheck = wheel.PredictedNumber;
    
        if (numCheck > 0) {
          if (UnityEngine.Random.Range(0.0f, 1.0f) < cozmoPredictiveAccuracy) {
            predictedNum = numCheck;
            cozmoExpectation = CozmoExpectation.EXPECT_WIN_RIGHT;
          }
          else {
            if (UnityEngine.Random.Range(0.0f, 1.0f) < wheel.PredictedPreviousNumberWeight) {
              predictedNum = wheel.PredictedPreviousNumber;
              if (wheel.PredictedPreviousNumberWeight > wheel.PredictedNextNumberWeight) {
                cozmoExpectation = CozmoExpectation.EXPECT_WIN_WRONG;
              }
              else {
                cozmoExpectation = CozmoExpectation.KNOWS_WRONG;
              }
            }
            else {
              predictedNum = wheel.PredictedNextNumber;
              if (wheel.PredictedPreviousNumberWeight < wheel.PredictedNextNumberWeight) {
                cozmoExpectation = CozmoExpectation.EXPECT_WIN_WRONG;
              }
              else {
                cozmoExpectation = CozmoExpectation.KNOWS_WRONG;
              }
            }
          }
          predictedDuration = wheel.TotalDuration;
          predictedTimeAfterLastPeg = wheel.TimeAfterLastPeg;
          predictedTimeMovingBackwards = wheel.TimeMovingBackwards;
          if (UnityEngine.Random.Range(0.0f, 1.0f) < cozmoEarlyGuess) {
            predictedEarly = true;
          }
          else {
            predictedEarly = false;
          }
        }
      }
  
      if (cozmoTapsSubmitted < 0 && predictedNum > 0) {
        float time = Time.time - wheel.SpinStartTime;
        float earlyTime = 0;
        if (predictedEarly) {
          earlyTime = cozmoEarlyGuessTime;
        }
        float timeToBid = predictedDuration - predictedTimeAfterLastPeg - predictedTimeMovingBackwards - cozmoPredicitveLeadTime - earlyTime;
        if (time > timeToBid) {
  
          cozmoTapsSubmitted = 0;
  
          if (robot != null) {
            //robot.TapBlockOnGround(predictedNum);
            switch (predictedNum) {
            case 1:
              CozmoEmotionManager.SetEmotion("TAP_ONE", true);
              break;
            case 2:
              CozmoEmotionManager.SetEmotion("TAP_TWO", true);
              break;
            case 3:
              CozmoEmotionManager.SetEmotion("TAP_THREE", true);
              break;
            case 4:
              CozmoEmotionManager.SetEmotion("TAP_FOUR", true);
              break;
            default:
              break;
            }
  
            //if we aren't faking cozmo's taps, then let's skip our local tapping for him
            if (!fakeCozmoTaps)
              cozmoTapsSubmitted = predictedNum;
          }

          StartCoroutine(SetCozmoTaps(predictedNum));

        }
      }
    }

    for (int i = 0; i < playerPanelFills.Length; i++) {
      if (i >= numPlayers || playerPanelFills[i].color.a == 0f)
        continue;
      Color col = playerPanelFills[i].color;
      col.a = Mathf.Max(0f, col.a - Time.deltaTime * 4f);
      playerPanelFills[i].color = col;
    }

    imageGear.color = Color.Lerp(GetPlayerUIColor(currentPlayerIndex), gearDefaultColor, playStateTimer);

    RefreshSpinningLights();

  }

  void Exit_SPINNING() {
    //stop looping spin audio
    //play spin finished audio
    wheel.Lock();
    lightingBall.CountRange = new RangeOfIntegers { Minimum = 0, Maximum = 0 };

    for (int i = 0; i < playerButtonCanvasGroups.Length; i++) {
      playerButtonCanvasGroups[i].interactable = false;
      playerButtonCanvasGroups[i].blocksRaycasts = false;
    }


    for (int i = 0; i < playerPanelFills.Length; i++) {
      if (i >= numPlayers || playerPanelFills[i].color.a == 0f)
        continue;
      Color col = playerPanelFills[i].color;
      col.a = 0f;
      playerPanelFills[i].color = col;
    }

    for (int i = 0; i < playerInputBlocks.Count && i < numPlayers; i++) {
      playerInputBlocks[i].SetMode(GetPlayerActiveCubeColorMode(i));
    }
    for (int i = 0; i < playerMockBlocks.Length && i < numPlayers; i++) {
      playerMockBlocks[i].SetLights(GetPlayerUIColor(i));
    }
    
  }

  void Enter_SPIN_COMPLETE() {
    
    playersThatAreCorrect.Clear();
    playersThatAreWrong.Clear();
    int fastestPlayer = -1;

    scoreDeltas.Clear();
    while (scoreDeltas.Count < scores.Count)
      scoreDeltas.Add(0);

    int number = wheel.GetDisplayedNumber();
    for (int i = 0; i < playerInputs.Count; i++) {
      if (playerInputs[i].stamps.Count != number)
        continue;
      if (playerInputs[i].stamps.Count == 0)
        continue;

      playersThatAreCorrect.Add(i);
    }

    playersThatAreCorrect.Sort(( obj1, obj2) => {
      float finalStamp1 = playerInputs[obj1].FinalTime;
      float finalStamp2 = playerInputs[obj2].FinalTime;

      if (finalStamp1 == finalStamp2)
        return 0;
      if (finalStamp1 > finalStamp2)
        return 1;
      return -1;   
    });

    if (playersThatAreCorrect.Count > 0) {
      fastestPlayer = playersThatAreCorrect[0];
    }

    for (int i = 0; i < numPlayers; i++) {
      if (playersEliminated[i])
        continue;
      if (playersThatAreCorrect.Contains(i))
        continue;
      if (playerInputs[i].stamps.Count == 0)
        continue;
      playersThatAreWrong.Add(i);
    }

    for (int i = 0; i < playersEliminated.Length; i++) {
      if (playersEliminated[i])
        continue;
      
      if (settings.winnerEliminated) {
        playersEliminated[i] = fastestPlayer == i;
      }
      else if (settings.elimination) {
        playersEliminated[i] = !playersThatAreCorrect.Contains(i) || playersThatAreCorrect.IndexOf(i) >= settings.maxSurvivePerRound[round - 1];
      }
    }

    if (cozmoIndex >= 0) {
  
      // cozmo reaction
      if (playersThatAreCorrect.Contains(1)) {
        if (playersThatAreCorrect[0] == cozmoIndex) {
          // major win
          SetRobotEmotion("MAJOR_WIN");
        }
        else {
          // minor win
          SetRobotEmotion("MINOR_WIN");
        }
      }
      else if (playersThatAreWrong.Contains(1)) {
        if (scores[cozmoIndex] < Math.Abs(settings.pointsIncorrectPenalty)) {
          // minor loss
          SetRobotEmotion("MINOR_FAIL");
        }
        else {
          SetRobotEmotion("MAJOR_FAIL");
        }
  
      }
    }

    for (int i = 0; i < playerInputs.Count; i++) {
      textPlayerBids[i].text = playerInputs[i].stamps.Count.ToString();
    }

    for (int i = 0; i < imagePlayerBidBGs.Length; i++) {
      imagePlayerBidBGs[i].gameObject.SetActive(true);
    }


    PlayRoundCompleteLights();
    int high_score = -1;
    for (int i = 0; i < scores.Count; i++) {
      if (scores[i] > high_score) {
        high_score = scores[i];
        lastLeader = i;
      }
    }

    DAS.Debug("Vortex", "last leader index is: " + lastLeader);

    //only winner is given points per round in winnerElimination
    if (settings.winnerEliminated) {
      if (fastestPlayer >= 0) {

        int eliminated = 0;
        for (int i = 0; i < playersEliminated.Length; i++)
          if (playersEliminated[i])
            eliminated++;

        int delta = 0;
        switch (eliminated) {
        case 1:
          delta = settings.pointsFirstPlace;
          break;
        case 2:
          delta = settings.pointsSecondPlace;
          break;
        case 3:
          delta = settings.pointsThirdPlace;
          break;
        case 4:
          delta = settings.pointsFourthPlace;
          break;
        }


        scoreDeltas[fastestPlayer] = delta;
        scores[fastestPlayer] += delta;
      }

    }
    else {
      for (int i = 0; i < playersThatAreCorrect.Count; i++) {
        int playerIndex = playersThatAreCorrect[i];
        if (playersEliminated[playerIndex])
          continue;
        int delta = 0;
        
        switch (i) {
        case 0:
          delta = settings.pointsFirstPlace;
          break;
        case 1:
          delta = settings.pointsSecondPlace;
          break;
        case 2:
          delta = settings.pointsThirdPlace;
          break;
        case 3:
          delta = settings.pointsFourthPlace;
          break;
        }

        scoreDeltas[playerIndex] = delta;
        scores[playerIndex] += delta;
      }
    }

    for (int i = 0; i < playersThatAreWrong.Count; i++) {
      scoreDeltas[playersThatAreWrong[i]] = settings.pointsIncorrectPenalty;
      scores[playersThatAreWrong[i]] = Mathf.Max(0, scores[playersThatAreWrong[i]] + settings.pointsIncorrectPenalty);
    }
    
//    if(playersThatAreCorrect.Count == 0) {
//      if(roundCompleteNoWinner != null) AudioManager.PlayOneShot(roundCompleteNoWinner);
//    }

    resultsDisplayIndex = 0;
    fadeTimer = scoreDisplayFillFade;
  }

  void PlayRoundCompleteLights() {
    
    int random;
    int max = (int)ActiveBlock.Mode.Count - 3;
    
    for (int playerIndex = 0; playerIndex < playerInputs.Count; playerIndex++) {
      
      textPlayerBids[playerIndex].text = playerInputs[playerIndex].stamps.Count.ToString();
      imagePlayerBidBGs[playerIndex].gameObject.SetActive(true);
      
      bool correct = playersThatAreCorrect.Contains(playerIndex);
      bool winner = correct && playersThatAreCorrect[0] == playerIndex;
      uint playerColor = CozmoPalette.ColorToUInt(CozmoPalette.instance.GetColorForActiveBlockMode(GetPlayerActiveCubeColorMode(playerIndex)));
      
      if (playerInputBlocks.Count > playerIndex) {
        
        ActiveBlock block = playerInputBlocks[playerIndex];
        
        if (!correct) {
          block.SetMode(ActiveBlock.Mode.Off);
          if (robot != null && playerIndex == cozmoIndex) {  
            robot.SetBackpackLEDs();
          }
          continue;
        }
        
        block.relativeMode = 0;
        random = UnityEngine.Random.Range(1, max);
        
        for (int i = 0; i < block.lights.Length; ++i) {
          block.lights[i].onColor = winner ? CozmoPalette.CycleColors(i) : playerColor;
          block.lights[i].onPeriod_ms = 125;
          block.lights[i].offPeriod_ms = 125;
          block.lights[i].offColor = winner ? CozmoPalette.CycleColors(i + random) : 0;
          block.lights[i].transitionOnPeriod_ms = 0;
          block.lights[i].transitionOffPeriod_ms = 0;
        }
      }
      
      //playerMockBlocks[playerIndex]
      
      if (robot != null && playerIndex == cozmoIndex) {      
        
        random = UnityEngine.Random.Range(1, max);
        
        for (int i = 0; i < robot.lights.Length; ++i) {
          robot.lights[i].onColor = winner ? CozmoPalette.CycleColors(i) : playerColor;
          robot.lights[i].onPeriod_ms = 125;
          robot.lights[i].offPeriod_ms = 125;
          robot.lights[i].offColor = winner ? CozmoPalette.CycleColors(i + random) : 0;
          robot.lights[i].transitionOnPeriod_ms = 0;
          robot.lights[i].transitionOffPeriod_ms = 0;
        }
      }
    }
    
  }

  void Update_SPIN_COMPLETE() {
    if (playersThatAreCorrect.Count == 0)
      return;
    if (resultsDisplayIndex > playersThatAreCorrect.Count)
      return;

    bool wasPositive = fadeTimer > 0f;
    fadeTimer -= Time.deltaTime;
    Color col = Color.black;

    //if done with correct players, let's display all the losers at once
    if (resultsDisplayIndex == playersThatAreCorrect.Count) {

      for (int i = 0; i < playersThatAreWrong.Count; i++) {

        int loserIndex = playersThatAreWrong[i];
        
        col = playerPanelFills[loserIndex].color;
        
        if (fadeTimer > 0f) {
          float factor = fadeTimer / scoreDisplayFillFade;
          col.a = Mathf.Lerp(scoreDisplayFillAlpha, 0f, factor);
          textPlayerScores[loserIndex].rectTransform.localScale = Vector3.Lerp(Vector3.one * scoreScaleMax, Vector3.one * scoreScaleBase, factor);
        }
        else {
          float factor = Mathf.Abs(fadeTimer) / scoreDisplayFillFade;
          col.a = Mathf.Lerp(scoreDisplayFillAlpha, scoreDisplayEmptyAlpha, factor);
          textPlayerScores[loserIndex].rectTransform.localScale = Vector3.Lerp(Vector3.one * scoreScaleMax, Vector3.one * scoreScaleBase, factor);
          
          if (wasPositive) {
            
            textPlayerScoreDeltas[loserIndex].text = scoreDeltas[loserIndex] + "!";
            textPlayerScoreDeltas[loserIndex].gameObject.SetActive(true);
            
            textPlayerScores[loserIndex].text = "SCORE: " + scores[loserIndex].ToString();
            
          }
        }
      
        playerPanelFills[loserIndex].color = col;
      }

      if (wasPositive && fadeTimer <= 0f) {
        if (roundCompleteNoWinner != null)
          AudioManager.PlayOneShot(roundCompleteNoWinner);
      }
      
      if (fadeTimer <= -scoreDisplayFillFade) {
        resultsDisplayIndex++;
      }

      return;
    }

    int playerIndex = playersThatAreCorrect[resultsDisplayIndex];

    col = playerPanelFills[playerIndex].color;
    
    if (fadeTimer > 0f) {
      float factor = fadeTimer / scoreDisplayFillFade;
      col.a = Mathf.Lerp(scoreDisplayFillAlpha, 0f, factor);
      textPlayerScores[playerIndex].rectTransform.localScale = Vector3.Lerp(Vector3.one * scoreScaleMax, Vector3.one * scoreScaleBase, factor);
    }
    else {
      float factor = Mathf.Abs(fadeTimer) / scoreDisplayFillFade;
      col.a = Mathf.Lerp(scoreDisplayFillAlpha, scoreDisplayEmptyAlpha, factor);
      textPlayerScores[playerIndex].rectTransform.localScale = Vector3.Lerp(Vector3.one * scoreScaleMax, Vector3.one * scoreScaleBase, factor);

      if (wasPositive) {

        textPlayerScoreDeltas[playerIndex].text = "+" + scoreDeltas[playerIndex] + "!";
        textPlayerScoreDeltas[playerIndex].gameObject.SetActive(true);

        textPlayerScores[playerIndex].text = "SCORE: " + scores[playerIndex].ToString();
        
        if (roundCompleteWinner != null)
          AudioManager.PlayOneShot(roundCompleteWinner);
      }
    }

    playerPanelFills[playerIndex].color = col;

    if (fadeTimer <= -scoreDisplayFillFade) {
      resultsDisplayIndex++;
      fadeTimer = scoreDisplayFillFade;
      //textPlayerScoreDeltas[playerIndex].gameObject.SetActive(false);
    }
  }

  void Exit_SPIN_COMPLETE() {
    for (int i = 0; i < playerPanelFills.Length; i++) {
      Color col = playerPanelFills[i].color;
      col.a = 0f;
      playerPanelFills[i].color = col;
    }

    for (int i = 0; i < textPlayerBids.Length; i++) {
      textPlayerBids[i].text = "";
    }

    for (int i = 0; i < imagePlayerBidBGs.Length; i++) {
      imagePlayerBidBGs[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < playerBidFlashAnimations.Length; i++) {
      playerBidFlashAnimations[i].Play();
    }
    
    for (int i = 0; i < playerInputBlocks.Count && i < numPlayers; i++) {
      playerInputBlocks[i].SetMode(GetPlayerActiveCubeColorMode(i));
    }

    for (int i = 0; i < playerMockBlocks.Length && i < numPlayers; i++) {
      playerMockBlocks[i].SetLights(GetPlayerUIColor(i));
    }
    

    for (int i = 0; i < textPlayerScores.Length && i < numPlayers; i++) {
      textPlayerScores[i].rectTransform.localScale = Vector3.one * scoreScaleBase;
    }

    for (int i = 0; i < textPlayerScoreDeltas.Length && i < numPlayers; i++) {
      textPlayerScoreDeltas[i].gameObject.SetActive(false);
    }

    if (robot != null) {  
      robot.SetBackpackLEDs();
    }
    
  }

  void PlaceTokens() {
    if (!settings.showTokens) {
      for (int i = 0; i < playerTokens.Length; i++) {
        playerTokens[i].gameObject.SetActive(false);
      }
      return;
    }

    for (int i = 0; i < playerTokens.Length && i < playerButtons.Length; i++) {
      playerTokens[i].gameObject.SetActive(i < numPlayers && !playersEliminated[i]);
      if (!playerTokens[i].gameObject.activeSelf)
        continue;

      Canvas tokenCanvas = playerTokens[i].gameObject.GetComponentInParent<Canvas>();
      RectTransform buttonT = playerButtons[i].transform as RectTransform;
      
      Vector2 screenButtonPos = RectTransformUtility.WorldToScreenPoint(null, buttonT.position);
      Vector2 localPos;
      RectTransformUtility.ScreenPointToLocalPointInRectangle(playerTokens[i], screenButtonPos, tokenCanvas.renderMode != RenderMode.ScreenSpaceOverlay ? tokenCanvas.worldCamera : null, out localPos);

      Vector2 anchorPos = localPos.normalized * (settings.inward ? wheel.TokenOuterRadius : wheel.TokenRadius);
      playerTokens[i].anchoredPosition = anchorPos;
    }
    
  }

  void ClearInputs() {
    for (int i = 0; i < playerInputs.Count; i++) {
      playerInputs[i].stamps.Clear();
    }

    for (int i = 0; i < textPlayerBids.Length; i++) {
      textPlayerBids[i].text = "";
    }

    for (int i = 0; i < imagePlayerBidBGs.Length; i++) {
      imagePlayerBidBGs[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < playerMockBlocks.Length; i++) {
      playerMockBlocks[i].Validate(false);
    }

    for (int i = 0; i < imageInputLocked.Length; i++) {
      imageInputLocked[i].gameObject.SetActive(false);
    }

    for (int i = 0; i < playerBidFlashAnimations.Length; i++) {
      playerBidFlashAnimations[i].Stop();
      playerBidFlashAnimations[i].Rewind();
    }
  }

  void BlockTapped(int blockID, int numTaps) {
    DAS.Debug("Vortex", "BlockTapped block(" + blockID + ")");
    for (int i = 0; i < playerInputBlocks.Count; i++) {
      if (playerInputBlocks[i] == null)
        continue;

      if (playerInputBlocks[i].ID != blockID)
        continue;

      //if we are faking cozmo's taps, let's ignore any real incoming messages for his block
      if (fakeCozmoTaps && i == cozmoIndex)
        continue;

      for (int j = 0; j < numTaps; j++) {
        PlayerInputTap(i);
      }
      break;
    }
  }

  void PlaceCozmoTouch(Vector2 screenPos) {
    if (imageCozmoDragHand == null)
      return;

    imageCozmoDragHand.gameObject.SetActive(true);
    RectTransform rTrans = cozmoDragHandRectTransform;
    Camera cam = imageCozmoDragHand.canvas.renderMode != RenderMode.ScreenSpaceOverlay ? imageCozmoDragHand.canvas.worldCamera : null;
    Vector2 anchor;
    RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, screenPos, cam, out anchor);
    imageCozmoDragHand.rectTransform.anchoredPosition = anchor;
  }

  ActiveBlock.Mode GetPlayerActiveCubeColorMode(int index) {

    switch (index) {
    case 0:
      return ActiveBlock.Mode.Blue;
    case 1:
      return ActiveBlock.Mode.Green;
    case 2:
      return ActiveBlock.Mode.Yellow;
    case 3:
      return ActiveBlock.Mode.Red;
    }

    return ActiveBlock.Mode.Off;
  }

  Color GetPlayerUIColor(int index) {
    return playerColors[index];
  }


  IEnumerator DelayBidLockedEffect(int index) {

    yield return new WaitForSeconds(maxPlayerInputTime);

    imageInputLocked[index].gameObject.SetActive(true);
  }

  IEnumerator TapAfterDelay(int index, float delay) {
    
    yield return new WaitForSeconds(delay);
    
    PlayerInputTap(index);
    robot.isBusy = false;
  }

  IEnumerator CozmoReaction() {
    switch (cozmoExpectation) {
    case CozmoExpectation.EXPECT_WIN_RIGHT:
    case CozmoExpectation.EXPECT_WIN_WRONG:
      CozmoEmotionManager.SetEmotion("EXPECT_REWARD");
      break;
    case CozmoExpectation.KNOWS_WRONG:
      CozmoEmotionManager.SetEmotion("KNOWS_WRONG");
      break;
    default:
      break;
    }
    float time = Time.time - wheel.SpinStartTime;
    yield return new WaitForSeconds(predictedDuration - time - predictedTimeMovingBackwards);
    if (cozmoExpectation == CozmoExpectation.EXPECT_WIN_WRONG) {
      SetRobotEmotion("SHOCKED");
    }

  }

  internal IEnumerator SetCozmoTaps(int predicted) {
    yield return new WaitForSeconds(cozmoTimeFirstTap);
    
    PlayerInputTap(1);
    cozmoTapsSubmitted++;

    for (int i = 0; i < predicted - 1; i++) {
      yield return new WaitForSeconds(cozmoTimePerTap);
      PlayerInputTap(1);
      cozmoTapsSubmitted++;
    }
  }

  void CheckForGotoStartCompletion(bool success, RobotActionType action_type) {
    DAS.Error("VortextController", "got " + action_type);
    switch (action_type) {
    case RobotActionType.COMPOUND:
      if (success) {
        atYourMark = true;
        if (RobotEngineManager.instance != null)
          RobotEngineManager.instance.SuccessOrFailure -= CheckForGotoStartCompletion;
        
        CozmoEmotionManager.SetEmotion("TAP_ONE", true);
        if (fakeCozmoTaps) {
          StartCoroutine(TapAfterDelay(1, cozmoTimeFirstTap));
        }
      }
      else { //try again to go to the start spot
        robot.GotoPose(robotPoses[0].x_mm, robotPoses[0].y_mm, robotPoses[0].rad);
      }
      break;
    case RobotActionType.DRIVE_TO_POSE:
      if (success) {
        atYourMark = true;
        if (RobotEngineManager.instance != null)
          RobotEngineManager.instance.SuccessOrFailure -= CheckForGotoStartCompletion;
      }
      else { //try again to go to the start spot
        robot.GotoPose(robotPoses[0].x_mm, robotPoses[0].y_mm, robotPoses[0].rad);
      }
      break;
    }
  }

  IEnumerator FlashBocks(int player_index) {
    Color playerColor = CozmoPalette.instance.GetColorForActiveBlockMode(GetPlayerActiveCubeColorMode(player_index));

    uint white = CozmoPalette.ColorToUInt(Color.white);
    uint black = CozmoPalette.ColorToUInt(Color.black);
    uint player = CozmoPalette.ColorToUInt(playerColor);

    playerInputBlocks[player_index].SetLEDs(white, 0, 0xFF, Robot.Light.FOREVER, 0, 0, 0, 0);
    yield return new WaitForSeconds(.5f);
    playerInputBlocks[player_index].SetLEDs(black, 0, 0xFF, Robot.Light.FOREVER, 0, 0, 0, 0);
    yield return new WaitForSeconds(.25f);
    playerInputBlocks[player_index].SetLEDs(player, 0, 0xFF, Robot.Light.FOREVER, 0, 0, 0, 0);




  }

  #endregion

  #region PUBLIC METHODS

  public void PlayerInputTap(int index) {

    DAS.Debug("Vortex", "PlayerInputTap index(" + index + ")");

    if (state == GameState.PRE_GAME) {
      if (index == cozmoIndex) { // cozmo
        //SetRobotEmotion ("LETS_PLAY");
        DAS.Error("VortextController", "lets play!");
        CozmoEmotionManager.SetEmotion("LETS_PLAY", true);
      }
      else {
        DAS.Debug("Vortex", "PlayerInputTap validating playerIndex(" + index + ")");
      }
      // flash blocks to indicate tapping
      StartCoroutine(FlashBocks(index));
      playerMockBlocks[index].Validate(true);
      if (buttonPressSound != null)
        AudioManager.PlayOneShot(buttonPressSound, 0f, 1f, 1f);
      return;
    }
    
    
    if (state != GameState.PLAYING)
      return;
    if (playState != VortexState.SPINNING)
      return;
    if (playersEliminated[index])
      return;
    
    while (index >= playerInputs.Count)
      playerInputs.Add(new VortexInput());
    if (playerInputs[index].stamps.Count >= 4)
      return;
    
    float time = Time.time;
    
    if (index != cozmoIndex && time - playerInputs[index].FirstTime > maxPlayerInputTime)
      return;
    
    playerInputs[index].stamps.Add(time);
    
    //    //if this is fifth stamp, then remove the prior 4 such that we go back to 1, 
    //    //  but still have our relevant 'last' time stamp at the end of the list
    //    if(playerInputs[index].stamps.Count > 4) {
    //      playerInputs[index].stamps.RemoveRange(0, 4);
    //    }
    
    if (index < playerInputBlocks.Count) {
      Color playerColor = CozmoPalette.instance.GetColorForActiveBlockMode(GetPlayerActiveCubeColorMode(index));
      
      uint c1 = CozmoPalette.ColorToUInt(playerInputs[index].stamps.Count > 0 ? playerColor : Color.black);
      uint c2 = CozmoPalette.ColorToUInt(playerInputs[index].stamps.Count > 1 ? playerColor : Color.black);
      uint c3 = CozmoPalette.ColorToUInt(playerInputs[index].stamps.Count > 2 ? playerColor : Color.black);
      uint c4 = CozmoPalette.ColorToUInt(playerInputs[index].stamps.Count > 3 ? playerColor : Color.black);
      
      playerInputBlocks[index].SetLEDs(c1, 0, (byte)ActiveBlock.Light.IndexToPosition(0), Robot.Light.FOREVER, 0, 0, 0, 0);
      playerInputBlocks[index].SetLEDs(c2, 0, (byte)ActiveBlock.Light.IndexToPosition(1), Robot.Light.FOREVER, 0, 0, 0, 0);
      playerInputBlocks[index].SetLEDs(c3, 0, (byte)ActiveBlock.Light.IndexToPosition(2), Robot.Light.FOREVER, 0, 0, 0, 0);
      playerInputBlocks[index].SetLEDs(c4, 0, (byte)ActiveBlock.Light.IndexToPosition(3), Robot.Light.FOREVER, 0, 0, 0, 0);
    }
    
    Color uiCol = GetPlayerUIColor(index);
    
    Color uiCol1 = playerInputs[index].stamps.Count > 0 ? uiCol : Color.black;
    Color uiCol2 = playerInputs[index].stamps.Count > 1 ? uiCol : Color.black;
    Color uiCol3 = playerInputs[index].stamps.Count > 2 ? uiCol : Color.black;
    Color uiCol4 = playerInputs[index].stamps.Count > 3 ? uiCol : Color.black;
    
    playerMockBlocks[index].SetLights(uiCol1, uiCol2, uiCol3, uiCol4);
    
    
    textPlayerBids[index].text = playerInputs[index].stamps.Count.ToString();
    imagePlayerBidBGs[index].gameObject.SetActive(true);
    
    playerBidFlashAnimations[index].Rewind();
    playerBidFlashAnimations[index].Play();
    
    Color fillCol = playerPanelFills[index].color;
    fillCol.a = 0.5f;
    playerPanelFills[index].color = fillCol;
    
    if (buttonPressSound != null)
      AudioManager.PlayOneShot(buttonPressSound, 0f, 1f, 1f);

    if (playerInputs[index].stamps.Count == 1) {
      StartCoroutine(DelayBidLockedEffect(index));
    }
  }

  public void SetRobotStartingPositions() {
    for (int i = 0; i < 3; i++) {
      float[] rads = new float [3]{ (3.0f * Mathf.PI) / 2.0f, 0f, Mathf.PI };
      float mark_rad = robot.poseAngle_rad + rads[i];
      mark_rad = mark_rad < 2.0f * Mathf.PI ? mark_rad : mark_rad - 2.0f * Mathf.PI;
      robotPoses[i].rad = mark_rad;
      Vector2 cozmo_desired_facing = MathUtil.RotateVector2d(Vector3.right, mark_rad);
      Vector2 offset = new Vector2(robot.WorldPosition.x, robot.WorldPosition.y) - (50 * cozmo_desired_facing.normalized);
      robotPoses[i].x_mm = offset.x;
      robotPoses[i].y_mm = offset.y;
    }
  }

  #endregion

}
