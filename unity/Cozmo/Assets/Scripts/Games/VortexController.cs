using UnityEngine;
using UnityEngine.EventSystems;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.Cozmo;
using System;
using DigitalRuby.ThunderAndLightning;

public class VortexController : GameController {

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
				if(stamps.Count == 0) return -1f;
				return stamps[stamps.Count-1];
			}
		}

		public float FirstTime {
			get {
				if(stamps.Count == 0) return Time.time;
				return stamps[0];
			}
		}
	}

	
	struct ScoreBoardData {
		public int playerIndex;
		public Color color;
		public int score;
		public int place; //distinct from ordering to handle ties
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
	}

	int currentWheelIndex {
		get {
			if(rings == 1) return 0;
			
			int index = 0;
			if(settings.inward) {
				index = Mathf.CeilToInt(round / roundsPerRing) - 1;
			}
			else {
				index = rings - Mathf.CeilToInt(round / roundsPerRing);
			}
			
			index = Mathf.Clamp(index, 0, wheels.Count-1);
			return index;
		}
	}

	SpinWheel wheel { 
		get {
			return wheels[currentWheelIndex];
		}
	}

	[SerializeField] List<SpinWheel> wheels;
	[SerializeField] Text textPlayState;
	[SerializeField] Text textCurrentNumber; // only use for single ring matches
	[SerializeField] Text textRoundNumber;
	[SerializeField] Text[] textPlayerCurrentNumbers; // only use for single ring matches

	[SerializeField] Text[] textfinalPlayerScores;
	[SerializeField] Image[] imageFinalPlayerScoreBGs;

	[SerializeField] AudioClip numberChangedSound;
	[SerializeField] AudioClip buttonPressSound;
	[SerializeField] AudioClip roundCompleteWinner;
	[SerializeField] AudioClip roundCompleteNoWinner;

	[SerializeField] AudioClip timeUp;
	[SerializeField] AudioClip newHighScore;
	[SerializeField] AudioClip spinRequestSound;

	[SerializeField] RectTransform[] playerPanels;
	[SerializeField] RectTransform[] playerTokens;
	[SerializeField] Button[] playerButtons;
	[SerializeField] LayoutBlock2d[] playerMockBlocks;
	[SerializeField] Image[] playerPanelFills;
	[SerializeField] Image[] imageInputLocked;
	[SerializeField] Text[] textPlayerBids;
	[SerializeField] Text[] textPlayerSpinNow;
	[SerializeField] Animation[] playerBidFlashAnimations;
	[SerializeField] Text[] textPlayerScores;

	[SerializeField] VortexSettings[] settingsPerLevel;

	[SerializeField] LightningBoltShapeSphereScript lightingBall;
	[SerializeField] float[] wheelLightningRadii  = { 3.5f, 2f, 1f };
	[SerializeField] int lightningMinCountAtSpeedMax = 8;
	[SerializeField] int lightningMaxCountAtSpeedMax = 16;

	[SerializeField] float maxPlayerInputTime = 3f;

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
	float cozmoTimePerTap = 1.25f;
	List<int> playersThatAreCorrect = new List<int>();
	
	[SerializeField] float scoreDisplayFillFade = 0.5f;
	[SerializeField] float scoreDisplayFillAlpha = 0.5f;
	[SerializeField] float scoreDisplayEmptyAlpha = 0.1f;
	
	float fadeTimer = 1f;
	int resultsDisplayIndex = 0;

	protected override void Awake () {
		base.Awake();

		playerButtonCanvasGroups = new CanvasGroup[playerButtons.Length];
		for(int i=0;i<playerButtons.Length;i++) {
			playerButtonCanvasGroups[i] = playerButtons[i].gameObject.GetComponent<CanvasGroup>();
		}
	}

	protected override void OnEnable () {
		base.OnEnable();
		
		numPlayers = PlayerPrefs.GetInt("NumberOfPlayers", 1);
		settings = settingsPerLevel[currentLevelNumber-1];

		rings = settings.rings;
		roundsPerRing = settings.roundsPerRing;
		slicesPerRing = settings.slicesPerRing;

		for(int i=0;i<playerPanels.Length;i++) {
			playerPanels[i].gameObject.SetActive(i < numPlayers);
		}

		for(int i=0;i<slicesPerRing.Length && i<wheels.Count;i++) {
			wheels[i].SetNumSlices(slicesPerRing[i]);
		}

		for(int i=0;i<wheels.Count;i++) {
			wheels[i].ResetWheel();
			wheels[i].Lock();
			wheels[i].Unfocus();
			wheels[i].gameObject.SetActive(i < rings);
		}
		
		PlaceTokens();

		lightingBall.CountRange = new RangeOfIntegers { Minimum = 0, Maximum = 0 };

		MessageDelay = .1f;

		for(int i=0;i<playerButtonCanvasGroups.Length;i++) {
			playerButtonCanvasGroups[i].interactable = false;
			playerButtonCanvasGroups[i].blocksRaycasts = false;
		}

		for(int i=0;i<textPlayerScores.Length;i++) {
			textPlayerScores[i].text = "SCORE: 0";
		}
		
		for(int i=0;i<playerPanelFills.Length;i++) {
			Color col = playerPanelFills[i].color;
			col.a = 0f;
			playerPanelFills[i].color = col;
		}

		for(int i=0;i<textPlayerBids.Length;i++) {
			textPlayerBids[i].text = "";
		}

		foreach(Image image in imageInputLocked) image.gameObject.SetActive(false);

		for(int i=0;i<textPlayerSpinNow.Length;i++) {
			textPlayerSpinNow[i].gameObject.SetActive(false);
		}


	}

	protected override void OnDisable () {
		base.OnDisable();

		if(playState == VortexState.SPINNING) ActiveBlock.TappedAction -= BlockTapped;
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

		playerInputBlocks.Clear();

		if( robot != null && PlayerPrefs.GetInt("DebugSkipLayoutTracker",0) == 0) {
			for(int i=0;i<robot.activeBlocks.Count && i<numPlayers;i++) {
				ActiveBlock block = robot.activeBlocks[i];
				Debug.Log("adding playerInputBlocks["+i+"] with PlayerInputTap("+i+")");
				playerInputBlocks.Add(block);
				
				switch(i) {
					case 0: block.SetMode(ActiveBlock.Mode.Blue); break;
					case 1: block.SetMode(ActiveBlock.Mode.Green); break;
					case 2: block.SetMode(ActiveBlock.Mode.Yellow); break;
					case 3: block.SetMode(ActiveBlock.Mode.Red); break;
				}
			}
	
			if(robot.carryingObject == null) {
				robot.PickAndPlaceObject(playerInputBlocks[1]);
				if(CozmoBusyPanel.instance != null)	{
					string desc = "Cozmo is attempting to pick-up\n a game cube.";
					CozmoBusyPanel.instance.SetDescription(desc);
				}
			}

		}
	}

	protected override void Update_PRE_GAME () {
		base.Update_PRE_GAME();
	}

	protected override void Exit_PRE_GAME () {
		base.Exit_PRE_GAME();
	}

	protected override void Enter_PLAYING () {
		for(int i=0;i<textPlayerScores.Length;i++) {
			textPlayerScores[i].text = "SCORE: 0";
		}

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

		ExitPlayState();
	}

	protected override void Enter_RESULTS() {
		base.Enter_RESULTS();

		sortedScoreData.Clear();

		for(int i=0;i<scores.Count && i<numPlayers;i++) {
			int score = scores[i];

			int insertIndex = sortedScoreData.Count;

			for(int j=0;j<sortedScoreData.Count;j++) {
				if(score < sortedScoreData[j].score) continue;
				insertIndex = j;
				break;
			}

			ScoreBoardData scoreData = new ScoreBoardData();
			scoreData.score = score;
			scoreData.playerIndex = i;
			switch(i) {
				case 0: scoreData.color = Color.blue; break;
				case 1: scoreData.color = Color.green; break;
				case 2: scoreData.color = Color.yellow; break;
				case 3: scoreData.color = Color.red; break;
				default:
					Debug.Log("no color assigned for playerIndex: " + i);
					break;
			}

			if(insertIndex < sortedScoreData.Count) {
				sortedScoreData.Insert(insertIndex, scoreData);
			}
			else {
				sortedScoreData.Add(scoreData);
			}
			
		}

		int placeCounter = 0;
		int lastScore = sortedScoreData[0].score;
		for(int i=0;i<sortedScoreData.Count;i++) {
			if(sortedScoreData[i].score != lastScore) placeCounter++;
			ScoreBoardData data = sortedScoreData[i];
			data.place = placeCounter;
			lastScore = data.score;
			sortedScoreData[i] = data;
		}
		
		
		for(int i=0;i<textfinalPlayerScores.Length && i<imageFinalPlayerScoreBGs.Length;i++) {

			if(i >= sortedScoreData.Count) {
				textfinalPlayerScores[i].gameObject.SetActive(false);
				imageFinalPlayerScoreBGs[i].gameObject.SetActive(false);
				continue;
			}

			string scoreText = "";

			switch(sortedScoreData[i].place) {
				case 0: scoreText += "1st place: "; break;
				case 1: scoreText += "2nd place: "; break;
				case 2: scoreText += "3rd place: "; break;
				case 3: scoreText += "4th place: "; break;
			}

			scoreText += sortedScoreData[i].score;

			textfinalPlayerScores[i].text = scoreText;
			textfinalPlayerScores[i].color = sortedScoreData[i].color;
			imageFinalPlayerScoreBGs[i].color = sortedScoreData[i].color;
			//Debug.Log("sortedScoreData["+i+"] scoreText("+scoreText+") ");

			
			textfinalPlayerScores[i].gameObject.SetActive(true);
			imageFinalPlayerScoreBGs[i].gameObject.SetActive(true);
		}
		
	}

	protected override void Exit_RESULTS() {
		base.Exit_RESULTS();
		
		for(int i=0;i<textPlayerScores.Length;i++) {
			textPlayerScores[i].text = "SCORE: 0";
		}
		
	}

	protected override bool IsPreGameCompleted() {
		if( robot != null && PlayerPrefs.GetInt("DebugSkipLayoutTracker",0) == 0 ) {

			if(robot.carryingObject == null || !robot.carryingObject.isActive || robot.isBusy) return false;

			if(!base.IsPreGameCompleted()) return false;
		}
		return true;
	}

	protected override bool IsGameReady () {
		if( robot != null && PlayerPrefs.GetInt("DebugSkipLayoutTracker",0) == 0 ) {

			if(robot.activeBlocks.Count < numPlayers) return false;

			if(!base.IsGameReady()) return false;
		}

		return true;
	}

	protected override bool IsGameOver() {
		//if(base.IsGameOver()) return true;

		if(round > (roundsPerRing * rings)) return true;

		for(int i=0;i<playersEliminated.Length;i++) {
			if(!playersEliminated[i]) return false;
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

	VortexState GetNextPlayState() {
		switch(playState) {
			case VortexState.INTRO:
				return VortexState.REQUEST_SPIN;
				//break;
			case VortexState.REQUEST_SPIN:
				if(wheel.Spinning) {
					Debug.Log("cozmo SpinUnderway");
					return VortexState.SPINNING;
				}
				break;
			case VortexState.SPINNING:
				if(wheel.Finished) {
					//float fullTime = Time.time - wheel.SpinStartTime;
					//Debug.Log("cozmo wheel.Finished playStateTimer("+playStateTimer+") fullTime("+fullTime+")");
					return VortexState.SPIN_COMPLETE;
				}
				break;
			case VortexState.SPIN_COMPLETE:
				if(!IsGameOver() && playStateTimer > 2f + (playersThatAreCorrect.Count * scoreDisplayFillFade * 2f)) return VortexState.REQUEST_SPIN;
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
		currentPlayerIndex = -1;//UnityEngine.Random.Range(0, numPlayers);
		round = 0;

		for(int i=0;i<playersEliminated.Length;i++) {
			playersEliminated[i] = false;
		}

		PlaceTokens();

		//enable intro text
	}
	void Update_INTRO() {}
	void Exit_INTRO() {
		//disable intro text
	}

	[SerializeField] Vector2 cozmoStartDragPos = Vector2.zero;
	[SerializeField] Vector2 cozmoEndDragPos = Vector2.zero;
	[SerializeField] float cozmoDragDelay = 2f;
	[SerializeField] float cozmoDragTime = 2f;

	void Enter_REQUEST_SPIN() {

		currentPlayerIndex++;
		if(currentPlayerIndex >= numPlayers) currentPlayerIndex = 0;

		round++;
		for(int i=0;i<wheels.Count;i++) {

			if(wheel == wheels[i]) continue;

			//wheels[i].ResetWheel();
			wheels[i].Lock();
			wheels[i].Unfocus();
		
			bool show = rings > i;

			if(settings.inward) {
				show &= round <= (i+1)*roundsPerRing;
			}
			else {
				show &= round <= (wheels.Count-i)*roundsPerRing;
			}

			wheels[i].gameObject.SetActive(show);
		}

		PlaceTokens();

		if(currentPlayerIndex == 1) {
			wheel.AutomateMode();

			Vector3 startDragPos = new Vector2(cozmoStartDragPos.x * Screen.width, cozmoStartDragPos.y * Screen.height);
			wheel.DragStart(startDragPos, Time.time);
		}
		else  {
			wheel.Unlock();
		}
		wheel.Focus();
		wheel.gameObject.SetActive(true);

		lightingBall.Radius = wheelLightningRadii[currentWheelIndex];

		if(spinRequestSound != null) AudioManager.PlayOneShot(spinRequestSound);
		lastNumber = wheel.GetDisplayedNumber();

		foreach(Image image in imageInputLocked) image.gameObject.SetActive(false);

		for(int i=0;i<textPlayerBids.Length;i++) {
			textPlayerBids[i].text = "";
		}

		for(int i=0;i<textPlayerSpinNow.Length;i++) {
			textPlayerSpinNow[i].gameObject.SetActive(currentPlayerIndex == i);
		}
	}
	void Update_REQUEST_SPIN() {

		if(currentPlayerIndex == 1 && playStateTimer > cozmoDragDelay) {
			if(playStateTimer < cozmoDragTime + cozmoDragDelay) {

				Vector3 startDragPos = new Vector2(cozmoStartDragPos.x * Screen.width, cozmoStartDragPos.y * Screen.height);
				Vector3 endDragPos = new Vector2(cozmoEndDragPos.x * Screen.width, cozmoEndDragPos.y * Screen.height);

				float factor = Mathf.Clamp01( (playStateTimer - cozmoDragDelay) / cozmoDragTime);
				wheel.DragUpdate(Vector2.Lerp(startDragPos, endDragPos, factor), Time.time);
			}
			else {
				wheel.DragEnd();
			}
		}

		int newNumber = wheel.GetDisplayedNumber();
		//Debug.Log("Update_SPINNING lightingMax("+lightingMax+") wheel.Speed("+wheel.Speed+") newNumber("+newNumber+") lastNumber("+lastNumber+")");
		if(newNumber != lastNumber) {
			lightingBall.SingleLightningBolt();
		}
		lastNumber = newNumber;

		

	}
	void Exit_REQUEST_SPIN() {
		ClearInputs();
		for(int i=0;i<textPlayerSpinNow.Length;i++) {
			textPlayerSpinNow[i].gameObject.SetActive(false);
		}
	}

	void Enter_SPINNING() {
		lightingBall.Radius = wheelLightningRadii[currentWheelIndex];

		for(int i=0;i<playerButtonCanvasGroups.Length;i++) {
			if(i == 1) continue; // player 2 is cozmo, no button
			playerButtonCanvasGroups[i].interactable = i < numPlayers;
			playerButtonCanvasGroups[i].blocksRaycasts = i < numPlayers;
		}
		predictedNum = -1;
		predictedDuration = -1f;
		predictedTimeAfterLastPeg = -1f;
		cozmoTapsSubmitted = 0;

		
		ActiveBlock.TappedAction += BlockTapped;

	}

	void Update_SPINNING() {
		int lightingMin = Mathf.FloorToInt(Mathf.Lerp(0, lightningMinCountAtSpeedMax, (wheel.Speed - 1f) * 0.1f));
		int lightingMax = Mathf.FloorToInt(Mathf.Lerp(0, lightningMaxCountAtSpeedMax, (wheel.Speed - 1f) * 0.1f));
		lightingBall.CountRange = new RangeOfIntegers { Minimum = lightingMin, Maximum = lightingMax };
		
		int newNumber = wheel.GetDisplayedNumber();
		//Debug.Log("Update_SPINNING lightingMax("+lightingMax+") wheel.Speed("+wheel.Speed+") newNumber("+newNumber+") lastNumber("+lastNumber+")");
		if(lightingMax == 0 && newNumber != lastNumber) {
			lightingBall.SingleLightningBolt();
		}
		lastNumber = newNumber;

		if(predictedNum < 0) {

			int numCheck = wheel.PredictedNumber;
	
			if(numCheck > 0) {
				predictedNum = numCheck;
				predictedDuration = wheel.TotalDuration;
				predictedTimeAfterLastPeg = wheel.TimeAfterLastPeg;
			}
		}

		if(cozmoTapsSubmitted == 0 && predictedNum > 0) {
			float time = Time.time - wheel.SpinStartTime;
			float timeToBid = predictedDuration - predictedTimeAfterLastPeg - (1f + predictedNum * cozmoTimePerTap);
			if(time > timeToBid) {

				if(robot != null) {
					robot.TapBlockOnGround(predictedNum);
					cozmoTapsSubmitted = predictedNum;
				}
				else {
					PlayerInputTap(1);
					cozmoTapsSubmitted = 1;
				}

				Debug.Log("cozmo predictedNum("+predictedNum+") time("+time+") timeToBid("+timeToBid+") predictedDuration("+predictedDuration+")");
			}
		}
		else if(predictedNum > 0 && cozmoTapsSubmitted < predictedNum) {
			if(Time.time - playerInputs[1].FinalTime >= cozmoTimePerTap) {
				PlayerInputTap(1);
				cozmoTapsSubmitted++;
				Debug.Log("cozmo predictedNum("+predictedNum+") cozmoTapsSubmitted("+cozmoTapsSubmitted+")");
			}
		}
		
		for(int i=0;i<playerPanelFills.Length;i++) {
			if(i >= numPlayers || playerPanelFills[i].color.a == 0f) continue;
			Color col = playerPanelFills[i].color;
			col.a = Mathf.Max(0f, col.a - Time.deltaTime * 4f);
			playerPanelFills[i].color = col;
		}

	}

	void Exit_SPINNING() {
		//stop looping spin audio
		//play spin finished audio
		wheel.Lock();
		lightingBall.CountRange = new RangeOfIntegers { Minimum = 0, Maximum = 0 };

		for(int i=0;i<playerButtonCanvasGroups.Length;i++) {
			playerButtonCanvasGroups[i].interactable = false;
			playerButtonCanvasGroups[i].blocksRaycasts = false;
		}

		
		ActiveBlock.TappedAction -= BlockTapped;

		for(int i=0;i<playerPanelFills.Length;i++) {
			if(i >= numPlayers || playerPanelFills[i].color.a == 0f) continue;
			Color col = playerPanelFills[i].color;
			col.a = 0f;
			playerPanelFills[i].color = col;
		}
	}


	void Enter_SPIN_COMPLETE() {
		
		playersThatAreCorrect.Clear();
		int fastestPlayer = -1;


		int number = wheel.GetDisplayedNumber();
		for(int i=0;i<playerInputs.Count;i++) {
			if(playerInputs[i].stamps.Count != number) continue;
			if(playerInputs[i].stamps.Count == 0) continue;

			playersThatAreCorrect.Add(i);
		}

		playersThatAreCorrect.Sort( ( obj1, obj2 ) =>  {
			float finalStamp1 = playerInputs[obj1].FinalTime;
			float finalStamp2 = playerInputs[obj2].FinalTime;

			if(finalStamp1 == finalStamp2) return 0;
			if(finalStamp1 > finalStamp2) return 1;
			return -1;   
		} );

		if(playersThatAreCorrect.Count > 0) {
			fastestPlayer = playersThatAreCorrect[0];
		}

		for(int i=0;i<playersEliminated.Length;i++) {
			if(playersEliminated[i]) continue;
			
			if(settings.winnerEliminated) {
				playersEliminated[i] = fastestPlayer == i;
			}
			else if(settings.elimination) {
				playersEliminated[i] = !playersThatAreCorrect.Contains(i) || playersThatAreCorrect.IndexOf(i) >= settings.maxSurvivePerRound[round-1];
//				if(playersEliminated[i]) {
//					Debug.Log("playersEliminated["+i+"] is true! playersThatAreCorrect.IndexOf(i)("+playersThatAreCorrect.IndexOf(i)+") maxSurvivePerRound["+(round-1)+"]("+settings.maxSurvivePerRound[round-1]+")");
//				}
			}

			//Debug.Log("playersEliminated["+i+"] = " + playersEliminated[i]);
		}

		for(int i=0;i<playerInputs.Count;i++) {

			Color c1 = Color.black;
			Color c2 = Color.black;
			Color c3 = Color.black;
			Color c4 = Color.black;

			if(i == fastestPlayer) {

				c1 = playerInputs[i].stamps.Count > 0 ? Color.green : Color.black;
				c2 = playerInputs[i].stamps.Count > 1 ? Color.green : Color.black;
				c3 = playerInputs[i].stamps.Count > 2 ? Color.green : Color.black;
				c4 = playerInputs[i].stamps.Count > 3 ? Color.green : Color.black;
			}
			else if(playersThatAreCorrect.Contains(i)) {
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

			textPlayerBids[i].text = playerInputs[i].stamps.Count.ToString();
		}


		//only winner is given points per round in winnerElimination
		if(settings.winnerEliminated) {
			if(fastestPlayer >= 0) {

				int eliminated = 0;
				for(int i=0;i<playersEliminated.Length;i++) if(playersEliminated[i]) eliminated++;

				switch(eliminated) {
					case 1: scores[fastestPlayer] += settings.pointsFirstPlace; break;
					case 2: scores[fastestPlayer] += settings.pointsSecondPlace; break;
					case 3: scores[fastestPlayer] += settings.pointsThirdPlace; break;
					case 4: scores[fastestPlayer] += settings.pointsFourthPlace; break;
				}
			}

		}
		else {
			for(int i=0;i<playersThatAreCorrect.Count;i++) {
				int playerIndex = playersThatAreCorrect[i];
				if(playersEliminated[playerIndex]) continue;
				
				switch(i) {
					case 0: scores[playerIndex] += settings.pointsFirstPlace; break;
					case 1: scores[playerIndex] += settings.pointsSecondPlace; break;
					case 2: scores[playerIndex] += settings.pointsThirdPlace; break;
					case 3: scores[playerIndex] += settings.pointsFourthPlace; break;
				}
			}
		}
		
		if(playersThatAreCorrect.Count == 0) {
			if(roundCompleteNoWinner != null) AudioManager.PlayOneShot(roundCompleteNoWinner);
		}

		resultsDisplayIndex = 0;
		fadeTimer = scoreDisplayFillFade;
	}
	
	void Update_SPIN_COMPLETE() {
		if(playersThatAreCorrect.Count == 0) return;
		if(resultsDisplayIndex >= playersThatAreCorrect.Count) return;

		bool wasPositive = fadeTimer > 0f;

		fadeTimer -= Time.deltaTime;

		int playerIndex = playersThatAreCorrect[resultsDisplayIndex];

		Color col = playerPanelFills[playerIndex].color;
		
		if(fadeTimer > 0f) {
			col.a = Mathf.Lerp(scoreDisplayFillAlpha, 0f, fadeTimer / scoreDisplayFillFade);
		}
		else {
			col.a = Mathf.Lerp(scoreDisplayFillAlpha, scoreDisplayEmptyAlpha, Mathf.Abs(fadeTimer) / scoreDisplayFillFade);

			if(wasPositive) {
				textPlayerScores[playerIndex].text = "SCORE: " + scores[playerIndex].ToString();
				
				if(roundCompleteWinner != null) AudioManager.PlayOneShot(roundCompleteWinner);
			}
		}

		playerPanelFills[playerIndex].color = col;

		if(fadeTimer <= -scoreDisplayFillFade) {
			resultsDisplayIndex++;
			fadeTimer = scoreDisplayFillFade;
		}
	}

	void Exit_SPIN_COMPLETE() {
		for(int i=0;i<playerPanelFills.Length;i++) {
			Color col = playerPanelFills[i].color;
			col.a = 0f;
			playerPanelFills[i].color = col;
		}

		for(int i=0;i<textPlayerBids.Length;i++) {
			textPlayerBids[i].text = "";
		}

		for(int i=0;i<playerBidFlashAnimations.Length;i++) {
			playerBidFlashAnimations[i].Play();
		}
		
	}

	void PlaceTokens() {
		if(!settings.showTokens) {
			for(int i=0;i<playerTokens.Length;i++) {
				playerTokens[i].gameObject.SetActive(false);
			}
			return;
		}

		for(int i=0;i<playerTokens.Length && i<playerButtons.Length;i++) {
			playerTokens[i].gameObject.SetActive(i < numPlayers && !playersEliminated[i]);
			if(!playerTokens[i].gameObject.activeSelf) continue;

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
		for(int i=0;i<playerInputs.Count;i++) {
			playerInputs[i].stamps.Clear();
		}

		for(int i=0;i<playerMockBlocks.Length;i++) {
			playerMockBlocks[i].Initialize(CubeType.LIGHT_CUBE);
			playerMockBlocks[i].SetLights(Color.black, Color.black, Color.black, Color.black);
			textPlayerBids[i].text = "";
		}

		for(int i=0;i<playerBidFlashAnimations.Length;i++) {
			playerBidFlashAnimations[i].Stop();
			playerBidFlashAnimations[i].Rewind();
		}
	}

	void BlockTapped(ActiveBlock block) {
		for(int i=0;i<playerInputBlocks.Count;i++) {
			if(playerInputBlocks[i] != block) continue;
			PlayerInputTap(i);
			break;
		}
	}

///
	public void PlayerInputTap(int index) {
		if(state != GameState.PLAYING) return;
		if(playState != VortexState.SPINNING) return;
		if(playersEliminated[index]) return;

		while(index >= playerInputs.Count) playerInputs.Add (new VortexInput());
		if(playerInputs[index].stamps.Count >= 4) return;

		float time = Time.time;

		if(index != 1 && time - playerInputs[index].FirstTime > maxPlayerInputTime) return;

		playerInputs[index].stamps.Add(time);

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
		textPlayerBids[index].text = playerInputs[index].stamps.Count.ToString();

		playerBidFlashAnimations[index].Rewind();
		playerBidFlashAnimations[index].Play();

		Color fillCol = playerPanelFills[index].color;
		fillCol.a = 0.5f;
		playerPanelFills[index].color = fillCol;

		if(buttonPressSound != null) AudioManager.PlayOneShot(buttonPressSound);

		Debug.Log("PlayerInputTap index: "+index);

		if(playerInputs[index].stamps.Count == 1) {
			StartCoroutine(DelayBidLockedEffect(index));
		}
	}


	IEnumerator DelayBidLockedEffect(int index) {

		yield return new WaitForSeconds(maxPlayerInputTime);

		imageInputLocked[index].gameObject.SetActive(true);
	}

}
