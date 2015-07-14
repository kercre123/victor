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
		public int pointsIncorrectPenalty = -5;
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
	[SerializeField] Image imageHub;
	[SerializeField] Text textCurrentNumber;
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
	[SerializeField] RectTransform[] preGameNotices;
	[SerializeField] RectTransform preGameAlert;
	[SerializeField] Animation[] playerBidFlashAnimations;
	[SerializeField] Text[] textPlayerScores;
	[SerializeField] Text[] textPlayerScoreDeltas;

	[SerializeField] VortexSettings[] settingsPerLevel;

	[SerializeField] LightningBoltShapeSphereScript lightingBall;
	[SerializeField] float[] wheelLightningRadii  = { 3.5f, 2f, 1f };
	[SerializeField] int lightningMinCountAtSpeedMax = 8;
	[SerializeField] int lightningMaxCountAtSpeedMax = 16;
	[SerializeField] Image imageGear;
	[SerializeField] Color gearDefaultColor = Color.white;
	[SerializeField] Color[] playerColors = { Color.blue, Color.green, Color.yellow, Color.red };

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
	List<int> playersThatAreWrong = new List<int>();
	
	[SerializeField] float scoreDisplayFillFade = 0.5f;
	[SerializeField] float scoreDisplayFillAlpha = 0.5f;
	[SerializeField] float scoreDisplayEmptyAlpha = 0.1f;
	
	float fadeTimer = 1f;
	int resultsDisplayIndex = 0;

	List<int> scoreDeltas = new List<int>();

	bool fakeCozmoTaps = true;

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

		if(imageCozmoDragHand != null) {
			cozmoDragHandRectTransform = imageCozmoDragHand.transform.parent as RectTransform;
			imageCozmoDragHand.gameObject.SetActive(false);
		}

		if(imageHub != null) {
			imageHub.gameObject.SetActive(false);
		}
		
		if(preGameNotices != null) {
			foreach(RectTransform rTrans in preGameNotices) rTrans.gameObject.SetActive(false);
		}

		if(preGameAlert != null) preGameAlert.gameObject.SetActive(false);

		ActiveBlock.TappedAction += BlockTapped;

		foreach(Text text in textPlayerScoreDeltas) text.gameObject.SetActive(false);
		
		imageGear.color = gearDefaultColor;

		while(playerInputs.Count < numPlayers) playerInputs.Add (new VortexInput());
	}

	protected override void OnDisable () {
		base.OnDisable();

		ActiveBlock.TappedAction -= BlockTapped;

		//revert to single player incase this ever matters to other games
		PlayerPrefs.SetInt("NumberOfPlayers", 1);
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

	ObservedObject humanHead;
	protected override void Enter_PRE_GAME () {
		
		if(preGameNotices != null) {
			for(int i=0;i<preGameNotices.Length;i++) {
				preGameNotices[i].gameObject.SetActive(i < numPlayers);
			}
		}

		if(playerMockBlocks != null) {
			for(int i=0;i < numPlayers;i++) {
				playerMockBlocks[i].Validate(false);
				playerMockBlocks[i].Initialize(CubeType.LIGHT_CUBE);
				Color col = CozmoPalette.instance.GetColorForActiveBlockMode(GetPlayerActiveCubeColorMode(i));
				playerMockBlocks[i].SetLights(col);
			}
		}

		for(int i=0;i<playerButtonCanvasGroups.Length;i++) {
			if(i == 1) continue; // player 2 is cozmo, no button
			playerButtonCanvasGroups[i].interactable = i < numPlayers;
			playerButtonCanvasGroups[i].blocksRaycasts = i < numPlayers;
		}

		if(preGameAlert != null) preGameAlert.gameObject.SetActive(true);

		ClearInputs();

		base.Enter_PRE_GAME();

		playerInputBlocks.Clear();
		
		humanHead = null;
		
		if( robot != null && PlayerPrefs.GetInt("DebugSkipLayoutTracker",0) == 0) {
			

			//robot.DriveWheels(-CozmoUtil.MAX_WHEEL_SPEED_MM, CozmoUtil.MAX_WHEEL_SPEED_MM);

			for(int i=0;i<robot.knownObjects.Count;i++) {
				if(!robot.knownObjects[i].isActive) continue;

				ActiveBlock block = robot.knownObjects[i] as ActiveBlock;
				//Debug.Log("adding playerInputBlocks["+i+"] with PlayerInputTap("+i+")");

				block.SetMode(GetPlayerActiveCubeColorMode(playerInputBlocks.Count));

				playerInputBlocks.Add(block);
			}

			for(int i=0;i<robot.knownObjects.Count;i++) {
				if(!robot.knownObjects[i].isFace) continue;
				humanHead = robot.knownObjects[i];
				break;
			}
			
			if(humanHead == null) {
				float height = 0.75f;
				robot.SetHeadAngle(height);
			}
			
		}

		headTimer = 0.1f;

		imageGear.color = gearDefaultColor;
	}

	float frequency = 0.1f;
	float headTimer = 0f;
	float turnStartAngle = 0f;
	protected override void Update_PRE_GAME () {
		base.Update_PRE_GAME();

		if( robot != null) {

			if(robot.carryingObject == null && !robot.isBusy) {
				robot.PickAndPlaceObject(playerInputBlocks[1]);
				if(CozmoBusyPanel.instance != null)	{
					string desc = "Cozmo is attempting to pick-up\n a game cube.";
					CozmoBusyPanel.instance.SetDescription(desc);
				}
			}
			else if(!robot.isBusy && !playerMockBlocks[1].Validated) {
				robot.TapBlockOnGround(1);
			}
			else if(humanHead == null) {
				
				for(int i=0;i<robot.knownObjects.Count;i++) {
					if(!robot.knownObjects[i].isFace) continue;
					humanHead = robot.knownObjects[i];
					break;
				}
				
//				if(humanHead == null) {
//					if(headTimer > 0f) {
//						headTimer -= Time.deltaTime;
//						
//						if(headTimer <= 0f) {
//							//oscillate
//							//float height = 1f + Mathf.Sin(2*Mathf.PI*frequency*Time.time) * 0.5f;
//							turnStartAngle = robot.poseAngle_rad * Mathf.Rad2Deg;
//							Debug.Log("DriveWheels turn!");
//							robot.DriveWheels(-CozmoUtil.MAX_WHEEL_SPEED_MM*0.1f, CozmoUtil.MAX_WHEEL_SPEED_MM*0.1f);
//							
//						}
//					}
//					else if(Mathf.Abs(MathUtil.AngleDelta(turnStartAngle, robot.poseAngle_rad * Mathf.Rad2Deg)) > 5f) {
//						Debug.Log("DriveWheels stop!");
//						robot.DriveWheels(0f, 0f);
//						headTimer = 1f;
//					}
//				}
			}
			
		}
	}

	protected override void Exit_PRE_GAME () {
		base.Exit_PRE_GAME();

		if(preGameNotices != null) {
			for(int i=0;i<preGameNotices.Length;i++) {
				preGameNotices[i].gameObject.SetActive(false);
			}
		}

		if(preGameAlert != null) preGameAlert.gameObject.SetActive(false);

		for(int i=0;i<playerButtonCanvasGroups.Length;i++) {
			playerButtonCanvasGroups[i].interactable = false;
			playerButtonCanvasGroups[i].blocksRaycasts = false;
		}
	}

	protected override void Enter_PLAYING () {
		for(int i=0;i<textPlayerScores.Length;i++) {
			textPlayerScores[i].text = "SCORE: 0";
		}

		ClearInputs();
		playState = VortexState.INTRO;
		EnterPlayState();

		if(imageHub != null) {
			imageHub.gameObject.SetActive(true);
		}

		if(robot != null) {
			//turn back towards ipad
			robot.SetLiftHeight(1f);
			//robot.TrackToObject(null, false);
		}

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

		if(robot != null) {
			if(humanHead == null) {
				
				for(int i=0;i<robot.knownObjects.Count;i++) {
					if(!robot.knownObjects[i].isFace) continue;
					humanHead = robot.knownObjects[i];
					break;
				}

//			if(playState != VortexState.SPINNING && robot.headTrackingObject == null && humanHead != null) {

//				robot.SetLiftHeight(0.1f);
//				robot.TrackHeadToObject(humanHead, true);
			}
		}


	}
	
	protected override void Exit_PLAYING (bool overrideStars = false) {
		base.Exit_PLAYING();

		ExitPlayState();

		if(imageHub != null) {
			imageHub.gameObject.SetActive(false);
		}
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
			scoreData.color = CozmoPalette.instance.GetColorForActiveBlockMode(GetPlayerActiveCubeColorMode(i));

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
		
		if(robot != null) {
			if(humanHead == null) {
				
				for(int i=0;i<robot.knownObjects.Count;i++) {
					if(!robot.knownObjects[i].isFace) continue;
					humanHead = robot.knownObjects[i];
					break;
				}
			}
			Debug.Log("Enter_RESULTS robot.SetLiftHeight(0.1f);");
			//robot.SetLiftHeight(0.1f);

			if(humanHead != null) {
				robot.FaceObject(humanHead, false);
			}
		}

	}

	protected override void Update_RESULTS() {
		base.Update_RESULTS();

		
		if(robot != null) {
			if(humanHead == null) {
				
				for(int i=0;i<robot.knownObjects.Count;i++) {
					if(!robot.knownObjects[i].isFace) continue;
					humanHead = robot.knownObjects[i];
					break;
				}
			}

			//Debug.Log( "Update_RESULTS robot.isBusy("+robot.isBusy+")" );
			if(!robot.isBusy && humanHead != null && robot.headTrackingObjectID < 0) {
				//Debug.Log( "Update_RESULTS robot.TrackToObject(humanHead, false);" );
				
				robot.TrackToObject(humanHead, false);
			}
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
			
			for(int i=0;i<numPlayers;i++) {
				if(!playerMockBlocks[i].Validated) return false;
			}

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
					//Debug.Log("cozmo SpinUnderway");
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
		currentPlayerIndex = UnityEngine.Random.Range(0, numPlayers) - 1;
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

	[SerializeField] Image imageCozmoDragHand;
	[SerializeField] Vector2 cozmoStartDragPos = Vector2.zero;
	[SerializeField] Vector2 cozmoEndDragPos = Vector2.zero;
	[SerializeField] float cozmoDragDelay = 2f;
	[SerializeField] float cozmoDragTime = 2f;
	[SerializeField] float cozmoFinalDragTime = 0.2f;

	RectTransform cozmoDragHandRectTransform;
	Vector2 startDragPos;
	Vector2 endDragPos;
	float dragTimer = 0f;
	int dragCount = 0;
	int dragCountMax = 2;

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
			wheel.AutomatedMode();

			startDragPos = new Vector2(cozmoStartDragPos.x * Screen.width, cozmoStartDragPos.y * Screen.height);
			startDragPos += UnityEngine.Random.insideUnitCircle * Screen.height * 0.1f;

			endDragPos = new Vector2(cozmoEndDragPos.x * Screen.width, cozmoEndDragPos.y * Screen.height);
			endDragPos += UnityEngine.Random.insideUnitCircle * Screen.height * 0.1f;
			dragTimer = cozmoDragTime;
			dragCount = 0;
			dragCountMax = UnityEngine.Random.Range(2,4);
			wheel.DragStart(startDragPos, Time.time);

//			Vector3 startWorld = Camera.main.ScreenToWorldPoint(startDragPos);
//			Vector3 endWorld = Camera.main.ScreenToWorldPoint(startDragPos);
//			startWorld.z = 1f;
//			endWorld.z = 1f;
//			Debug.DrawLine(startWorld, endWorld, (dragCount % 2 == 0) ? Color.green : Color.blue, 30f);
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

		imageGear.color = GetPlayerUIColor(currentPlayerIndex);
	}

	void Update_REQUEST_SPIN() {
		
		//cozmo's automated spinWheel dragging
		if(currentPlayerIndex == 1 && playStateTimer > cozmoDragDelay) {

			dragTimer -= Time.deltaTime;
		
			if(dragTimer <= 0f) {

				Vector2 temp = startDragPos;
				startDragPos = endDragPos;
				endDragPos = temp;
				dragTimer = cozmoDragTime;
				dragCount++;

				if(dragCount >= dragCountMax) {
					Vector2 delta = endDragPos - startDragPos;
					endDragPos += delta * 2f;
					dragTimer = cozmoFinalDragTime;
				}
				//Debug.Log("frame("+Time.frameCount+") Update_REQUEST_SPIN dragCount("+dragCount+")");
			}

			//we drag back and forth some to make it obvious coz is spinning
			if(dragCount < dragCountMax) {
				float factor = 1f - Mathf.Clamp01(dragTimer / cozmoDragTime);
				//Debug.Log("frame("+Time.frameCount+") Update_REQUEST_SPIN DragUpdate factor("+factor+")");
				Vector2 currentPos = Vector2.Lerp(startDragPos, endDragPos, factor);
				wheel.DragUpdate(currentPos, Time.time);
				PlaceCozmoTouch(currentPos);
			}
			else {
				float factor = 1f - Mathf.Clamp01(dragTimer / cozmoFinalDragTime);
				Vector2 currentPos = Vector2.Lerp(startDragPos, endDragPos, factor);
				wheel.DragUpdate(currentPos, Time.time);

				//Debug.Log("frame("+Time.frameCount+") Update_REQUEST_SPIN DragEnd dragCount("+dragCount+")");
				if(factor > 0.7f) {
					wheel.DragEnd();
					if(imageCozmoDragHand != null) {
						imageCozmoDragHand.gameObject.SetActive(false);
					}
				}
				else {	
					PlaceCozmoTouch(currentPos);
				}
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
		if(imageCozmoDragHand != null) {
			imageCozmoDragHand.gameObject.SetActive(false);
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
		cozmoTapsSubmitted = -1;

		foreach(ActiveBlock block in playerInputBlocks) block.SetMode(ActiveBlock.Mode.Off);
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

		if(cozmoTapsSubmitted < 0 && predictedNum > 0) {
			float time = Time.time - wheel.SpinStartTime;
			float timeToBid = predictedDuration - predictedTimeAfterLastPeg - (1f + predictedNum * cozmoTimePerTap);
			if(time > timeToBid) {

				if(robot != null) {
					robot.TapBlockOnGround(predictedNum);
					cozmoTapsSubmitted = predictedNum;
				}
				else {
				//if(fakeCozmoTaps) {
					cozmoTapsSubmitted = 0;
				}
				//Debug.Log("cozmo predictedNum("+predictedNum+") time("+time+") timeToBid("+timeToBid+") predictedDuration("+predictedDuration+")");
			}
		}
		else if(predictedNum > 0 && cozmoTapsSubmitted < predictedNum) {
			if(Time.time - playerInputs[1].FinalTime >= cozmoTimePerTap) {
				PlayerInputTap(1);
				cozmoTapsSubmitted++;
				//Debug.Log("cozmo predictedNum("+predictedNum+") cozmoTapsSubmitted("+cozmoTapsSubmitted+")");
			}
		}
		
		for(int i=0;i<playerPanelFills.Length;i++) {
			if(i >= numPlayers || playerPanelFills[i].color.a == 0f) continue;
			Color col = playerPanelFills[i].color;
			col.a = Mathf.Max(0f, col.a - Time.deltaTime * 4f);
			playerPanelFills[i].color = col;
		}

		imageGear.color = Color.Lerp (GetPlayerUIColor(currentPlayerIndex), gearDefaultColor, playStateTimer);

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


		for(int i=0;i<playerPanelFills.Length;i++) {
			if(i >= numPlayers || playerPanelFills[i].color.a == 0f) continue;
			Color col = playerPanelFills[i].color;
			col.a = 0f;
			playerPanelFills[i].color = col;
		}

		for(int i=0;i<playerInputBlocks.Count && i<numPlayers;i++) {
			playerInputBlocks[i].SetMode(GetPlayerActiveCubeColorMode(i));
		}

	}

	[SerializeField] float scoreScaleBase = 0.75f;
	[SerializeField] float scoreScaleMax = 1f;

	void Enter_SPIN_COMPLETE() {
		
		playersThatAreCorrect.Clear();
		playersThatAreWrong.Clear();
		int fastestPlayer = -1;

		scoreDeltas.Clear();
		while(scoreDeltas.Count < scores.Count) scoreDeltas.Add(0);

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

		for(int i=0;i<numPlayers;i++) {
			if(playersEliminated[i]) continue;
			if(playersThatAreCorrect.Contains (i)) continue;
			playersThatAreWrong.Add (i);
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

			textPlayerBids[i].text = playerInputs[i].stamps.Count.ToString();

			if(playerInputBlocks.Count <= i) continue;

			Color playerColor = CozmoPalette.instance.GetColorForActiveBlockMode(GetPlayerActiveCubeColorMode(i));

			Color c1 = Color.clear;
			Color c2 = Color.clear;
			Color c3 = Color.clear;
			Color c4 = Color.clear;

			if(playersThatAreCorrect.Contains(i)) {
				int orderIndex = playersThatAreCorrect.IndexOf(i);
				if(orderIndex > 2) {
					c4 = playerColor;
				}
				if(orderIndex > 1) {
					c3 = playerColor;
				}
				if(orderIndex > 0) {
					c2 = playerColor;
				}

				c1 = playerColor;
			}
			
			uint uColor1 = CozmoPalette.ColorToUInt(c1);
			uint uColor2 = CozmoPalette.ColorToUInt(c2);
			uint uColor3 = CozmoPalette.ColorToUInt(c3);
			uint uColor4 = CozmoPalette.ColorToUInt(c4);

			playerInputBlocks[i].SetLEDs(uColor1, 0, (byte)ActiveBlock.Light.IndexToPosition(0), 250, 250, 0, 0, 0 );
			playerInputBlocks[i].SetLEDs(uColor2, 0, (byte)ActiveBlock.Light.IndexToPosition(1), 250, 250, 0, 0, 0 );
			playerInputBlocks[i].SetLEDs(uColor3, 0, (byte)ActiveBlock.Light.IndexToPosition(2), 250, 250, 0, 0, 0 );
			playerInputBlocks[i].SetLEDs(uColor4, 0, (byte)ActiveBlock.Light.IndexToPosition(3), 250, 250, 0, 0, 0 );
		}


		//only winner is given points per round in winnerElimination
		if(settings.winnerEliminated) {
			if(fastestPlayer >= 0) {

				int eliminated = 0;
				for(int i=0;i<playersEliminated.Length;i++) if(playersEliminated[i]) eliminated++;

				int delta = 0;
				switch(eliminated) {
					case 1: delta = settings.pointsFirstPlace; break;
					case 2: delta = settings.pointsSecondPlace; break;
					case 3: delta = settings.pointsThirdPlace; break;
					case 4: delta = settings.pointsFourthPlace; break;
				}


				scoreDeltas[fastestPlayer] = delta;
				scores[fastestPlayer] += delta;
			}

		}
		else {
			for(int i=0;i<playersThatAreCorrect.Count;i++) {
				int playerIndex = playersThatAreCorrect[i];
				if(playersEliminated[playerIndex]) continue;
				int delta = 0;
				
				switch(i) {
					case 0: delta = settings.pointsFirstPlace; break;
					case 1: delta = settings.pointsSecondPlace; break;
					case 2: delta = settings.pointsThirdPlace; break;
					case 3: delta = settings.pointsFourthPlace; break;
				}

				scoreDeltas[playerIndex] = delta;
				scores[playerIndex] += delta;
			}
		}

		for(int i=0;i<playersThatAreWrong.Count;i++) {
			scoreDeltas[playersThatAreWrong[i]] = settings.pointsIncorrectPenalty;
			scores[playersThatAreWrong[i]] = Mathf.Max (0, scores[playersThatAreWrong[i]] + settings.pointsIncorrectPenalty);
		}
		
		if(playersThatAreCorrect.Count == 0) {
			if(roundCompleteNoWinner != null) AudioManager.PlayOneShot(roundCompleteNoWinner);
		}

		resultsDisplayIndex = 0;
		fadeTimer = scoreDisplayFillFade;
	}
	
	void Update_SPIN_COMPLETE() {
		if(playersThatAreCorrect.Count == 0) return;
		if(resultsDisplayIndex > playersThatAreCorrect.Count) return;

		bool wasPositive = fadeTimer > 0f;
		fadeTimer -= Time.deltaTime;
		Color col = Color.black;

		//if done with correct players, let's display all the losers at once
		if(resultsDisplayIndex == playersThatAreCorrect.Count) {

			for(int i=0;i<playersThatAreWrong.Count;i++) {

				int loserIndex = playersThatAreWrong[i];
				
				col = playerPanelFills[loserIndex].color;
				
				if(fadeTimer > 0f) {
					float factor = fadeTimer / scoreDisplayFillFade;
					col.a = Mathf.Lerp(scoreDisplayFillAlpha, 0f, factor);
					textPlayerScores[loserIndex].rectTransform.localScale = Vector3.Lerp(Vector3.one*scoreScaleMax, Vector3.one*scoreScaleBase, factor);
				}
				else {
					float factor = Mathf.Abs(fadeTimer) / scoreDisplayFillFade;
					col.a = Mathf.Lerp(scoreDisplayFillAlpha, scoreDisplayEmptyAlpha, factor);
					textPlayerScores[loserIndex].rectTransform.localScale = Vector3.Lerp(Vector3.one*scoreScaleMax, Vector3.one*scoreScaleBase, factor);
					
					if(wasPositive) {
						
						textPlayerScoreDeltas[loserIndex].text = scoreDeltas[loserIndex] +"!";
						textPlayerScoreDeltas[loserIndex].gameObject.SetActive(true);
						
						textPlayerScores[loserIndex].text = "SCORE: " + scores[loserIndex].ToString();
						
						if(roundCompleteWinner != null) AudioManager.PlayOneShot(roundCompleteWinner);
					}
				}
			
				playerPanelFills[loserIndex].color = col;
			}
			
			if(fadeTimer <= -scoreDisplayFillFade) {
				resultsDisplayIndex++;
			}

			return;
		}

		int playerIndex = playersThatAreCorrect[resultsDisplayIndex];

		col = playerPanelFills[playerIndex].color;
		
		if(fadeTimer > 0f) {
			float factor = fadeTimer / scoreDisplayFillFade;
			col.a = Mathf.Lerp(scoreDisplayFillAlpha, 0f, factor);
			textPlayerScores[playerIndex].rectTransform.localScale = Vector3.Lerp(Vector3.one*scoreScaleMax, Vector3.one*scoreScaleBase, factor);
		}
		else {
			float factor = Mathf.Abs(fadeTimer) / scoreDisplayFillFade;
			col.a = Mathf.Lerp(scoreDisplayFillAlpha, scoreDisplayEmptyAlpha, factor);
			textPlayerScores[playerIndex].rectTransform.localScale = Vector3.Lerp(Vector3.one*scoreScaleMax, Vector3.one*scoreScaleBase, factor);

			if(wasPositive) {

				textPlayerScoreDeltas[playerIndex].text = "+" + scoreDeltas[playerIndex] +"!";
				textPlayerScoreDeltas[playerIndex].gameObject.SetActive(true);

				textPlayerScores[playerIndex].text = "SCORE: " + scores[playerIndex].ToString();
				
				if(roundCompleteWinner != null) AudioManager.PlayOneShot(roundCompleteWinner);
			}
		}

		playerPanelFills[playerIndex].color = col;

		if(fadeTimer <= -scoreDisplayFillFade) {
			resultsDisplayIndex++;
			fadeTimer = scoreDisplayFillFade;
			//textPlayerScoreDeltas[playerIndex].gameObject.SetActive(false);
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
		
		for(int i=0;i<playerInputBlocks.Count && i<numPlayers;i++) {
			playerInputBlocks[i].SetMode(GetPlayerActiveCubeColorMode(i));
		}

		for(int i=0;i<textPlayerScores.Length && i<numPlayers;i++) {
			textPlayerScores[i].rectTransform.localScale = Vector3.one*scoreScaleBase;
		}

		for(int i=0;i<textPlayerScoreDeltas.Length && i<numPlayers;i++) {
			textPlayerScoreDeltas[i].gameObject.SetActive(false);
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
//			playerMockBlocks[i].Initialize(CubeType.LIGHT_CUBE);
//			playerMockBlocks[i].SetLights(Color.black);
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

	void PlaceCozmoTouch(Vector2 screenPos) {
		if(imageCozmoDragHand == null) return;

		imageCozmoDragHand.gameObject.SetActive(true);
		RectTransform rTrans = cozmoDragHandRectTransform;
		Camera cam = imageCozmoDragHand.canvas.renderMode != RenderMode.ScreenSpaceOverlay ? imageCozmoDragHand.canvas.worldCamera : null;
		Vector2 anchor;
		RectTransformUtility.ScreenPointToLocalPointInRectangle(rTrans, screenPos, cam, out anchor);
		imageCozmoDragHand.rectTransform.anchoredPosition = anchor;
		//Debug.Log("PlaceCozmoTouch screenPos("+screenPos+") anchor("+anchor+")");
	}

	ActiveBlock.Mode GetPlayerActiveCubeColorMode(int index) {

		switch(index) {
			case 0: return ActiveBlock.Mode.Blue;
			case 1: return ActiveBlock.Mode.Green;
			case 2: return ActiveBlock.Mode.Yellow;
			case 3: return ActiveBlock.Mode.Red;
		}

		return ActiveBlock.Mode.Off;
	}

	Color GetPlayerUIColor(int index) {
		return playerColors[index];
	}


	
///
	public void PlayerInputTap(int index) {
		if(state == GameState.PRE_GAME) {
			playerMockBlocks[index].Validate(true);
			return;
		}


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

		if(index < playerInputBlocks.Count) {
			Color playerColor = CozmoPalette.instance.GetColorForActiveBlockMode(GetPlayerActiveCubeColorMode(index));
	
			uint c1 = CozmoPalette.ColorToUInt(playerInputs[index].stamps.Count > 0 ? playerColor : Color.black);
			uint c2 = CozmoPalette.ColorToUInt(playerInputs[index].stamps.Count > 1 ? playerColor : Color.black);
	        uint c3 = CozmoPalette.ColorToUInt(playerInputs[index].stamps.Count > 2 ? playerColor : Color.black);
	        uint c4 = CozmoPalette.ColorToUInt(playerInputs[index].stamps.Count > 3 ? playerColor : Color.black);
	
			playerInputBlocks[index].SetLEDs(c1, 0, (byte)ActiveBlock.Light.IndexToPosition(0), Robot.Light.FOREVER, 0, 0, 0, 0 );
			playerInputBlocks[index].SetLEDs(c2, 0, (byte)ActiveBlock.Light.IndexToPosition(1), Robot.Light.FOREVER, 0, 0, 0, 0 );
			playerInputBlocks[index].SetLEDs(c3, 0, (byte)ActiveBlock.Light.IndexToPosition(2), Robot.Light.FOREVER, 0, 0, 0, 0 );
			playerInputBlocks[index].SetLEDs(c4, 0, (byte)ActiveBlock.Light.IndexToPosition(3), Robot.Light.FOREVER, 0, 0, 0, 0 );
		}

		textPlayerBids[index].text = playerInputs[index].stamps.Count.ToString();

		playerBidFlashAnimations[index].Rewind();
		playerBidFlashAnimations[index].Play();

		Color fillCol = playerPanelFills[index].color;
		fillCol.a = 0.5f;
		playerPanelFills[index].color = fillCol;

		if(buttonPressSound != null) AudioManager.PlayOneShot(buttonPressSound);

		//Debug.Log("PlayerInputTap index: "+index);

		if(playerInputs[index].stamps.Count == 1) {
			StartCoroutine(DelayBidLockedEffect(index));
		}
	}


	IEnumerator DelayBidLockedEffect(int index) {

		yield return new WaitForSeconds(maxPlayerInputTime);

		imageInputLocked[index].gameObject.SetActive(true);
	}

}
