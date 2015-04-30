using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;

public class GameController : MonoBehaviour {

	public enum GameState {
		BUILDING,
		PRE_GAME,
		PLAYING,
		RESULTS
	}

	[Serializable]
	public class TimerAudio {
		public int time = 0;
		public AudioClip sound = null;
	}

	[SerializeField] protected Text countdownText = null;
	[SerializeField] protected float countdownToStart = 0f;
	[SerializeField] protected AudioClip countdownTickSound;
	[SerializeField] protected float maxPlayTime = 0f;
	[SerializeField] protected TimerAudio[] timerSounds;
	[SerializeField] protected AudioSource notificationAudio;
	[SerializeField] protected AudioClip gameStartingIn;
	[SerializeField] protected int scoreToWin = 0;
	[SerializeField] protected int numPlayers = 1;
	[SerializeField] protected Text textScore = null;
	[SerializeField] protected Text textError = null;
	[SerializeField] protected Text textState = null;
	[SerializeField] protected Text textTime = null;
	[SerializeField] protected bool autoPlay = false;
	[SerializeField] protected Button playButton = null;
	[SerializeField] protected string buildInstructionsLayoutFilter = null;
	[SerializeField] protected Image resultsPanel = null;
	[SerializeField] protected AudioClip gameStartSound;
	[SerializeField] protected AudioClip playerScoreSound;
	[SerializeField] protected AudioClip playingLoopSound;
	[SerializeField] protected AudioClip gameOverSound;

	protected bool playRequested = false;
	protected bool buildRequested = false;

	internal GameState state = GameState.BUILDING;
	protected float stateTimer = 0f;
	protected int currentCountdown = 0;

	protected float lastPlayTime = 0;
	protected int timerEventIndex = 0;
	protected float bonusTime = 0; // bonus time is awarded each time the player numDropsForBonusTime drop offs 

	protected int[] scores;

	protected int winnerIndex;
	protected bool firstFrame = true;

	protected float errorMsgTimer = 0f;

	protected Robot robot;

	//public float gameStartingInDelay = 1.3f;

	public float gameStartingInDelay {
		get {
			return gameStartingIn != null ? gameStartingIn.length : 0f;
		}
	}

	protected int lastTimerSeconds = 0;
	protected float coundownTimer = 0f;

	protected static float _MessageDelay = 0.5f;
	public static float MessageDelay { get { return _MessageDelay; } protected set { _MessageDelay = value; } }

	protected virtual void OnEnable () {
		state = GameState.BUILDING;
		stateTimer = 0f;
		scores = new int[numPlayers];
		winnerIndex = -1;
		errorMsgTimer = 0f;
		firstFrame = true;
		playRequested = false;
		buildRequested = false;

		if(textError != null) textError.gameObject.SetActive(false);
		if(playButton != null) playButton.gameObject.SetActive(false);
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
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

		RefreshHUD();
		firstFrame = false;
		playRequested = false;
		buildRequested = false;
	}

	protected virtual void OnDisable () {
		if(textError != null) textError.gameObject.SetActive(false);
		if(playButton != null) playButton.gameObject.SetActive(false);
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
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

	protected virtual void RefreshHUD() {
		if(textScore != null && scores != null && scores.Length > 0) {
			textScore.text = "score: " + scores[0].ToString();

		}

		if(textState != null) {
			textState.text = state.ToString();
		}

		if (textTime != null && state == GameState.PLAYING) {
			if (maxPlayTime > 0f) {
				textTime.text = Mathf.CeilToInt (maxPlayTime - stateTimer).ToString ();
			} else {
				textTime.text = Mathf.CeilToInt (stateTimer).ToString ();
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
		winnerIndex = -1;
		Debug.Log(gameObject.name + " Enter_BUILDING");

		if(playButton != null) playButton.gameObject.SetActive(true);
	}
	protected virtual void Update_BUILDING() {
		//Debug.Log(gameObject.name + " Update_BUILDING");
	}
	protected virtual void Exit_BUILDING() {
		Debug.Log(gameObject.name + " Exit_BUILDING");

		if(playButton != null) playButton.gameObject.SetActive(false);
	}
		
	bool countdownAnnounced = false;
	protected virtual void Enter_PRE_GAME() {
		Debug.Log(gameObject.name + " Enter_PRE_GAME");

		if(countdownText != null) {
			countdownText.gameObject.SetActive(false);
		}

		coundownTimer = countdownToStart;
		countdownAnnounced = false;
	}

	protected virtual void Update_PRE_GAME() {
		//Debug.Log(gameObject.name + " Update_PRE_GAME");
		//add game specific intro messaging in an override of this method
		// for instance, if this game requires a certain action to start, tell them here: 'pick up X to begin!'
		//	or if this game has a count-down before start, we display that count down here

		if(countdownToStart > 0f) {

			if(!countdownAnnounced) {
				if(coundownTimer == countdownToStart) {
					if(gameStartingIn != null && audio != null) audio.PlayOneShot(gameStartingIn);
					lastTimerSeconds = 0;
				}
			}

			countdownAnnounced = true;

			if(stateTimer >= gameStartingInDelay) {

				coundownTimer -= Time.deltaTime;
				int remaining = Mathf.CeilToInt( Mathf.Clamp(coundownTimer, 0f, countdownToStart) );

				PlayCountdownAudio(remaining);
				
				if(countdownText != null) {
					countdownText.text = remaining.ToString();
					currentCountdown = remaining;
					countdownText.gameObject.SetActive(true);
				}
			}

		}
	}

	protected virtual void Exit_PRE_GAME() {
		Debug.Log(gameObject.name + " Exit_PRE_GAME");
		if(countdownText != null) {
			countdownText.gameObject.SetActive(false);
		}
	}

	protected virtual void Enter_PLAYING() {

		timerEventIndex = 0;
		bonusTime = 0;

		Debug.Log(gameObject.name + " Enter_PLAYING");
		if(gameStartSound != null && audio != null) audio.PlayOneShot(gameStartSound);

		if(textScore != null) textScore.gameObject.SetActive(true);
		if(textError != null) textError.gameObject.SetActive(false);


	}

	protected virtual void Update_PLAYING() {
		//Debug.Log(gameObject.name + " Update_PLAYING");
	}
	protected virtual void Exit_PLAYING() {
		Debug.Log(gameObject.name + " Exit_PLAYING");
		if(textScore != null) textScore.gameObject.SetActive(false);

		if(gameOverSound != null && audio != null) audio.PlayOneShot(gameOverSound);
	}

	protected virtual void Enter_RESULTS() {
		Debug.Log(gameObject.name + " Enter_RESULTS");
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(true);
		if(textScore != null) textScore.gameObject.SetActive(true);
	}
	protected virtual void Update_RESULTS() {
		//Debug.Log(gameObject.name + " Update_RESULTS");
	}
	protected virtual void Exit_RESULTS() {
		Debug.Log(gameObject.name + " Exit_RESULTS");

		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
		if(textScore != null) textScore.gameObject.SetActive(false);
	}

	protected virtual bool IsGameReady() {
		if(RobotEngineManager.instance == null) return false;
		if(RobotEngineManager.instance.current == null) return false;
		if(GameLayoutTracker.instance == null) return false;

		return GameLayoutTracker.instance.Validated;
	}

	protected virtual bool IsPreGameCompleted() {
		if(RobotEngineManager.instance == null) return false;
		if(RobotEngineManager.instance.current == null) return false;
		if(GameLayoutTracker.instance == null) return false;

		//add game specific gating in an override of this method
		// for instance, if this game requires a certain action to start: 'pick up X to begin!'
		//	or if this game has a count-down before start, handle that check here
		if(countdownToStart > 0f && coundownTimer > 0f) return false;

		return true;
	}

	protected virtual bool IsGameOver() {
		
		if(maxPlayTime > 0f && stateTimer >= maxPlayTime) return true;
		if(scoreToWin > 0) {
			for(int i=0;i<scores.Length;i++) {
				if(scores[i] >= scoreToWin) {
					winnerIndex = i;
					return true;
				}
			}
		}
		
		return false;
	}

	public void PlayRequested() {
		playRequested = true;
	}

	public void BuildRequested() {
		buildRequested = true;
	}

	protected void PlayCountdownAudio(int secondsLeft) {
		bool played = false;

		for(int i=0; i<timerSounds.Length; i++) {
			if(secondsLeft == timerSounds[i].time && lastTimerSeconds != timerSounds[i].time) {
				// defer to other notifications
				if( !notificationAudio.isPlaying ) {
					notificationAudio.PlayOneShot(timerSounds[i].sound);
				}
				played = true;
				break;
			}
		}

		if(!played && lastTimerSeconds != secondsLeft && countdownTickSound != null && audio != null) audio.PlayOneShot(countdownTickSound);
		
		lastTimerSeconds = secondsLeft;
	}
	
	protected void PlayNotificationAudio(AudioClip clip)
	{
		if (notificationAudio != null) 
		{
			notificationAudio.Stop ();
			Debug.Log ("Should be playing " + clip.name);
			notificationAudio.PlayOneShot (clip);
		}
	}
}
