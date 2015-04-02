using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class GameController : MonoBehaviour {

	public enum GameState {
		BUILDING,
		PLAYING,
		RESULTS
	}

	[SerializeField] protected float maxPlayTime = 0f;
	[SerializeField] protected int scoreToWin = 0;
	[SerializeField] protected int numPlayers = 1;
	[SerializeField] protected Text textScore = null;
	[SerializeField] protected Text textError = null;
	[SerializeField] protected Text textState = null;
	[SerializeField] protected Text textTime = null;
	[SerializeField] protected bool autoPlay = false;
	[SerializeField] protected Button playButton = null;
	[SerializeField] protected BuildInstructionsController buildInstructionsController = null;
	[SerializeField] protected string buildInstructionsLayoutFilter = null;
	[SerializeField] protected Image resultsPanel = null;
	[SerializeField] protected AudioClip playerScoreSound;
	[SerializeField] protected AudioClip playingLoopSound;
	[SerializeField] protected AudioClip gameOverSound;

	protected bool playRequested = false;
	protected bool buildRequested = false;

	protected GameState state = GameState.BUILDING;
	protected float stateTimer = 0f;
	protected int[] scores;

	protected int winnerIndex;
	protected bool firstFrame = true;

	float errorMsgTimer = 0f;

	void OnEnable () {
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
		if(buildInstructionsController != null) buildInstructionsController.SetLayoutForGame(buildInstructionsLayoutFilter);
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

	void OnDisable () {
		if(textError != null) textError.gameObject.SetActive(false);
		if(playButton != null) playButton.gameObject.SetActive(false);
		if(resultsPanel != null) resultsPanel.gameObject.SetActive(false);
	}

	GameState GetNextState() {

		//consider switching states
		switch(state) {
			case GameState.BUILDING:
				if((playRequested || autoPlay) && IsGameReady()) return GameState.PLAYING; 
				break;
			case GameState.PLAYING:
				if(buildRequested) return GameState.BUILDING;
				if(IsGameOver()) return GameState.RESULTS;
				break;
			case GameState.RESULTS:
				if(buildRequested) return GameState.BUILDING;
				if(playRequested && IsGameReady()) return GameState.PLAYING;
				break;
		}

		//stay in current state
		return state;
	}

	void EnterState() {
		stateTimer = 0f;

		switch(state) {
			case GameState.BUILDING: 	Enter_BUILDING(); break;
			case GameState.PLAYING: 	Enter_PLAYING(); break;
			case GameState.RESULTS: 	Enter_RESULTS(); break;
		}
	}

	void UpdateState() {
		switch(state) {
			case GameState.BUILDING: 	Update_BUILDING(); break;
			case GameState.PLAYING: 	Update_PLAYING(); break;
			case GameState.RESULTS: 	Update_RESULTS(); break;
		}
	}

	void ExitState() {
		switch(state) {
			case GameState.BUILDING: 	Exit_BUILDING(); break;
			case GameState.PLAYING: 	Exit_PLAYING(); break;
			case GameState.RESULTS: 	Exit_RESULTS(); break;
		}
	}

	protected virtual void RefreshHUD() {
		if(textScore != null && scores != null && scores.Length > 0) {
			textScore.text = "score: " + scores[0];

		}

		if(textState != null) {
			textState.text = state.ToString();
		}

		if(textTime != null) {
			if(maxPlayTime > 0f) {
				textTime.text = Mathf.FloorToInt(maxPlayTime - stateTimer).ToString();
			}
			else {
				textTime.text = Mathf.FloorToInt(stateTimer).ToString();
			}
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
		
	protected virtual void Enter_PLAYING() {
		Debug.Log(gameObject.name + " Enter_PLAYING");
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

		return buildInstructionsController.Validated;
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
}
