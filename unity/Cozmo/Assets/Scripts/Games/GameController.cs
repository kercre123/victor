using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;

public class GameController : MonoBehaviour {

	public enum GameState {
		BUILDING,
		PRE_GAME,
		PLAYING,
		RESULTS,
		Count
	}

	[Serializable]
	public struct TimerAudio {
		public int time;
		public AudioClip sound;
	}

	public const int STAR_COUNT = 3;

	[SerializeField] private Text countdownText = null;
	[SerializeField] private float countdownToStart = 0f;
	[SerializeField] private AudioClip countdownTickSound;
	//[SerializeField] protected float maxPlayTime = 0f;
	[SerializeField] protected TimerAudio[] timerSounds;
	[SerializeField] private AudioClip gameStartingIn;
	//[SerializeField] protected int scoreToWin = 0;
	//[SerializeField] protected int[] starThresholds = new int[STAR_COUNT];
	[SerializeField] private AudioClip starPop;
	[SerializeField] protected Text textScore = null;
	[SerializeField] protected Text textError = null;
	[SerializeField] protected Text textState = null;
	[SerializeField] protected Text textTime = null;
	[SerializeField] protected Text textAddaboy = null;
	[SerializeField] private bool autoPlay = false;
	[SerializeField] private Button playButton = null;
	[SerializeField] protected string buildInstructionsLayoutFilter = null;
	[SerializeField] private Image resultsPanel = null;
	[SerializeField] protected AudioClip instructionsSound;
	[SerializeField] private AudioClip gameStartSound;
	[SerializeField] protected AudioClip playerScoreSound;
	[SerializeField] private AudioClip playingLoopSound;
	[SerializeField] protected GameActions[] stateActions = new GameActions[(int)GameState.Count];
	[SerializeField] private AudioClip[] winSounds = new AudioClip[STAR_COUNT+1];
	[SerializeField] private AudioClip[] winLoopSounds = new AudioClip[STAR_COUNT+1];
	[SerializeField] private AudioClip loseSound;
	[SerializeField] private AudioClip loseLoopSound;
	[SerializeField] private Image[] starImages = new Image[STAR_COUNT];
	[SerializeField] private float starPopInTime = .5f;
	[SerializeField] private float maxStarPop = 1.5f;
	[SerializeField] private float minStarPop = 0.3f;

	private AudioClip gameOverSound { get { if(win) return stars < winSounds.Length ? winSounds[stars] : null; return loseSound; } }
	private AudioClip resultsLoopSound { get { if(win) return stars < winLoopSounds.Length ? winLoopSounds[stars] : null; return loseLoopSound; } }

	protected bool playRequested = false;
	protected bool buildRequested = false;

	[System.NonSerialized] public GameState state = GameState.BUILDING;
	protected float stateTimer = 0f;
	protected bool countdownAnnounced = false;
	protected float lastPlayTime = 0;
	protected int timerEventIndex = 0;
	protected float bonusTime = 0; // bonus time is awarded each time the player numDropsForBonusTime drop offs 

	protected int score = 0;
	protected int stars = 0;
	protected bool win = false;

	protected string currentGameName { get; private set; }
	protected int currentLevelNumber { get; private set; }

	public const string STARS_END =  "_stars";
	public string SAVED_STARS { get { return currentGameName + currentLevelNumber + STARS_END; } }
	public int savedStars { get { return PlayerPrefs.GetInt(SAVED_STARS, 0); } set { PlayerPrefs.SetInt(SAVED_STARS, value); } }

	protected bool firstFrame = true;

	protected float errorMsgTimer = 0f;

	protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	//public float gameStartingInDelay = 1.3f;

	private float gameStartingInDelay { get { return gameStartingIn != null ? gameStartingIn.length + 0.1f : 0f; } }
	private float instructionsDelay { get { return instructionsSound != null ? instructionsSound.length + 0.5f : 0f; } }

	protected int lastTimerSeconds = 0;
	protected float coundownTimer = 0f;

	protected static float _MessageDelay = 0.5f;
	public static float MessageDelay { get { return _MessageDelay; } protected set { _MessageDelay = value; } }

	protected GameData.LevelData levelData { get; private set; }

	protected virtual void Awake()
	{
		for(int i=0; i<stateActions.Length; i++) {
			if(stateActions[i] == null) continue;
			stateActions[i].gameObject.SetActive(false);
		}
	}

	protected virtual void OnEnable () {
		state = GameState.BUILDING;
		stateTimer = 0f;
		errorMsgTimer = 0f;
		firstFrame = true;
		playRequested = false;
		buildRequested = false;

		currentGameName = PlayerPrefs.GetString("CurrentGame", "Unknown");
		currentLevelNumber = PlayerPrefs.GetInt(currentGameName + "_CurrentLevel", 1);

		if(GameData.instance != null)
		{
			if(GameData.instance.levelData.ContainsKey(currentGameName + currentLevelNumber))
			{
				levelData = GameData.instance.levelData[currentGameName + currentLevelNumber];
			}
			else
			{
				Debug.LogError("GameData does not contain level: " + currentGameName + currentLevelNumber);
			}
		}
		else
		{
			Debug.LogError("GameData is not in scene");
		}

		if(textError != null) textError.gameObject.SetActive(false);
		if(playButton != null) playButton.gameObject.SetActive(false);
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
		if(countdownText != null) countdownText.gameObject.SetActive(false);
		if(textScore != null) textScore.gameObject.SetActive(false);
		if(robot != null) robot.ClearData();
	}

	void Update () {
		stateTimer += Time.deltaTime;

		//force our initial state to enter in case there is set up code there
		if(firstFrame) {
			EnterState();
		}
		else {

			GameState nextState = GetNextState();

			if(nextState != state) {
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

	protected virtual void OnDisable () {
		if(textError != null) textError.gameObject.SetActive(false);
		if(playButton != null) playButton.gameObject.SetActive(false);
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
		AudioManager.Stop(); // stop all sounds
		if(robot != null) robot.ClearData();
	}

	GameState GetNextState() {

		//consider switching states
		switch(state) {
			case GameState.BUILDING:
				if((playRequested || autoPlay) && IsGameReady()) return GameState.PRE_GAME; 
				break;
			case GameState.PRE_GAME:
				if(IsPreGameCompleted()) return GameState.PLAYING; 
				break;
			case GameState.PLAYING:
				if(buildRequested) return GameState.BUILDING;
				if(IsGameOver()) return GameState.RESULTS;
				break;
			case GameState.RESULTS:
				if(buildRequested) return GameState.BUILDING;
				if(playRequested && IsGameReady()) return GameState.PRE_GAME;
				break;
		}

		//stay in current state
		return state;
	}

	void EnterState() {
		stateTimer = 0f;

		switch(state) {
			case GameState.BUILDING: 	Enter_BUILDING(); break;
			case GameState.PRE_GAME: 	Enter_PRE_GAME(); break;
			case GameState.PLAYING: 	Enter_PLAYING(); break;
			case GameState.RESULTS: 	Enter_RESULTS(); break;
		}
	}

	void UpdateState() {
		switch(state) {
			case GameState.BUILDING: 	Update_BUILDING(); break;
			case GameState.PRE_GAME: 	Update_PRE_GAME(); break;
			case GameState.PLAYING: 	Update_PLAYING(); break;
			case GameState.RESULTS: 	Update_RESULTS(); break;
		}
	}

	void ExitState() {
		switch(state) {
			case GameState.BUILDING: 	Exit_BUILDING(); break;
			case GameState.PRE_GAME: 	Exit_PRE_GAME(); break;
			case GameState.PLAYING: 	Exit_PLAYING(); break;
			case GameState.RESULTS: 	Exit_RESULTS(); break;
		}
	}

	void RefreshGameActionsForState() {
		GameActions currentActions = stateActions[(int)state];
		
		for(int i=0;i<stateActions.Length;i++) {
			if(stateActions[i] != null && currentActions != stateActions[i]) stateActions[i].gameObject.SetActive(false);
		}
		
		if(currentActions != null) currentActions.gameObject.SetActive(true);
	}

	protected const string suffix_seconds = "s";

	protected virtual void RefreshHUD() {
		if(textScore != null) {
			textScore.text = score.ToString();
		}

		if(textState != null) {
			textState.text = state.ToString();
		}

		if (textTime != null && state == GameState.PLAYING) {
			if (levelData.maxPlayTime > 0f) {
				textTime.text = Mathf.CeilToInt (levelData.maxPlayTime - stateTimer).ToString () + suffix_seconds;
			} else {
				textTime.text = Mathf.CeilToInt (stateTimer).ToString () + suffix_seconds;
			}
		} else {
			textTime.text = string.Empty;
		}

		if(state != GameState.PLAYING && playRequested && !IsGameReady()) {
			if(textError != null) {
				//set specific text once we have analyzer
				textError.gameObject.SetActive(true);
			}
			errorMsgTimer = 5f;
		}
		
		if(errorMsgTimer > 0f) {
			errorMsgTimer -= Time.deltaTime;
			if(errorMsgTimer <= 0f) {
				if(textError != null) {
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

		if(playButton != null) playButton.gameObject.SetActive(true);
		if(robot != null) robot.SetObjectAdditionAndDeletion(true, false);
	}
	protected virtual void Update_BUILDING() {
		//Debug.Log(gameObject.name + " Update_BUILDING");
	}
	protected virtual void Exit_BUILDING() {
		//Debug.Log(gameObject.name + " Exit_BUILDING");

		if(playButton != null) playButton.gameObject.SetActive(false);
		if(robot != null) robot.SetObjectAdditionAndDeletion(false, false);
	}
		
	protected virtual void Enter_PRE_GAME() {
		//Debug.Log(gameObject.name + " Enter_PRE_GAME");

		if(countdownText != null) {
			countdownText.gameObject.SetActive(false);
		}

		coundownTimer = 0f;
		countdownAnnounced = false;

		//if(robot != null) robot.SetObjectAdditionAndDeletion(false, false);
	}

	protected virtual void Update_PRE_GAME() {
		//Debug.Log(gameObject.name + " Update_PRE_GAME");
		//add game specific intro messaging in an override of this method
		// for instance, if this game requires a certain action to start, tell them here: 'pick up X to begin!'
		//	or if this game has a count-down before start, we display that count down here
		if(countdownToStart > 0f) {

			if(coundownTimer == 0f) {
				if(instructionsSound != null) AudioManager.PlayOneShot(instructionsSound);
				//Debug.Log("gameStartingIn stateTimer("+stateTimer+")");
				if(gameStartingIn != null) AudioManager.PlayAudioClip(gameStartingIn, instructionsDelay, AudioManager.Source.Notification);
				lastTimerSeconds = 0;
				countdownAnnounced = true;
			}

			coundownTimer += Time.deltaTime;

			if(coundownTimer >= gameStartingInDelay + instructionsDelay) {

				int remaining = Mathf.CeilToInt( Mathf.Clamp((countdownToStart + gameStartingInDelay + instructionsDelay) - coundownTimer, 0f, countdownToStart) );

				//Debug.Log("PlayCountdownAudio stateTimer("+stateTimer+")");
				PlayCountdownAudio(remaining);
				
				if(countdownText != null) {
					countdownText.text = remaining.ToString();
					countdownText.gameObject.SetActive(true);
				}
			}
		}
	}

	protected virtual void Exit_PRE_GAME() {
		//Debug.Log(gameObject.name + " Exit_PRE_GAME");
		if(countdownText != null) {
			countdownText.gameObject.SetActive(false);
		}
	}

	protected virtual void Enter_PLAYING() {

		timerEventIndex = 0;
		bonusTime = 0;

		//Debug.Log(gameObject.name + " Enter_PLAYING");
		if(gameStartSound != null) AudioManager.PlayOneShot(gameStartSound);

		if(textScore != null) textScore.gameObject.SetActive(true);
		if(textError != null) textError.gameObject.SetActive(false);

		score = 0;
	}

	protected virtual void Update_PLAYING() {
		//Debug.Log(gameObject.name + " Update_PLAYING");
	}
	protected virtual void Exit_PLAYING(bool overrideStars = false) {
		win = false;
		
		if(levelData.scoreToWin > 0) {
			win = score >= levelData.scoreToWin;
		}
		else {
			win = true;
		}

		if(!overrideStars) {
			stars = 0;
			int[] starRequirements = levelData.stars;
			for(int i = 0; i < starRequirements.Length; ++i) {
				if(score >= starRequirements[i] && starRequirements[i] > 0) stars = i + 1;
			}

			if(stars >= savedStars) savedStars = stars;
		}

		//Debug.Log(gameObject.name + " Exit_PLAYING");
		if(textScore != null) textScore.gameObject.SetActive(false);

		if(gameOverSound != null) AudioManager.PlayOneShot(gameOverSound);
	}

	protected virtual void Enter_RESULTS() {

		//Debug.Log(gameObject.name + " Enter_RESULTS");
		StartCoroutine(PopInStars());
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(true);
		if(textScore != null) textScore.gameObject.SetActive(true);
		if(resultsLoopSound != null) AudioManager.PlayAudioClipLooping(resultsLoopSound, gameOverSound != null ? gameOverSound.length + 0.5f : 0.5f, AudioManager.Source.Gameplay);
		if(textAddaboy != null) {
			switch(stars) {
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

		if( robot != null ) {
			robot.isBusy = true;
		}
	}

	protected virtual void Update_RESULTS() {
		//Debug.Log(gameObject.name + " Update_RESULTS");
	}
	protected virtual void Exit_RESULTS() {
		//Debug.Log(gameObject.name + " Exit_RESULTS");

		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
		if(textScore != null) textScore.gameObject.SetActive(false);
		AudioManager.Stop(resultsLoopSound);
		if(robot != null) {
			robot.TurnOffAllLights();
			robot.isBusy = true;
		}
	}

	protected virtual bool IsGameReady() {
		if(robot == null) return false;
		if(GameLayoutTracker.instance == null) return false;

		//layout tracker dominates unless it is disabled
		return GameLayoutTracker.instance.Phase == LayoutTrackerPhase.DISABLED;
	}

	protected virtual bool IsPreGameCompleted() {
		if(robot == null) return false;
		if(GameLayoutTracker.instance == null) return false;

		//add game specific gating in an override of this method
		// for instance, if this game requires a certain action to start: 'pick up X to begin!'
		//	or if this game has a count-down before start, handle that check here
		if(countdownToStart > 0f && ((countdownToStart + gameStartingInDelay + instructionsDelay) - coundownTimer) > 0f) return false;

		return true;
	}

	protected virtual bool IsGameOver() {
		if(levelData.maxPlayTime > 0f && stateTimer >= levelData.maxPlayTime) return true;
		if(levelData.scoreToWin > 0) {
			if(score >= levelData.scoreToWin) {
				return true;
			}
		}
		
		return false;
	}

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

	protected void PlayCountdownAudio(int secondsLeft) {
		bool played = false;
		for(int i=0; i<timerSounds.Length; i++) {
			if(secondsLeft == timerSounds[i].time && lastTimerSeconds != timerSounds[i].time) {
				AudioManager.PlayAudioClipDeferred(timerSounds[i].sound, 0, AudioManager.Source.Notification);
				played = true;
				break;
			}
		}

		if(!played && lastTimerSeconds != secondsLeft && countdownTickSound != null) AudioManager.PlayOneShot(countdownTickSound);
		
		lastTimerSeconds = secondsLeft;
	}

	IEnumerator PopInStars()
	{
		int num_pops = 0;
		float lastPopTime = -1;

		for(int i = 0; i < starImages.Length; ++i) 
		{
			if(starImages[i] != null) starImages[i].gameObject.SetActive(false);
		}

		
		while(num_pops < stars + 1)
		{
			if( lastPopTime + starPopInTime < Time.realtimeSinceStartup )
			{
				// time to pop in a star
				if( num_pops < starImages.Length )
				{
					starImages[num_pops].gameObject.SetActive(win && num_pops < stars);
					starImages[num_pops].transform.localScale = new Vector3(minStarPop, minStarPop, 0);
					if( win && num_pops < stars )
					{
						AudioManager.PlayOneShot(starPop);
					}
				}
				lastPopTime = Time.realtimeSinceStartup;
				num_pops++;
			}
			else
			{
				// lerp it in three phases:
				/// 1) from minStarPop to maxStarPop
				/// 2) from maxStarPop back to 1
				float pop_time = Time.realtimeSinceStartup - lastPopTime;
				float scale = 1;
				if( pop_time < starPopInTime/2 )
				{
					// first half
					scale = Mathf.Lerp(minStarPop, maxStarPop, pop_time/(starPopInTime/2));
				}
				else
				{
					// second half
					scale = Mathf.Lerp(maxStarPop, 1, (pop_time-(starPopInTime/2))/(starPopInTime/2));
				}
				starImages[num_pops-1].transform.localScale = new Vector3(scale,scale,0);

			}
			yield return 0;
		}
		
		yield return 0;
	}


}
