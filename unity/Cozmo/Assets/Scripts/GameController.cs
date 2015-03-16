using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class GameController : MonoBehaviour {

	public enum GameState {
		PRE_GAME,
		PLAYING,
		RESULTS
	}

	[SerializeField] protected float maxPlayTime = 0f;
	[SerializeField] protected int scoreToWin = 0;
	[SerializeField] protected int numPlayers = 1;
	[SerializeField] protected Text textScore = null;
	[SerializeField] protected Text textState = null;
	[SerializeField] protected Text textTime = null;
	[SerializeField] protected bool autoPlay = false;
	[SerializeField] protected Button playButton = null;

	protected bool playRequested = false;
	protected GameState state = GameState.PRE_GAME;
	protected float stateTimer = 0f;
	protected int[] scores;
	protected int winnerIndex;
	protected bool firstFrame = true;

	void OnEnable () {
		state = GameState.PRE_GAME;
		stateTimer = 0f;
		scores = new int[numPlayers];
		winnerIndex = -1;
		firstFrame = true;
		playRequested = autoPlay;
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
	}

	GameState GetNextState() {

		//consider switching states
		switch(state) {
			case GameState.PRE_GAME:
				if(IsGameReady()) return GameState.PLAYING; 
				break;
			case GameState.PLAYING:
				if(IsGameOver()) return GameState.RESULTS;
				break;
			case GameState.RESULTS:
				
				break;
		}

		//stay in current state
		return state;
	}

	void EnterState() {
		stateTimer = 0f;

		switch(state) {
			case GameState.PRE_GAME: 	Enter_PRE_GAME(); break;
			case GameState.PLAYING: 	Enter_PLAYING(); break;
			case GameState.RESULTS: 	Enter_RESULTS(); break;
		}
	}

	void UpdateState() {
		switch(state) {
			case GameState.PRE_GAME: 	Update_PRE_GAME(); break;
			case GameState.PLAYING: 	Update_PLAYING(); break;
			case GameState.RESULTS: 	Update_RESULTS(); break;
		}
	}

	void ExitState() {
		switch(state) {
			case GameState.PRE_GAME: 	Exit_PRE_GAME(); break;
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

		if(playButton != null) playButton.gameObject.SetActive(state == GameState.PRE_GAME);

	}

	protected virtual void Enter_PRE_GAME() {
		winnerIndex = -1;
		Debug.Log(gameObject.name + " Enter_PRE_GAME");
	}
	protected virtual void Update_PRE_GAME() {
		//Debug.Log(gameObject.name + " Update_PRE_GAME");
	}
	protected virtual void Exit_PRE_GAME() {
		Debug.Log(gameObject.name + " Exit_PRE_GAME");
	}
		
	protected virtual void Enter_PLAYING() {
		Debug.Log(gameObject.name + " Enter_PLAYING");
	}
	protected virtual void Update_PLAYING() {
		//Debug.Log(gameObject.name + " Update_PLAYING");
	}
	protected virtual void Exit_PLAYING() {
		Debug.Log(gameObject.name + " Exit_PLAYING");
	}

	protected virtual void Enter_RESULTS() {
		Debug.Log(gameObject.name + " Enter_RESULTS");
	}
	protected virtual void Update_RESULTS() {
		//Debug.Log(gameObject.name + " Update_RESULTS");
	}
	protected virtual void Exit_RESULTS() {
		Debug.Log(gameObject.name + " Exit_RESULTS");
	}

	protected virtual bool IsGameReady() {
		if(!playRequested) return false;
		if(RobotEngineManager.instance == null) return false;
		if(RobotEngineManager.instance.current == null) return false;

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
}
