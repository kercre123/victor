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

	[SerializeField] private Text countdownText = null;
	[SerializeField] private float countdownToStart = 0f;
	[SerializeField] private AudioClip countdownTickSound;
	[SerializeField] protected float maxPlayTime = 0f;
	[SerializeField] protected TimerAudio[] timerSounds;
	[SerializeField] protected AudioSource notificationAudio;
	[SerializeField] private AudioClip gameStartingIn;
	[SerializeField] protected int scoreToWin = 0;
	[SerializeField] protected int[] starThresholds;
	[SerializeField] protected Text textScore = null;
	[SerializeField] protected Text textError = null;
	[SerializeField] protected Text textState = null;
	[SerializeField] protected Text textTime = null;
	[SerializeField] private bool autoPlay = false;
	[SerializeField] private Button playButton = null;
	[SerializeField] protected string buildInstructionsLayoutFilter = null;
	[SerializeField] private Image resultsPanel = null;
	[SerializeField] private AudioClip instructionsSound;
	[SerializeField] private AudioClip gameStartSound;
	[SerializeField] protected AudioClip playerScoreSound;
	[SerializeField] private AudioClip playingLoopSound;
	[SerializeField] protected GameActions[] stateActions = new GameActions[(int)GameState.Count];
	[SerializeField] private AudioClip[] winSounds = new AudioClip[4];
	[SerializeField] private AudioClip[] winLoopSounds = new AudioClip[4];
	[SerializeField] private AudioClip loseSound;
	[SerializeField] private AudioClip loseLoopSound;

	private AudioClip gameOverSound { get { if(win) return stars < winSounds.Length ? winSounds[stars] : null; return loseSound; } }
	private AudioClip resultsLoopSound {  get { if(win) return stars < winLoopSounds.Length ? winLoopSounds[stars] : null; return loseLoopSound; } }

	protected bool playRequested = false;
	protected bool buildRequested = false;

	[System.NonSerialized] public GameState state = GameState.BUILDING;
	protected float stateTimer = 0f;
	protected bool countdownAnnounced = false;
	protected float lastPlayTime = 0;
	protected int timerEventIndex = 0;
	protected float bonusTime = 0; // bonus time is awarded each time the player numDropsForBonusTime drop offs 

	protected int score;
	protected int stars = 0;
	protected bool win = false;
	
	protected bool firstFrame = true;

	protected float errorMsgTimer = 0f;

	protected Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	//public float gameStartingInDelay = 1.3f;

	private float gameStartingInDelay { get { return gameStartingIn != null ? gameStartingIn.length : 0f; } }
	private float instructionsDelay { get { return instructionsSound != null ? instructionsSound.length + 0.5f : 0f; } }

	protected int lastTimerSeconds = 0;
	protected float coundownTimer = 0f;

	protected static float _MessageDelay = 0.5f;
	public static float MessageDelay { get { return _MessageDelay; } protected set { _MessageDelay = value; } }

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
		score = 0;
		errorMsgTimer = 0f;
		firstFrame = true;
		playRequested = false;
		buildRequested = false;

		if(textError != null) textError.gameObject.SetActive(false);
		if(playButton != null) playButton.gameObject.SetActive(false);
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
		if(countdownText != null) countdownText.gameObject.SetActive(false);
		if(textScore != null) textScore.gameObject.SetActive(false);

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
		if( robot != null ) robot.isBusy = false;
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
			if (maxPlayTime > 0f) {
				textTime.text = Mathf.CeilToInt (maxPlayTime - stateTimer).ToString () + suffix_seconds;
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
		
	protected virtual void Enter_PRE_GAME() {
		Debug.Log(gameObject.name + " Enter_PRE_GAME");

		if(countdownText != null) {
			countdownText.gameObject.SetActive(false);
		}

		coundownTimer = 0f;
		countdownAnnounced = false;

		score = 0;
	}

	protected virtual void Update_PRE_GAME() {
		//Debug.Log(gameObject.name + " Update_PRE_GAME");
		//add game specific intro messaging in an override of this method
		// for instance, if this game requires a certain action to start, tell them here: 'pick up X to begin!'
		//	or if this game has a count-down before start, we display that count down here
		if(countdownToStart > 0f) {

			if(coundownTimer == 0f) {
				if(instructionsSound != null) PlayOneShot(instructionsSound);
				//Debug.Log("gameStartingIn stateTimer("+stateTimer+")");
				if(audio != null) {
					if(gameStartingIn != null) PlayDelayed(gameStartingIn, instructionsDelay);
				}
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
		Debug.Log(gameObject.name + " Exit_PRE_GAME");
		if(countdownText != null) {
			countdownText.gameObject.SetActive(false);
		}
	}

	protected virtual void Enter_PLAYING() {

		timerEventIndex = 0;
		bonusTime = 0;

		Debug.Log(gameObject.name + " Enter_PLAYING");
		if(gameStartSound != null) PlayOneShot(gameStartSound);

		if(textScore != null) textScore.gameObject.SetActive(true);
		if(textError != null) textError.gameObject.SetActive(false);

		score = 0;
	}

	protected virtual void Update_PLAYING() {
		//Debug.Log(gameObject.name + " Update_PLAYING");
	}
	protected virtual void Exit_PLAYING() {
		win = false;
		
		if(scoreToWin > 0) {
			win = score >= scoreToWin;
		}
		else {
			win = true;
		}
		
		stars = 0;
		for(int i = 0; i < starThresholds.Length; ++i) {
			if(score >= starThresholds[i]) stars = i + 1;
		}

		Debug.Log(gameObject.name + " Exit_PLAYING");
		if(textScore != null) textScore.gameObject.SetActive(false);

		if(gameOverSound != null) PlayOneShot(gameOverSound);
	}

	protected void PlayOneShot(AudioClip clip, float pitch = 1f) {
		if(audio != null) {
			audio.pitch = pitch;
			audio.PlayOneShot(clip);
		}
	}

	protected virtual void Enter_RESULTS() {
		Debug.Log(gameObject.name + " Enter_RESULTS");
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(true);
		if(textScore != null) textScore.gameObject.SetActive(true);
		if(resultsLoopSound != null && audio != null) {
			PlayDelayed(resultsLoopSound, gameOverSound != null ? gameOverSound.length + 0.5f : 0.5f, true );
		}
	}

	protected void PlayDelayed(AudioClip clip, float delay, bool loop = false, float pitch = 1f) {
		StartCoroutine(_PlayDelayed(clip, delay, loop));
	}

	private IEnumerator _PlayDelayed(AudioClip clip, float delay, bool loop = false, float pitch = 1f) {
		yield return new WaitForSeconds(delay);

		audio.pitch = pitch;
		audio.loop = loop;
		audio.clip = clip;
		audio.Play();
	}

	protected virtual void Update_RESULTS() {
		//Debug.Log(gameObject.name + " Update_RESULTS");
	}
	protected virtual void Exit_RESULTS() {
		Debug.Log(gameObject.name + " Exit_RESULTS");

		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
		if(textScore != null) textScore.gameObject.SetActive(false);
		if(audio != null && audio.isPlaying && audio.clip == resultsLoopSound) audio.Stop();
	}

	protected virtual bool IsGameReady() {
		if(robot == null) return false;
		if(GameLayoutTracker.instance == null) return false;

		return GameLayoutTracker.instance.Validated;
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
		
		if(maxPlayTime > 0f && stateTimer >= maxPlayTime) return true;
		if(scoreToWin > 0) {
			if(score >= scoreToWin) {
				return true;
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

	protected void PlayAudioClips(AudioClip[] clips, float initalDelay = 0f, float additionalDelay = 0.05f, float pitch = 1f) {
		audio.pitch = pitch;
		if(clips.Length > 0) {
			PlayDelayed(clips[0], initalDelay);

			for(int i = 1; i < clips.Length; ++i) {
				PlayDelayed(clips[i], clips[i-1].length + additionalDelay);
			}
		}
	}

	protected void PlayCountdownAudio(int secondsLeft, float pitch = 1f) {
		bool played = false;

		for(int i=0; i<timerSounds.Length; i++) {
			if(secondsLeft == timerSounds[i].time && lastTimerSeconds != timerSounds[i].time) {
				// defer to other notifications
				if( !notificationAudio.isPlaying ) {
					notificationAudio.pitch = pitch;
					notificationAudio.PlayOneShot(timerSounds[i].sound);
				}
				played = true;
				break;
			}
		}

		if(!played && lastTimerSeconds != secondsLeft && countdownTickSound != null) PlayOneShot(countdownTickSound);
		
		lastTimerSeconds = secondsLeft;
	}
	
	protected void PlayNotificationAudio(AudioClip clip, float pitch = 1f)
	{
		if (notificationAudio != null) 
		{
			notificationAudio.pitch = pitch;
			notificationAudio.clip = clip; // sets clip to be the audio source's default so isPlaying works properly
			notificationAudio.Stop (); 
			Debug.LogWarning ("Should be playing " + clip.name);
			notificationAudio.Play (); // playoneshot creates a new AudioSource, making isPlaying false for the AudioSource
		}
	}

	// doesn't play if the audio source is currently playing a clip
	protected void PlayNotificationAudioDeferred(AudioClip clip, float pitch = 1f)
	{
		if (notificationAudio != null && !notificationAudio.isPlaying) 
		{
			notificationAudio.pitch = pitch;
			notificationAudio.clip = clip; // sets clip to be the audio source's default so isPlaying works properly
			notificationAudio.Stop (); 
			Debug.LogWarning ("Should be playing " + clip.name);
			notificationAudio.Play (); // playoneshot creates a new AudioSource, making isPlaying false for the AudioSource
		}
	}

}
