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


	protected GameState state = GameState.PRE_GAME;
	protected float stateTimer = 0f;
	protected int[] scores;
	protected int winnerIndex;

	void OnEnable () {
		state = GameState.PRE_GAME;
		stateTimer = 0f;
		scores = new int[numPlayers];
		winnerIndex = -1;
	}

	void Update () {
		stateTimer += Time.deltaTime;

		GameState nextState = GetNextState();

		if(nextState != state) {
			ExitState();
			state = nextState;
			EnterState();
		}
		else {
			UpdateState();
		}

		RefreshScoreBoard();
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

	protected virtual void RefreshScoreBoard() {
		if(textScore == null) return;
		if(scores == null) return;
		if(scores.Length == 0) return;

		textScore.text = "score: " + scores[0];
	}

	protected virtual void Enter_PRE_GAME() {
		winnerIndex = -1;
	}
	protected virtual void Update_PRE_GAME() {}
	protected virtual void Exit_PRE_GAME() {}
		
	protected virtual void Enter_PLAYING() {}
	protected virtual void Update_PLAYING() {}
	protected virtual void Exit_PLAYING() {}

	protected virtual void Enter_RESULTS() {}
	protected virtual void Update_RESULTS() {}
	protected virtual void Exit_RESULTS() {}

	protected virtual bool IsGameReady() {

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
}
