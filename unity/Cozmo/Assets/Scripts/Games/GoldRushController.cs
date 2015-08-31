#define RUSH_DEBUG
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.Cozmo;
using System;

public class GoldRushController : GameController {

  public const uint EXTRACTOR_COLOR = 0x00FFFFFF;

  [SerializeField] protected AudioClip locatorBeep;
  [SerializeField] protected AudioClip foundBeep;
  [SerializeField] protected AudioClip collectedSound;
  [SerializeField] protected AudioClip pickupEnergyScanner;
  [SerializeField] protected AudioClip placeEnergyScanner;
  [SerializeField] protected AudioClip findEnergy;
  [SerializeField] protected AudioClip dropEnergy;
  [SerializeField] protected AudioClip extractingEnergy;
  [SerializeField] protected AudioClip depositingEnergy;
  [SerializeField] protected AudioClip timeUp;
  [SerializeField] protected AudioClip newHighScore;

  [SerializeField] protected AudioClip timeExtension;
  [SerializeField] protected AudioClip[] scoreSounds;
  [SerializeField] protected AudioClip points;
  public float detectRangeDelayFar = 2.0f;
  public float detectRangeDelayClose = .2f;
  public float light_messaging_delay = .05f;

  private int numDrops = 0;

  //radius around origin's initial position in which its gold will be buried
  [SerializeField] float hideRadius;

  //dropping cube within find radius will trigger transmutation/score
  [SerializeField] float findRadius;

  //leaving lost radius will put you back in searching mode
  [SerializeField] float lostRadius;

  //dropping cube within find radius will trigger transmutation/score
  [SerializeField] float returnRadius;

  //dropping cube within find radius will trigger transmutation/score
  [SerializeField] float autoReturnRadius;

  //pulsing will accelerate from detect to find ranges
  [SerializeField] float detectRadius;

  [SerializeField] int numDropsForBonusTime = 1;

  [SerializeField] float baseTimeBonus = 30f;

  [SerializeField] float bonusTimeDecay = 2f / 3f;

  //for the results screen
  [SerializeField] Text resultsScore;

  //for the results screen
  [SerializeField] Text resultsHighScore;


  //List<ObservedObject> goldCubes = new List<ObservedObject>();
  Dictionary<int, Vector2> buriedLocations = new Dictionary<int, Vector2>();
  List<int> foundItems = new List<int>();
  int lastCarriedObjectId = -1;
  [System.NonSerialized] public ActiveBlock goldExtractingObject = null;
  [System.NonSerialized] public ObservedObject goldCollectingObject = null;
  //ScreenMessage //hintMessage;
  private bool audioLocatorEnabled = true;
  private bool setHighScore = false;
  private int oldHighScore = 0;
  GameObject successOrFailureGameObject = null;

  enum PlayState {
    IDLE,
    SEARCHING,
    CAN_EXTRACT,
    EXTRACTING,
    READY_TO_RETURN,
    RETURNING,
    AUTO_DEPOSITING,
    RETURNED,
    DEPOSITING,
    NUMSTATES}

  ;

  private PlayState playState = PlayState.IDLE;
  float playStateTimer = 0;

  // only increments when the robot is searching for or returing gold
  float totalActiveTime = 0;



  public bool inExtractRange { get { return playState == PlayState.CAN_EXTRACT; } }

  public bool inDepositRange { get { return playState == PlayState.RETURNED; } }

  public bool isReturning { get { return playState == PlayState.RETURNING; } }

  enum BuildState {
    WAITING_TO_PICKUP_BLOCK,
    WAITING_FOR_STACK,
    WAITING_FOR_PLAY,
    NUMSTATES}

  ;

  private BuildState buildState = BuildState.WAITING_TO_PICKUP_BLOCK;

  protected override void Awake() {
    base.Awake();

    //hintMessage = GetComponentInChildren<ScreenMessage> ();
  }

  protected override void OnEnable() {
    base.OnEnable();

    MessageDelay = .1f;
  }

  protected override void OnDisable() {
    base.OnDisable();

    if (RobotEngineManager.instance != null) {
      RobotEngineManager.instance.VisualizeQuad(21, CozmoPalette.ColorToUInt(Color.clear), Vector3.zero, Vector3.zero, Vector3.zero, Vector3.zero);
      RobotEngineManager.instance.VisualizeQuad(22, CozmoPalette.ColorToUInt(Color.clear), Vector3.zero, Vector3.zero, Vector3.zero, Vector3.zero);
      RobotEngineManager.instance.VisualizeQuad(23, CozmoPalette.ColorToUInt(Color.clear), Vector3.zero, Vector3.zero, Vector3.zero, Vector3.zero);
    }
  }

  protected override void RefreshHUD() {
    base.RefreshHUD();

    // need timer to reflect our games unique use of it
    if (textTime != null && state == GameState.PLAYING) {
      textTime.text = Mathf.FloorToInt(levelData.maxPlayTime + bonusTime - totalActiveTime).ToString() + suffix_seconds;
    }

    if (textScore != null && state == GameState.PLAYING) {
      textScore.text = score.ToString();  
    }
    else {
      textScore.text = string.Empty;
    }
  }

  protected override void Enter_BUILDING() {
    base.Enter_BUILDING();
    //RobotEngineManager.instance.SuccessOrFailure += PickedUpActiveBlock;
  }

  protected override void Exit_BUILDING() {
    base.Exit_BUILDING();
    //RobotEngineManager.instance.SuccessOrFailure -= PickedUpActiveBlock;
  }

  protected override void Update_BUILDING() {
    base.Update_BUILDING();
#if RUSH_DEBUG
    if (robot != null) {
      var enumerator = robot.activeBlocks.GetEnumerator();

      while (enumerator.MoveNext()) {
        goldExtractingObject = enumerator.Current.Value;
      }
    }

    if (Input.GetKeyDown(KeyCode.Keypad1)) {
      StartCoroutine(AwardPoints());
    }
    else if (Input.GetKeyDown(KeyCode.Keypad2)) {
      StartCoroutine(StartExtracting());
    }
    else if (Input.GetKeyDown(KeyCode.Keypad3)) {
      playStateTimer = 0;
      EnterPlayState(PlayState.CAN_EXTRACT);
    }
    else if (Input.GetKeyDown(KeyCode.Keypad4)) {
      EnterPlayState(PlayState.IDLE);
    }

    if (playState == PlayState.CAN_EXTRACT) {
      UpdateCanExtract();
      playStateTimer += Time.deltaTime;
    }
#endif
  }

  protected override void Enter_PLAYING() {
    base.Enter_PLAYING();
    playState = PlayState.IDLE;
    playStateTimer = 0;
    totalActiveTime = 0;
    numDrops = 0;

    //CozmoVision.EnableDing(false); // just in case we were in searching mode
    SetEnergyBars(0, 0);
    setHighScore = false;
    oldHighScore = PlayerPrefs.GetInt("EnergyHunt_HighScore", 0);

    SuccessOrFailureText text = FindObjectOfType<SuccessOrFailureText>();
    if (text != null) {
      successOrFailureGameObject = text.gameObject;
    }
    else {
      DAS.Error("GoldRushController", "unable to find object of type SuccessOrFailureText");
    }

    if (successOrFailureGameObject != null) {
      successOrFailureGameObject.SetActive(false);
    }

    RobotEngineManager.instance.SuccessOrFailure += CheckForAutoDepositFailure;
  }

  protected override void Exit_PLAYING(bool overrideStars = false) {
    base.Exit_PLAYING();
    AudioManager.Stop(); // stop all audio
    StopAllCoroutines();
    AudioManager.PlayAudioClip(timeUp, 0f, AudioManager.Source.Notification);
    resultsScore.text = "Score: " + score;
    if (setHighScore) {
      PlayerPrefs.SetInt("EnergyHunt_HighScore", score);
      resultsHighScore.color = Color.red;
      resultsHighScore.text = "New High Score!";
    }
    else {
      resultsHighScore.color = resultsScore.color;
      resultsHighScore.text = "High Score: " + oldHighScore.ToString();
    }
    CozmoVision.EnableDing(); // just in case we were in searching mode
    //ActionButton.DROP = null;
    //RobotEngineManager.instance.SuccessOrFailure -= CheckForGoldDropOff;
    playState = PlayState.IDLE;
    SetAllDetectorLights(0);
    AudioManager.Stop(AudioManager.Source.UI);
    SetEnergyBars(0, 0);
    if (successOrFailureGameObject != null) {
      successOrFailureGameObject.SetActive(true);
    }
    RobotEngineManager.instance.SuccessOrFailure -= CheckForAutoDepositFailure;
  }

  protected override void Update_PLAYING() {
    base.Update_PLAYING();

    playStateTimer += Time.deltaTime;

    int secondsLeft = Mathf.CeilToInt((levelData.maxPlayTime + bonusTime) - totalActiveTime);
    PlayCountdownAudio(secondsLeft);

    if (robot != null) {
      for (int i = 0; i < robot.knownObjects.Count; i++) {
        ObservedObject obj = robot.knownObjects[i];
        if (!obj.isActive)
          goldCollectingObject = obj;
      }
    }

    switch (playState) {
    case PlayState.IDLE:
      UpdateIdle();
      break;
    case PlayState.SEARCHING:
      UpdateSearching();
      break;
    case PlayState.EXTRACTING:
      UpdateExtracting();
      break;
    case PlayState.RETURNING:
      UpdateReturning();
      break;
    case PlayState.READY_TO_RETURN:
      UpdateReadyToReturn();
      break;
    case PlayState.RETURNED:
      UpdateReturned();
      break;
    case PlayState.CAN_EXTRACT:
      UpdateCanExtract();
      break;
    default:
      break;
    }
  }

  Vector2 cozmoForwardWhenPickUp = Vector2.right;

  protected override void Enter_PRE_GAME() {
    DAS.Debug("GoldRushController", "Enter_PRE_GAME");
    base.Enter_PRE_GAME();

    if (robot == null)
      return;

    lastCarriedObjectId = -1;
    goldExtractingObject = null;
    //goldCollectingObject = null;

    RefreshGameProps();

    //hintMessage.ShowMessage("Pick up the energy scanner to begin", Color.black);
    if (robot.carryingObject == null || (int)robot.carryingObject != goldExtractingObject) {
      //PlayNotificationAudio (pickupEnergyScanner);

      robot.PickAndPlaceObject(goldExtractingObject);
      if (CozmoBusyPanel.instance != null) {
        string desc = "Cozmo is attempting to pick-up\n the extractor.";
        CozmoBusyPanel.instance.SetDescription(desc);
      }
    }

    RobotEngineManager.instance.SuccessOrFailure += PickedUpActiveBlock;
  }

  protected override void Exit_PRE_GAME() {
    DAS.Debug("GoldRushController", "Exit_PRE_GAME");
    //hintMessage.KillMessage ();
    base.Exit_PRE_GAME();

    RobotEngineManager.instance.SuccessOrFailure -= PickedUpActiveBlock;
  }

  protected override void Update_PRE_GAME() {

    if (stateTimer < pickupEnergyScanner.length)
      return;

    RefreshGameProps();
    if (!PropsAreCorrect())
      return;

    base.Update_PRE_GAME();



#if RUSH_DEBUG
//    if (goldExtractingObject != null && robot.carryingObject != null ) 
//    {
//      UpdateDirectionLights(Vector2.zero);
//    }

//    if( Input.GetKeyDown(KeyCode.C ))
//    {
//      StartCoroutine(CountdownToPlay());
//    }

#endif

  }

  void RefreshGameProps() {
    if (robot == null)
      return;
    for (int i = 0; i < robot.knownObjects.Count; i++) {
      ObservedObject obj = robot.knownObjects[i];
      if (obj.isActive && goldExtractingObject == null) {
        goldExtractingObject = obj as ActiveBlock;
        //goldExtractingObject.SetLEDs(EXTRACTOR_COLOR, 0, 0xFF, 150, 150); 
      }
      else if (!obj.isActive) {
        goldCollectingObject = obj;
      }
    }
  }

  void CheckForStackSuccess(bool success, RobotActionType action_type) {
    DAS.Debug("GoldRushController", "action type is: " + action_type);
    if (success) { // hardcoded until we get enums over from the engine
      switch (buildState) {
      case BuildState.WAITING_TO_PICKUP_BLOCK:
        if ((int)action_type == 5 || (int)action_type == 6) {
          // picked up our detector block (will need to verify that it's an active block later)
          buildState = BuildState.WAITING_FOR_STACK;
          goldExtractingObject = robot.carryingObject as ActiveBlock;
          //UpdateDetectorLights (1);
          goldExtractingObject.SetLEDs(0); 
          //hintMessage.ShowMessage("Place the scanner on the transformer", Color.black);
          AudioManager.PlayAudioClip(placeEnergyScanner, 0f, AudioManager.Source.Notification);
        }
        break;
      case BuildState.WAITING_FOR_STACK:
        if ((int)action_type == 8) {
          // stacked our detector block
          buildState = BuildState.WAITING_FOR_PLAY;
          AudioManager.PlayAudioClip(pickupEnergyScanner, 0f, AudioManager.Source.Notification);
          //hintMessage.ShowMessage("Pick up the scanner to begin play", Color.black);
        }
        break;
      case BuildState.WAITING_FOR_PLAY:
        if ((int)action_type == 6) {
          // start the game
          //StartCoroutine(CountdownToPlay());
          //PlayRequested();
          //hintMessage.KillMessage();
        }
        break;
      default:
        break;
      }
    }
  }

  protected override bool IsGameReady() {
    if (!base.IsGameReady())
      return false;

    return true;
  }

  protected override bool IsPreGameCompleted() {
    if (!base.IsPreGameCompleted())
      return false;
    return PropsAreCorrect();
  }

  bool PropsAreCorrect() {
    if (goldExtractingObject == null)
      return false;
    if (goldCollectingObject == null)
      return false;
    if (robot.carryingObject == null)
      return false;
    if ((int)robot.carryingObject != goldExtractingObject)
      return false;
    
    return true;
  }

  protected override bool IsGameOver() {
    //if(base.IsGameOver()) return true;
    if (levelData.maxPlayTime > 0f && totalActiveTime >= levelData.maxPlayTime + bonusTime)
      return true;

    //game specific end conditions...
    return false;
  }

  void EnableAudioLocator(bool on) {
    audioLocatorEnabled = on;
  }

  void UpdateLocatorSound(float current_rate) {
    if (audioLocatorEnabled) {
      float timeSinceLast = Time.realtimeSinceStartup - lastPlayTime;

      if (timeSinceLast >= current_rate) {
        AudioManager.PlayAudioClip(locatorBeep);

        lastPlayTime = Time.realtimeSinceStartup;
      }
    }
  }

  [SerializeField]
  public int testRate = 300;

  void EnterPlayState(PlayState new_state) {
    ExitPlayState(playState);
    playStateTimer = 0;
    playState = new_state;
    switch (playState) {
    case PlayState.IDLE:
      BuryTreasure();
      break;
    case PlayState.SEARCHING:
      if (!buriedLocations.ContainsKey(robot.carryingObject)) {
        BuryTreasure();
      }
      robot.SetHeadAngle();
      AudioManager.PlayAudioClipDeferred(findEnergy, 0f, AudioManager.Source.Notification);
      break;
    case PlayState.CAN_EXTRACT:
      if (PlayerPrefs.GetInt("EnergyHuntAutoCollect", 0) == 1) {
        // auto extract
        EnterPlayState(PlayState.EXTRACTING);
      }
      else if (goldExtractingObject != null) {
        goldExtractingObject.SetLEDs(EXTRACTOR_COLOR, 0, 0xFF, 188, 187);
        AudioManager.PlayAudioClipLooping(foundBeep, 0f, AudioManager.Source.Gameplay);
      }
      break;
    case PlayState.EXTRACTING:
      StartCoroutine(StartExtracting());
      break;
    case PlayState.READY_TO_RETURN:
      //hintMessage.ShowMessage("Find the energy!", Color.black);
      break;
    case PlayState.RETURNING:
      robot.SetHeadAngle();
      goldExtractingObject.SetLEDs(EXTRACTOR_COLOR, 0, 0xFF); 
      AudioManager.PlayAudioClip(dropEnergy, 0f, AudioManager.Source.Notification);

      //hintMessage.ShowMessageForDuration("Drop the energy at the transformer", 3.0f, Color.black);
      break;
    case PlayState.RETURNED:
      robot.SetHeadAngle();
      //ActionButton.DROP = "COLLECT";
      //RobotEngineManager.instance.SuccessOrFailure += CheckForGoldDropOff;

      if (PlayerPrefs.GetInt("EnergyHuntAutoCollect", 0) == 1) {
        // auto deposit
        EnterPlayState(PlayState.DEPOSITING);
      }
      else if (goldExtractingObject != null) {
        goldExtractingObject.SetLEDs(EXTRACTOR_COLOR, 0, 0xFF, 188, 187); 
        AudioManager.PlayAudioClipLooping(foundBeep, 0f, AudioManager.Source.Gameplay);
      }
      //hintMessage.ShowMessage("Deposit the energy!", Color.black);
      break;
    case PlayState.DEPOSITING:
      StartCoroutine(AwardPoints());
      break;
    case PlayState.AUTO_DEPOSITING:
      StartCoroutine(AutoDeposit());
      break;
    default:
      break;
    }
  }

  void ExitPlayState(PlayState new_state) {
    switch (playState) {
    case PlayState.IDLE:
      break;
    case PlayState.SEARCHING:
      break;
    case PlayState.EXTRACTING:

      //hintMessage.KillMessage();
      break;
    case PlayState.DEPOSITING:
      robot.isBusy = false;
      break;
    case PlayState.READY_TO_RETURN:
      //hintMessage.KillMessage();
      break;
    case PlayState.RETURNING:
      //ActionButton.DROP = null;
      break;
    case PlayState.RETURNED:
      //RobotEngineManager.instance.SuccessOrFailure -= CheckForGoldDropOff;
      //hintMessage.KillMessage();
      AudioManager.Stop(AudioManager.Source.Gameplay);
      break;
    case PlayState.CAN_EXTRACT:
      AudioManager.Stop(AudioManager.Source.Gameplay);
      break;
    default:
      break;
    }
  }

  void PickedUpActiveBlock(bool success, RobotActionType action_type) {
    if (success
        && (action_type == RobotActionType.PICKUP_OBJECT_HIGH || action_type == RobotActionType.PICKUP_OBJECT_LOW)
        && robot.carryingObject.isActive) {
      // turn on  the block's lights
      goldExtractingObject = robot.carryingObject as ActiveBlock;
      goldExtractingObject.SetLEDs(EXTRACTOR_COLOR);
      cozmoForwardWhenPickUp = robot.Forward;
    }
  }

  void CheckForAutoDepositFailure(bool success, RobotActionType action_type) {
    if (!success && playState == PlayState.AUTO_DEPOSITING && action_type == RobotActionType.DRIVE_TO_POSE) {
      // failed our autodeposit, try again
      SendRobotToCollector();
      DAS.Warn("GoldRushController", "Robot failed to drivetopose during auto-deposit");
    }
  }

  void UpdateSearching() {
    if (robot.Status(RobotStatusFlagClad.CarryingBlock) && robot.carryingObject != null) {
      lastCarriedObjectId = robot.carryingObject;
      Vector2 buriedLocation;
      if (buriedLocations.TryGetValue(robot.carryingObject, out buriedLocation)) {
        UpdateDirectionLights(buriedLocation);
        Vector2 collector_pos = (Vector2)robot.WorldPosition + (Vector2)robot.Forward * CozmoUtil.CARRIED_OBJECT_HORIZONTAL_OFFSET;
        float distance = (buriedLocation - collector_pos).magnitude;
        //Debug.Log ("distance: "+ distance +", robot.carryingObject.WorldPosition: " + robot.carryingObject.WorldPosition);
        if (!foundItems.Contains(robot.carryingObject)) {
          if (distance <= findRadius) {
            //show 'found' light pattern
            if (audioLocatorEnabled) {

              EnterPlayState(PlayState.CAN_EXTRACT);
            }
            foundItems.Add(robot.carryingObject);
            DAS.Debug("GoldRushController", "found!");
          }
          else if (distance <= detectRadius) {
            //float warmthFactor = Mathf.Clamp01((distance - findRadius) / (detectRadius - findRadius));
            //show 'warmer' light pattern to indicate proximity 
            float dist_percent = 1 - ((detectRadius - findRadius) - (distance - findRadius)) / (detectRadius - findRadius);
            float current_rate = Mathf.Lerp(detectRangeDelayClose, detectRangeDelayFar, dist_percent);
            UpdateLocatorSound(current_rate);
            //UpdateDetectorLights(1-dist_percent);

            //hintMessage.KillMessage();
          }
        }
        else if (distance <= findRadius) {
          EnterPlayState(PlayState.CAN_EXTRACT);
          SetAllDetectorLights(1);
        }
        else if (foundItems.Contains(robot.carryingObject) && distance > findRadius) {
          // remove it from our found list if we exit the find radius without dropping it
          foundItems.Remove(robot.carryingObject);
          //hintMessage.KillMessage();
        }
      }
      
    }

    totalActiveTime += Time.deltaTime;
  }

  void UpdateIdle() {
    if (robot.Status(RobotStatusFlagClad.CarryingBlock)) {
      EnterPlayState(PlayState.SEARCHING);
    }
  }

  void UpdateExtracting() {
    //hintMessage.ShowMessage ("Extracting: " + ((int)(100 * (playStateTimer / extractionTime))).ToString () + "%", Color.black);
  }

  void UpdateReadyToReturn() {
    if (robot.Status(RobotStatusFlagClad.CarryingBlock)) {
      EnterPlayState(PlayState.RETURNING);
    }
  }

  void UpdateReturning(bool auto_returning = false) {
    robot.SetHeadAngle();
    Vector2 home_base_pos = Vector2.zero;
    if (goldCollectingObject != null && robot.knownObjects.Find(x => x == goldCollectingObject) != null) {
      home_base_pos = robot.knownObjects.Find(x => x == goldCollectingObject).WorldPosition;
    }
    Vector2 collector_pos = (Vector2)robot.WorldPosition + (Vector2)robot.Forward * CozmoUtil.CARRIED_OBJECT_HORIZONTAL_OFFSET;
    float distance = (home_base_pos - collector_pos).magnitude;
    //Debug.Log ("distance: " + distance);
    float return_distance = auto_returning ? autoReturnRadius : returnRadius;
    if (distance < return_distance) {
      EnterPlayState(PlayState.RETURNED);
    }
    else if (!auto_returning) {
      float dist_percent = 1 - ((detectRadius - returnRadius) - (distance - returnRadius)) / (detectRadius - returnRadius);
      float current_rate = Mathf.Lerp(detectRangeDelayClose, detectRangeDelayFar, dist_percent);
      UpdateLocatorSound(current_rate);
    }
      
    totalActiveTime += Time.deltaTime;
  }

  void UpdateReturned() {
    Vector2 home_base_pos = Vector2.zero;
    if (goldCollectingObject != null && robot.knownObjects.Find(x => x == goldCollectingObject) != null) {
      home_base_pos = robot.knownObjects.Find(x => x == goldCollectingObject).WorldPosition;
      DAS.Debug("GoldRushController", "home_base_pos: " + home_base_pos.ToString());
    }
    Vector2 collector_pos = (Vector2)robot.WorldPosition + (Vector2)robot.Forward * CozmoUtil.CARRIED_OBJECT_HORIZONTAL_OFFSET;
    float distance = (home_base_pos - collector_pos).magnitude;

    if (distance > returnRadius) {
      EnterPlayState(PlayState.RETURNING);
    }
  }

  void UpdateCanExtract() {
    Vector2 buriedLocation;

    if (buriedLocations.TryGetValue(robot.carryingObject, out buriedLocation)) {  
      Vector2 collector_pos = (Vector2)robot.WorldPosition + (Vector2)robot.Forward * CozmoUtil.CARRIED_OBJECT_HORIZONTAL_OFFSET;
      float distance = (buriedLocation - collector_pos).magnitude;

      if (distance > lostRadius) {
        // go back to searching
        EnterPlayState(PlayState.SEARCHING);
      }
    }
  }

  public void BeginExtracting() {
    // start extracting
    AudioManager.Stop(AudioManager.Source.Gameplay);
    foundItems.Remove(lastCarriedObjectId);
    //goldExtractingObjectId = lastCarriedObjectId;
    lastCarriedObjectId = -1;
    //hintMessage.KillMessage();
    EnterPlayState(PlayState.EXTRACTING);
  }

  public void BeginDepositing() {
    EnterPlayState(PlayState.DEPOSITING);
  }

  public void BeginAutoDepositing() {
    EnterPlayState(PlayState.AUTO_DEPOSITING);
  }

  #region helpers

  void BuryTreasure() {
    foundItems.Clear();
    buriedLocations.Clear();
    
    Vector2 randomSpot = UnityEngine.Random.insideUnitCircle;
    randomSpot *= hideRadius;
    Vector2 depositSpot = (Vector2)goldCollectingObject.WorldPosition;
    
    while (Vector2.Distance(randomSpot, depositSpot) < returnRadius) {
      //keep searching until we find a spot far enough away from the deposit spot
      randomSpot = UnityEngine.Random.insideUnitCircle;
      randomSpot *= hideRadius;
    }
    buriedLocations[robot.carryingObject] = randomSpot;

    Vector3 spotZ = (Vector3)randomSpot + Vector3.forward * CozmoUtil.BLOCK_LENGTH_MM * 10f;
    Vector3 spotY1 = (Vector3)randomSpot - Vector3.up * CozmoUtil.BLOCK_LENGTH_MM;
    Vector3 spotY2 = (Vector3)randomSpot + Vector3.up * CozmoUtil.BLOCK_LENGTH_MM;
    Vector3 spotX1 = (Vector3)randomSpot - Vector3.right * CozmoUtil.BLOCK_LENGTH_MM;
    Vector3 spotX2 = (Vector3)randomSpot + Vector3.right * CozmoUtil.BLOCK_LENGTH_MM;
    RobotEngineManager.instance.VisualizeQuad(21, CozmoPalette.ColorToUInt(Color.blue), randomSpot, randomSpot, spotZ, spotZ);
    RobotEngineManager.instance.VisualizeQuad(22, CozmoPalette.ColorToUInt(Color.blue), spotY1, spotY1, spotY2, spotY2);
    RobotEngineManager.instance.VisualizeQuad(23, CozmoPalette.ColorToUInt(Color.blue), spotX1, spotX1, spotX2, spotX2);
  }

  void SendRobotToCollector() {
    robot.GotoObject(goldCollectingObject, 50);
  }

  #endregion

  #region IEnumerator


  [SerializeField]
  protected float extractTrimTime = 0.1f;
  [SerializeField]
  protected float accelStabilizationTime = 0.1f;

  IEnumerator StartExtracting() {

    robot.isBusy = true;

    if (goldExtractingObject != null)
      goldExtractingObject.SetLEDs(CozmoPalette.ColorToUInt(Color.clear), CozmoPalette.ColorToUInt(detectorColor), 0xff, 0, Robot.Light.FOREVER, 0, (uint)(extractingEnergy.length * 1000), 0);
    AudioManager.PlayAudioClip(extractingEnergy, 0f, AudioManager.Source.Notification);
    yield return new WaitForSeconds(extractingEnergy.length - extractTrimTime);
    robot.isBusy = false;
    yield return new WaitForSeconds(extractTrimTime);
    
    EnterPlayState(PlayState.RETURNING);
    
  }

  [SerializeField]
  public float depositTrimTime = 0.1f;

  IEnumerator AwardPoints() {
    // will end up doing active block light stuff here
    robot.isBusy = true;
    uint color = EXTRACTOR_COLOR;
    AudioManager.PlayAudioClip(depositingEnergy, 0f, AudioManager.Source.Notification);
    if (goldExtractingObject != null)
      goldExtractingObject.SetLEDs(CozmoPalette.ColorToUInt(detectorColor), CozmoPalette.ColorToUInt(Color.clear), 0xff, 0, Robot.Light.FOREVER, 0, (uint)(depositingEnergy.length * 1000), 0);
    yield return new WaitForSeconds(depositingEnergy.length - depositTrimTime);

    if (goldExtractingObject != null)
      goldExtractingObject.SetLEDs(0);
    // PlayNotificationAudio (collectedSound);
    // award points

    robot.isBusy = false;

    yield return new WaitForSeconds(depositTrimTime);


    if (numDrops < scoreSounds.Length) {
      AudioManager.PlayAudioClip(scoreSounds[numDrops], 0f, AudioManager.Source.Notification);
      yield return new WaitForSeconds(scoreSounds[numDrops].length + .05f);
    }
    else {
      AudioManager.PlayAudioClip(scoreSounds[scoreSounds.Length - 1], 0f, AudioManager.Source.Notification);
      yield return new WaitForSeconds(scoreSounds[scoreSounds.Length - 1].length + .05f);
    }

    numDrops++;
    score += 10 * numDrops;

    if (score < 2760) {
      AudioManager.PlayAudioClip(points, 0f, AudioManager.Source.Notification);
      yield return new WaitForSeconds(points.length + .05f);
    }

    if (score > oldHighScore && !setHighScore) {
      setHighScore = true;
      AudioManager.PlayAudioClip(newHighScore, 0f, AudioManager.Source.Notification);

      yield return new WaitForSeconds(newHighScore.length);
    }


    int num_drops_this_run = numDrops % numDropsForBonusTime;
    if (num_drops_this_run == 0) {
      // award time, clear robot lights
      int num_bonuses_awarded = (numDrops / numDropsForBonusTime) - 1;
      float awardedTime = num_bonuses_awarded == 0 ? baseTimeBonus : baseTimeBonus * Mathf.Pow(bonusTimeDecay, (float)num_bonuses_awarded);
      bonusTime += awardedTime;
      //ResetTimerIndex(totalActiveTime);
      SetEnergyBars(numDropsForBonusTime, color);

    }
    else {
      // set the robot lights
      SetEnergyBars(num_drops_this_run, color);
    }


    if (num_drops_this_run == 0) {
      AudioManager.PlayAudioClip(timeExtension, 0f, AudioManager.Source.Notification);
      EnterPlayState(PlayState.IDLE);
      yield return new WaitForSeconds(timeExtension.length / 2);
      SetEnergyBars(0, 0);
    }
    else {
      EnterPlayState(PlayState.IDLE);
    }

  }

  IEnumerator AutoDeposit() {
    // first need to get in range
    SendRobotToCollector();

    while (!inDepositRange) {
      UpdateReturning(true);
      yield return 0;
    }

    robot.CancelAction(RobotActionType.DRIVE_TO_OBJECT);
    EnterPlayState(PlayState.DEPOSITING);


  }

  #endregion

  #region Active Block IFC

  void SetAllDetectorLights(float light_intensity) {
    for (int i = 0; i < 4; i++) {
      byte which_leds = (byte)ActiveBlock.Light.IndexToPosition(i);
      Color col = Color.Lerp(Color.clear, detectorColor, light_intensity);
      goldExtractingObject.SetLEDs(CozmoPalette.ColorToUInt(col), 0, which_leds, Robot.Light.FOREVER, 0, 0, 0, 0);
    }
  }

  Color detectorColor = Color.cyan;
  float[] lightWeights = new float[4];

  void UpdateDirectionLights(Vector2 target_position) {
    if (goldExtractingObject == null)
      return;

    Vector2 collector_pos = (Vector2)robot.WorldPosition + (Vector2)robot.Forward * CozmoUtil.CARRIED_OBJECT_HORIZONTAL_OFFSET;
    ;

    Vector2 to_target = target_position - (Vector2)collector_pos;
    to_target = to_target.normalized;

    float angleDelta = MathUtil.SignedVectorAngle(cozmoForwardWhenPickUp, robot.Forward, Vector3.forward);

    Vector3 TopNorth = Quaternion.AngleAxis(angleDelta, Vector3.forward) * goldExtractingObject.TopNorth;
    Vector3 TopEast = Quaternion.AngleAxis(angleDelta, Vector3.forward) * goldExtractingObject.TopEast;

    lightWeights[0] = Mathf.Clamp01(Vector2.Dot(to_target, TopEast));
    lightWeights[1] = Mathf.Clamp01(Vector2.Dot(to_target, TopNorth));
    lightWeights[2] = Mathf.Clamp01(Vector2.Dot(to_target, -TopEast));
    lightWeights[3] = Mathf.Clamp01(Vector2.Dot(to_target, -TopNorth));
    
    for (int i = 0; i < lightWeights.Length; i++) {
      byte which_leds = (byte)ActiveBlock.Light.IndexToPosition(i);
      Color col = Color.Lerp(Color.clear, detectorColor, lightWeights[i]);
      goldExtractingObject.SetLEDs(CozmoPalette.ColorToUInt(col), 0, which_leds, Robot.Light.FOREVER, 0, 0, 0, 0);
    }

  }

  void SetEnergyBars(int num_bars, uint color = 0) {
    for (int i = 0; i < robot.lights.Length; ++i) {
      if (i < num_bars) {
        robot.lights[i].onColor = color;
      }
      else {
        robot.lights[i].onColor = 0;
      }
      robot.lights[i].offColor = 0;
      robot.lights[i].onPeriod_ms = 1000;
      robot.lights[i].offPeriod_ms = 0;
      robot.lights[i].transitionOnPeriod_ms = 0;
      robot.lights[i].transitionOffPeriod_ms = 0;
    }
  }

  #endregion
}
