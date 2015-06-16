using UnityEngine;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.Cozmo;
using System;

public class VortexController : GameController {

	public enum VortexState {
		INTRO,
		REQUEST_SPIN,
		SPINNING,
		SPIN_COMPLETE		
	}

	class VortexInput {
		public List<float> stamps = new List<float>();
	}

	[Serializable]
	public class VortexSettings {
		public int rings;
		public int[] slicesPerRing;
		public int roundsPerRing;
	}

	SpinWheel wheel { 
		get {
			if(rings == 1) return wheels[0];

			int index = rings - Mathf.CeilToInt(round / roundsPerRing);
			index = Mathf.Clamp(index, 0, wheels.Length-1);
			return wheels[index];
		}
	}

	[SerializeField] SpinWheel[] wheels;
	[SerializeField] Text textPlayState;
	[SerializeField] Text textCurrentNumber; // only use for single ring matches
	[SerializeField] Text textRoundNumber;
	[SerializeField] Text[] textPlayerCurrentNumbers; // only use for single ring matches

	[SerializeField] Text[] textfinalPlayerScores;

	[SerializeField] AudioClip numberChangedSound;
	[SerializeField] AudioClip buttonPressSound;
	[SerializeField] AudioClip roundCompleteWinner;
	[SerializeField] AudioClip roundCompleteNoWinner;

	[SerializeField] AudioClip timeUp;
	[SerializeField] AudioClip newHighScore;

	[SerializeField] Button[] playerButtons;
	[SerializeField] LayoutBlock2d[] playerMockBlocks;
	[SerializeField] Text[] textPlayerScores;
	[SerializeField] List<VortexInput> playerInputs = new List<VortexInput>();
	
	[SerializeField] int pointsForRoundWin = 30;
	[SerializeField] int pointsForRoundCorrect = 10;

	[SerializeField] VortexSettings[] settingsPerLevel;

	int numPlayers = 0;
	int currentPlayerIndex = 0;
	int round = 0;

	VortexState playState = VortexState.INTRO;

	protected override void Awake () {
		base.Awake();
	}

	protected override void OnEnable () {
		base.OnEnable();
		
		rings = settingsPerLevel[currentLevelNumber-1].rings;
		roundsPerRing = settingsPerLevel[currentLevelNumber-1].roundsPerRing;
		slicesPerRing = settingsPerLevel[currentLevelNumber-1].slicesPerRing;
		
		for(int i=0;i<slicesPerRing.Length && i<wheels.Length;i++) {
			wheels[i].SetNumSlices(slicesPerRing[i]);
		}

		for(int i=0;i<wheels.Length;i++) {
			wheels[i].Lock();
			wheels[i].HidePeg();
			wheels[i].gameObject.SetActive(i < rings);
		}
		
		MessageDelay = .1f;
	}

	protected override void OnDisable () {
		base.OnDisable();
	}

	protected override void Enter_BUILDING () {
		base.Enter_BUILDING();
	}

	protected override void Exit_BUILDING () {
		base.Exit_BUILDING();
	}

	protected override void Update_BUILDING () {
		base.Update_BUILDING();
	}

	protected override void Enter_PRE_GAME () {
		
		ClearInputs();

		base.Enter_PRE_GAME();
	}

	protected override void Update_PRE_GAME () {
		base.Update_PRE_GAME();
	}

	protected override void Exit_PRE_GAME () {
		base.Exit_PRE_GAME();
	}

	protected override void Enter_PLAYING () {
		ClearInputs();
		playState = VortexState.INTRO;

		EnterPlayState();
		base.Enter_PLAYING();
	}
	
	protected override void Update_PLAYING () {
		base.Update_PLAYING();

		VortexState nextPlayState = GetNextPlayState();

		if(playState != nextPlayState) {
			//Debug.Log("frame("+Time.frameCount+") playState("+playState+")->("+nextPlayState+")");
			ExitPlayState();
			playState = nextPlayState;
			EnterPlayState();
		}
		else {
			UpdatePlayState();
		}

	}
	
	protected override void Exit_PLAYING (bool overrideStars = false) {
		base.Exit_PLAYING();
	}

	protected override void Enter_RESULTS() {
		base.Enter_RESULTS();

		for(int i=0;i<textfinalPlayerScores.Length && i<scores.Count;i++) {
			textfinalPlayerScores[i].text = "Player " + (i+1).ToString() + ": " + scores[i].ToString();
			textfinalPlayerScores[i].color = winners.Contains(i) ? Color.green : Color.white;
		}
		
	}

	protected override bool IsPreGameCompleted() {
		return true;
	}

	protected override bool IsGameReady () {
		//if(!base.IsGameReady()) return false;

		return true;
	}

	protected override bool IsGameOver() {
		//if(base.IsGameOver()) return true;
		return round > (roundsPerRing * rings);
	}

	int rings = 1;
	int roundsPerRing = 5;
	int[] slicesPerRing;
	int lastNumber = 0;
	protected override void RefreshHUD() {
		base.RefreshHUD();

		textRoundNumber.text = "ROUND " + Mathf.Max(1, round).ToString();

		int newNumber = wheel.GetCurrentNumber();


//		if(numberChangedSound != null && lastNumber != newNumber && lastNumber != 0) {
//			AudioManager.PlayOneShot(numberChangedSound);
//		}


		textCurrentNumber.text = newNumber.ToString();
		textPlayState.text = state == GameState.PLAYING ? playState.ToString() : "";
		lastNumber = newNumber;
	}

	float playStateTimer = 0f;
	VortexState GetNextPlayState() {
		switch(playState) {
			case VortexState.INTRO:
				return VortexState.REQUEST_SPIN;
				//break;
			case VortexState.REQUEST_SPIN:
				if(wheel.Spinning) return VortexState.SPINNING;
				break;
			case VortexState.SPINNING:
				if(wheel.Finished) {
					return VortexState.SPIN_COMPLETE;
				}
				break;
			case VortexState.SPIN_COMPLETE:
				if(!IsGameOver() && playStateTimer > 5f) return VortexState.REQUEST_SPIN;
				break;
		}

		return playState;
	}

	void EnterPlayState() {
		playStateTimer = 0f;
		switch(playState) {
			case VortexState.INTRO: 		Enter_INTRO(); break;
			case VortexState.REQUEST_SPIN: 	Enter_REQUEST_SPIN(); break;
			case VortexState.SPINNING: 		Enter_SPINNING(); break;
			case VortexState.SPIN_COMPLETE: Enter_SPIN_COMPLETE(); break;
		}
	}
	
	void UpdatePlayState() {
		playStateTimer += Time.deltaTime;
		switch(playState) {
			case VortexState.INTRO: 		Update_INTRO(); break;
			case VortexState.REQUEST_SPIN: 	Update_REQUEST_SPIN(); break;
			case VortexState.SPINNING: 		Update_SPINNING(); break;
			case VortexState.SPIN_COMPLETE: Update_SPIN_COMPLETE(); break;
		}
	}

	void ExitPlayState() {
		switch(playState) {
			case VortexState.INTRO: 		Exit_INTRO(); break;
			case VortexState.REQUEST_SPIN: 	Exit_REQUEST_SPIN(); break;
			case VortexState.SPINNING: 		Exit_SPINNING(); break;
			case VortexState.SPIN_COMPLETE: Exit_SPIN_COMPLETE(); break;
		}
	}

	void Enter_INTRO() {
		currentPlayerIndex = UnityEngine.Random.Range(0, numPlayers);
		round = 0;
		//enable intro text
	}
	void Update_INTRO() {}
	void Exit_INTRO() {
		//disable intro text
	}

	void Enter_REQUEST_SPIN() {

		currentPlayerIndex++;
		if(currentPlayerIndex >= numPlayers) currentPlayerIndex = 0;

		round++;
		for(int i=0;i<wheels.Length;i++) {
			if(wheel == wheels[i]) continue;
			wheels[i].Lock();
			wheels[i].HidePeg();
			wheels[i].gameObject.SetActive(round <= (wheels.Length-i)*roundsPerRing);
		}

		//enable intro text
		wheel.Unlock();
		wheel.ShowPeg();

	}
	void Update_REQUEST_SPIN() {}
	void Exit_REQUEST_SPIN() {
		ClearInputs();
	}

	void Enter_SPINNING() {
		//start spin audio?
	}
	void Update_SPINNING() {}
	void Exit_SPINNING() {
		//stop looping spin audio
		//play spin finished audio
		wheel.Lock();
	}

	List<int> playersThatAreCorrect = new List<int>();
	void Enter_SPIN_COMPLETE() {
		
		playersThatAreCorrect.Clear();
		int fastestPlayer = -1;
		float earliestFinalStamp = float.MaxValue;

		int number = wheel.GetCurrentNumber();
		for(int i=0;i<playerInputs.Count;i++) {
			if(playerInputs[i].stamps.Count != number) continue;
			if(playerInputs[i].stamps.Count == 0) continue;

			playersThatAreCorrect.Add(i);
			float finalStamp = playerInputs[i].stamps[playerInputs[i].stamps.Count-1];
			if(finalStamp < earliestFinalStamp) {
				earliestFinalStamp = finalStamp;
				fastestPlayer = i;
			}
		}

		for(int i=0;i<scores.Count;i++) {

			Color c1 = Color.black;
			Color c2 = Color.black;
			Color c3 = Color.black;
			Color c4 = Color.black;

			if(i == fastestPlayer) {
				scores[i] += pointsForRoundWin;

				c1 = playerInputs[i].stamps.Count > 0 ? Color.green : Color.black;
				c2 = playerInputs[i].stamps.Count > 1 ? Color.green : Color.black;
				c3 = playerInputs[i].stamps.Count > 2 ? Color.green : Color.black;
				c4 = playerInputs[i].stamps.Count > 3 ? Color.green : Color.black;

			}
			else if(playersThatAreCorrect.Contains(i)) {
				scores[i] += pointsForRoundCorrect;

				c1 = playerInputs[i].stamps.Count > 0 ? Color.blue : Color.black;
				c2 = playerInputs[i].stamps.Count > 1 ? Color.blue : Color.black;
				c3 = playerInputs[i].stamps.Count > 2 ? Color.blue : Color.black;
				c4 = playerInputs[i].stamps.Count > 3 ? Color.blue : Color.black;
			}
			else if(playerInputs.Count > i) {
				c1 = playerInputs[i].stamps.Count > 0 ? Color.red : Color.black;
				c2 = playerInputs[i].stamps.Count > 1 ? Color.red : Color.black;
				c3 = playerInputs[i].stamps.Count > 2 ? Color.red : Color.black;
				c4 = playerInputs[i].stamps.Count > 3 ? Color.red : Color.black;
			}

			if(playerMockBlocks.Length > i) {
				playerMockBlocks[i].SetLights(c1, c2, c3, c4);
			}

			if(i < textPlayerScores.Length) textPlayerScores[i].text = "P" + (i+1).ToString() + ": " + scores[i].ToString();
		}

		if(playersThatAreCorrect.Count > 0) {
			if(roundCompleteWinner != null) AudioManager.PlayOneShot(roundCompleteWinner);
		}
		else {
			if(roundCompleteNoWinner != null) AudioManager.PlayOneShot(roundCompleteNoWinner);
		}
	}

	void Update_SPIN_COMPLETE() {}
	void Exit_SPIN_COMPLETE() {



	}

	void ClearInputs() {
		for(int i=0;i<playerInputs.Count;i++) {
			playerInputs[i].stamps.Clear();
		}

		for(int i=0;i<playerMockBlocks.Length;i++) {
			playerMockBlocks[i].Initialize(CubeType.LIGHT_CUBE);
			playerMockBlocks[i].SetLights(Color.black, Color.black, Color.black, Color.black);
		}
	}

///
	public void PlayerInputTap(int index) {
		if(state != GameState.PLAYING) return;
		if(playState != VortexState.SPINNING) return;

		while(index >= playerInputs.Count) playerInputs.Add (new VortexInput());
		if(playerInputs[index].stamps.Count >= 4) return;

		playerInputs[index].stamps.Add(Time.time);

//		//if this is fifth stamp, then remove the prior 4 such that we go back to 1, 
//		//	but still have our relevant 'last' time stamp at the end of the list
//		if(playerInputs[index].stamps.Count > 4) {
//			playerInputs[index].stamps.RemoveRange(0, 4);
//		}

		Color c1 = playerInputs[index].stamps.Count > 0 ? Color.white : Color.black;
		Color c2 = playerInputs[index].stamps.Count > 1 ? Color.white : Color.black;
		Color c3 = playerInputs[index].stamps.Count > 2 ? Color.white : Color.black;
		Color c4 = playerInputs[index].stamps.Count > 3 ? Color.white : Color.black;

		playerMockBlocks[index].SetLights(c1, c2, c3, c4);

		if(buttonPressSound != null) AudioManager.PlayOneShot(buttonPressSound);
	}

}
