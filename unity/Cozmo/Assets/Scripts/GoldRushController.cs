//#define RUSH_DEBUG
using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.Cozmo;
using System;

public class GoldRushController : GameController {
	public static GoldRushController instance = null;

	public const uint COLOR_RED = 0xFF0000FF;

	[SerializeField] protected AudioClip foundBeep;
	[SerializeField] protected AudioClip collectedSound;
	[SerializeField] protected AudioClip pickupEnergyScanner;
	[SerializeField] protected AudioClip placeEnergyScanner;
	[SerializeField] protected AudioClip findEnergy;
	[SerializeField] protected AudioClip dropEnergy;
	[SerializeField] protected AudioClip timeUp;

	[SerializeField] protected AudioClip timeExtension;
	[SerializeField] protected Button extractButton = null;
	public float detectRangeDelayFar = 2.0f;
	public float detectRangeDelayClose = .2f;
	public float light_messaging_delay = .05f;
	private float last_light_message_time = -1;

	private int numDrops = 0;

	[SerializeField] float hideRadius;	//radius around cube's initial position in which its gold will be buried
	[SerializeField] float findRadius;	//dropping cube within find radius will trigger transmutation/score
	[SerializeField] float returnRadius;	//dropping cube within find radius will trigger transmutation/score
	[SerializeField] float detectRadius; //pulsing will accelerate from detect to find ranges
	[SerializeField] float extractionTime = 1.5f; //time it takes to extract
	[SerializeField] float rewardTime = 1.5f; //time it takes to reward
	[SerializeField] int numDropsForBonusTime = 1;

	[SerializeField] float baseTimeBonus = 30f;
	[SerializeField] float bonusTimeDecay = 2f / 3f;

	[SerializeField] Text resultsScore; //for the results screen

	//List<ObservedObject> goldCubes = new List<ObservedObject>();
	Dictionary<int, Vector2> buriedLocations = new Dictionary<int, Vector2>();
	List<int> foundItems = new List<int>();
	int lastCarriedObjectId = -1;
	[System.NonSerialized] public ObservedObject goldExtractingObject = null;
	[System.NonSerialized] public ObservedObject goldCollectingObject = null;
	int baseObjectId = -1;
	ScreenMessage hintMessage;
	private bool audioLocatorEnabled = true;
	byte last_leds = 0xFF;
	bool sent = false;

	enum PlayState
	{
		IDLE,
		SEARCHING,
		CAN_EXTRACT,
		EXTRACTING,
		READY_TO_RETURN,
		RETURNING,
		RETURNED,
		DEPOSITING,
		NUMSTATES
	};

	private PlayState playState = PlayState.IDLE;
	float playStateTimer = 0;
	float totalActiveTime = 0; // only increments when the robot is searching for or returing gold


	internal bool inExtractRange { get { return playState == PlayState.CAN_EXTRACT; } }
	internal bool inDepositRange { get { return playState == PlayState.RETURNED; } }

	enum BuildState
	{
		WAITING_TO_PICKUP_BLOCK,
		WAITING_FOR_STACK,
		WAITING_FOR_PLAY,
		NUMSTATES
	};

	private BuildState buildState = BuildState.WAITING_TO_PICKUP_BLOCK;


	//int lastHeldID;
	int instance_count = 0;
	void Awake()
	{
		hintMessage = GetComponentInChildren<ScreenMessage> ();
		extractButton.gameObject.SetActive (false);
		instance = this;
		instance_count++;
	}

	protected override void OnEnable()
	{
		base.OnEnable();

		instance = this;

		MessageDelay = .05f;
	}

	protected override void OnDisable()
	{
		base.OnDisable();

		if( instance == this ) instance = null;

		RobotEngineManager.instance.SuccessOrFailure -= CheckForStackSuccess;
	}

	protected override void RefreshHUD ()
	{
		base.RefreshHUD ();

		// need timer to reflect our games unique use of it
		if (textTime != null && state == GameState.PLAYING) 
		{
			textTime.text = Mathf.FloorToInt (maxPlayTime+bonusTime - totalActiveTime).ToString ();
		}

		if (textScore != null && scores != null && scores.Length > 0 && state == GameState.PLAYING) 
		{
			textScore.text = "score: " + scores [0].ToString ();	
		} 
		else 
		{
			textScore.text = string.Empty;
		}
	}

	protected override void Enter_PLAYING() 
	{
		base.Enter_PLAYING();
		playState = PlayState.IDLE;
		playStateTimer = 0;
		totalActiveTime = 0;
		scores [0] = 0;
		numDrops = 0;

		CozmoVision.EnableDing(false); // just in case we were in searching mode
		SetEnergyBars (0, 0);
	}

	protected override void Exit_PLAYING()
	{
		base.Exit_PLAYING();
		PlayNotificationAudio (timeUp);
		resultsScore.text = "Score: " + scores [0];
		CozmoVision.EnableDing(); // just in case we were in searching mode
		//ActionButton.DROP = null;
		//RobotEngineManager.instance.SuccessOrFailure -= CheckForGoldDropOff;
		playState = PlayState.IDLE;
		UpdateDetectorLights (0);
		audio.Stop ();
		SetEnergyBars (0, 0);
	}

	protected override void Update_PLAYING() 
	{
		base.Update_PLAYING();

		playStateTimer += Time.deltaTime;

		UpdateTimerEvents (totalActiveTime);

		if( robot != null )
		{
			foreach(ObservedObject obj in robot.knownObjects)
			{
				if( obj.Family != 3 )
				{
					goldCollectingObject = obj;
				}
			}
		}

		switch(playState)
		{
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
		default:
			break;
		}
	}

	protected override void Enter_BUILDING ()
	{
		base.Enter_BUILDING ();

		if(RobotEngineManager.instance == null) return;

		//GameLayoutTracker.instance.ValidateBuild ();
		lastCarriedObjectId = -1;
		goldExtractingObject = null;
		playButton.gameObject.SetActive (false);
		buildState = BuildState.WAITING_TO_PICKUP_BLOCK;

		robot = RobotEngineManager.instance.current;
		RobotEngineManager.instance.SuccessOrFailure += CheckForStackSuccess;

		goldCollectingObject = null;

		hintMessage.ShowMessage("Pick up the energy scanner to begin", Color.black);
		PlayNotificationAudio (pickupEnergyScanner);
	}

	protected override void Exit_BUILDING ()
	{
		RobotEngineManager.instance.SuccessOrFailure -= CheckForStackSuccess;
		hintMessage.KillMessage ();
		base.Exit_BUILDING ();
	}

	protected override void Update_BUILDING ()
	{
		base.Update_BUILDING ();
		if( robot != null )
		{
			foreach(ObservedObject obj in robot.knownObjects)
			{
				if( obj.Family == 3 && goldExtractingObject == null )
				{
					goldExtractingObject = obj;
					goldExtractingObject.SetActiveObjectLEDs(1, COLOR_RED, 0, 0xFF, 150, 150); 
				}
				else if( obj.Family != 3 )
				{
					goldCollectingObject = obj;
				}
			}
		}
#if RUSH_DEBUG
		if (goldExtractingObject != null && robot.carryingObject != null ) 
		{
			UpdateDirectionLights(Vector2.zero);
		}

		if( Input.GetKeyDown(KeyCode.C ))
		{
			StartCoroutine(CountdownToPlay());
		}

#endif
	}

	void CheckForStackSuccess(bool success, ActionCompleted action_type)
	{
		Debug.Log ("action type is: " + action_type);
		if( success ) // hardcoded until we get enums over from the engine
		{
			switch(buildState)
			{
			case BuildState.WAITING_TO_PICKUP_BLOCK:
				if( (int)action_type == 5 || (int)action_type == 6 )
				{
					// picked up our detector block (will need to verify that it's an active block later)
					buildState = BuildState.WAITING_FOR_STACK;
					goldExtractingObject = robot.carryingObject;
					//UpdateDetectorLights (1);
					goldExtractingObject.SetActiveObjectLEDs(0); 
					hintMessage.ShowMessage("Place the scanner on the transformer", Color.black);
					PlayNotificationAudio(placeEnergyScanner);
				}
				break;
			case BuildState.WAITING_FOR_STACK:
				if( (int)action_type == 8 )
				{
					// stacked our detector block
					buildState = BuildState.WAITING_FOR_PLAY;
					PlayNotificationAudio(pickupEnergyScanner);
					hintMessage.ShowMessage("Pick up the scanner to begin play", Color.black);
				}
				break;
			case BuildState.WAITING_FOR_PLAY:
				if( (int)action_type == 6 )
				{
					// start the game
					StartCoroutine(CountdownToPlay());
					//PlayRequested();
					hintMessage.KillMessage();
				}
				break;
			default:
				break;
			}
		}
	}

	protected override bool IsGameReady() 
	{
		//if(!base.IsGameReady()) return false;
		if(RobotEngineManager.instance == null) return false;
		if(RobotEngineManager.instance.current == null) return false;

		return true;
	}

	protected override bool IsGameOver() 
	{
		//if(base.IsGameOver()) return true;
		if(maxPlayTime > 0f && totalActiveTime >= maxPlayTime + bonusTime  ) return true;

		//game specific end conditions...
		return false;
	}

	void EnableAudioLocator(bool on)
	{
		audioLocatorEnabled = on;
	}

	void UpdateLocatorSound(float current_rate) 
	{
		if( audioLocatorEnabled )
		{
			float timeSinceLast = Time.realtimeSinceStartup - lastPlayTime;
			audio.loop = false;

			if (timeSinceLast >= current_rate) 
			{
				audio.Stop();
				gameObject.audio.Play();

				lastPlayTime = Time.realtimeSinceStartup;
			}
		}
	}

	void EnterPlayState(PlayState new_state)
	{
		ExitPlayState (playState);
		playStateTimer = 0;
		playState = new_state;
		switch (playState) {
		case PlayState.IDLE:
			break;
		case PlayState.SEARCHING:
			robot.SetHeadAngle(0);
			PlayNotificationAudio(findEnergy);
			foundItems.Clear();
			buriedLocations.Clear();
			Vector2 randomSpot = new Vector2(UnityEngine.Random.value, UnityEngine.Random.value); 
			buriedLocations[robot.carryingObject] = randomSpot * hideRadius;
			break;
		case PlayState.CAN_EXTRACT:
			if ( goldExtractingObject != null )
			{
				goldExtractingObject.SetActiveObjectLEDs(1, COLOR_RED, 0, 0xFF, 150, 150); 
			}
			break;
		case PlayState.EXTRACTING:
			StartCoroutine(StartExtracting());
			break;
		case PlayState.READY_TO_RETURN:
			hintMessage.ShowMessage("Find the energy!", Color.black);
			break;
		case PlayState.RETURNING:
			robot.SetHeadAngle(0);
			PlayNotificationAudio(dropEnergy);
			hintMessage.ShowMessageForDuration("Drop the energy at the transformer", 3.0f, Color.black);
			break;
		case PlayState.RETURNED:
			robot.SetHeadAngle(0);
			//ActionButton.DROP = "COLLECT";
			//RobotEngineManager.instance.SuccessOrFailure += CheckForGoldDropOff;

			if ( goldExtractingObject != null )
			{
				goldExtractingObject.SetActiveObjectLEDs(1, COLOR_RED, 0, 0xFF, 150, 150); 
			}
			hintMessage.ShowMessage("Deposit the energy!", Color.black);
			break;
		default:
			break;
		}
	}

	void ExitPlayState(PlayState new_state)
	{
		switch(playState)
		{
		case PlayState.IDLE:
			break;
		case PlayState.SEARCHING:
			if( goldExtractingObject != null ) goldExtractingObject.SetActiveObjectLEDs(0);
			break;
		case PlayState.EXTRACTING:
			hintMessage.KillMessage();
			break;
		case PlayState.READY_TO_RETURN:
			hintMessage.KillMessage();
			break;
		case PlayState.RETURNING:
			//ActionButton.DROP = null;
			break;
		case PlayState.RETURNED:
			//RobotEngineManager.instance.SuccessOrFailure -= CheckForGoldDropOff;
			hintMessage.KillMessage();
			break;
		default:
			break;
		}
	}

	void CheckForGoldDropOff(bool success, int action_type)
	{
		if( success )
		{
			if( action_type == 8 )
			{
				StartCoroutine(AwardPoints());
			}
		}
	}

	void UpdateCanExtract()
	{
		Vector2 buriedLocation;
		if (buriedLocations.TryGetValue (robot.carryingObject, out buriedLocation)) {
			float distance = (buriedLocation - (Vector2)robot.WorldPosition).magnitude;
			if( distance > findRadius )
			{
				EnterPlayState(PlayState.SEARCHING);
			}
		}
	}

	void UpdateSearching()
	{
		if(robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK) && robot.carryingObject != null) 
		{
			lastCarriedObjectId = robot.carryingObject;
			Vector2 buriedLocation;
			if(buriedLocations.TryGetValue(robot.carryingObject, out buriedLocation)) 
			{
				UpdateDirectionLights(buriedLocation);

				float distance = (buriedLocation - (Vector2)robot.WorldPosition).magnitude;
				if( !foundItems.Contains(robot.carryingObject) ) 
				{
					if(distance <= findRadius) 
					{
						//show 'found' light pattern
						if( audioLocatorEnabled )
						{
							audio.Stop();
							audio.loop = true;
							gameObject.audio.PlayOneShot(foundBeep);
							EnterPlayState(PlayState.CAN_EXTRACT);
						}
						foundItems.Add(robot.carryingObject);
						Debug.Log("found!");
					}
					else if(distance <= detectRadius) 
					{
						//float warmthFactor = Mathf.Clamp01((distance - findRadius) / (detectRadius - findRadius));
						//show 'warmer' light pattern to indicate proximity 
						float dist_percent = 1 - ((detectRadius-findRadius)-(distance-findRadius))/(detectRadius-findRadius);
						float current_rate = Mathf.Lerp(detectRangeDelayClose, detectRangeDelayFar, dist_percent);
						UpdateLocatorSound(current_rate);
						//UpdateDetectorLights(1-dist_percent);

						hintMessage.KillMessage();
					}
				}
				else if(distance <= findRadius)
				{
					EnterPlayState(PlayState.CAN_EXTRACT);
					UpdateDetectorLights (1);
				}
				else if( foundItems.Contains(robot.carryingObject) && distance > findRadius )
				{
					// remove it from our found list if we exit the find radius without dropping it
					foundItems.Remove(robot.carryingObject);
					hintMessage.KillMessage();
				}
			}
			
		}

		totalActiveTime += Time.deltaTime;
	}

	void UpdateIdle()
	{
		if (robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) 
		{
			EnterPlayState(PlayState.SEARCHING);
		}
	}

	void UpdateExtracting()
	{
		hintMessage.ShowMessage ("Extracting: " + ((int)(100 * (playStateTimer / extractionTime))).ToString () + "%", Color.black);
	}

	void UpdateReadyToReturn()
	{
		if (robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) 
		{
			EnterPlayState(PlayState.RETURNING);
		}
	}

	void UpdateReturning()
	{
		Vector2 home_base_pos = Vector2.zero;
		if (goldCollectingObject != null && robot.knownObjects.Find(x => x == goldCollectingObject) != null )
		{
			home_base_pos = robot.knownObjects.Find(x => x == goldCollectingObject).WorldPosition;
			Debug.Log("home_base_pos: "+home_base_pos.ToString());
		}
		float distance = (home_base_pos - (Vector2)robot.WorldPosition).magnitude;
		Debug.Log ("distance: " + distance);
		if (distance < returnRadius) 
		{
			EnterPlayState(PlayState.RETURNED);
		}


		totalActiveTime += Time.deltaTime;
	}

	void UpdateReturned()
	{
		Vector2 home_base_pos = Vector2.zero;
		if (goldCollectingObject != null && robot.knownObjects.Find(x => x == goldCollectingObject) != null )
		{
			home_base_pos = robot.knownObjects.Find(x => x == goldCollectingObject).WorldPosition;
			Debug.Log("home_base_pos: "+home_base_pos.ToString());
		}
		// todo: add code to make sure the player doen't leave the box before dropping it off
		float distance = (home_base_pos - (Vector2)robot.WorldPosition).magnitude;
		Debug.Log ("distance: " + distance);
		if (distance > returnRadius) 
		{
			EnterPlayState(PlayState.RETURNING);
		}
	}

	public void BeginExtracting()
	{
		// start extracting
		audio.loop = false;
		audio.Stop();
		foundItems.Remove(lastCarriedObjectId);
		//goldExtractingObjectId = lastCarriedObjectId;
		lastCarriedObjectId = -1;
		hintMessage.KillMessage();
		EnterPlayState(PlayState.EXTRACTING);
	}

	public void BeginDepositing()
	{
		EnterPlayState (PlayState.DEPOSITING);
		StartCoroutine(AwardPoints());
	}

#region IEnumerator

	public float gameStartingInDelay = 1.3f;
	IEnumerator CountdownToPlay()
	{
		PlayNotificationAudio (gameStartingIn);
		yield return new WaitForSeconds (gameStartingInDelay);
		int timer_index = timerSounds.Length - 3;
		while( timer_index < timerSounds.Length )
		{
			PlayNotificationAudio(timerSounds [timer_index]);
			timer_index++;
			yield return new WaitForSeconds (1);
		}
		PlayRequested();
	}

	IEnumerator StartExtracting()
	{
		uint color = COLOR_RED;
		if( goldExtractingObject != null ) goldExtractingObject.SetActiveObjectLEDs (1, color, 0, 0xCC);
		yield return new WaitForSeconds(extractionTime/2.0f);
		if( goldExtractingObject != null ) goldExtractingObject.SetActiveObjectLEDs (1, color, 0, 0xFF);
		yield return new WaitForSeconds(extractionTime/2.0f);
		EnterPlayState (PlayState.RETURNING);
	}

	IEnumerator AwardPoints()
	{
		// will end up doing active block light stuff here
		uint color = COLOR_RED;
		if( goldExtractingObject != null ) goldExtractingObject.SetActiveObjectLEDs (1, color, 0, 0xCC);
		yield return new WaitForSeconds(rewardTime);
		if( goldExtractingObject != null ) goldExtractingObject.SetActiveObjectLEDs (0);
		PlayNotificationAudio (collectedSound);
		// award points
		numDrops++;
		scores[0]+= 10 * numDrops;

		int num_drops_this_run = numDrops % numDropsForBonusTime;
		if (num_drops_this_run == 0) 
		{
			// award time, clear robot lights
			int num_bonuses_awarded = (numDrops / numDropsForBonusTime) - 1;
			float awardedTime = num_bonuses_awarded == 0 ? baseTimeBonus : baseTimeBonus*bonusTimeDecay;
			bonusTime += awardedTime;
			ResetTimerIndex(totalActiveTime);
			SetEnergyBars(numDropsForBonusTime, color);

		}
		else
		{
			// set the robot lights
			SetEnergyBars(num_drops_this_run, 0xff0000ff);
		}
		EnterPlayState(PlayState.IDLE);
		yield return new WaitForSeconds (2.089f);
		if (num_drops_this_run == 0) 
		{
			PlayNotificationAudio(timeExtension);
		}

	}
#endregion

	#region Active Block IFC
	void UpdateDetectorLights(float light_intensity)
	{
		float time_now = Time.realtimeSinceStartup;

		if (time_now - last_light_message_time > light_messaging_delay) 
		{
			Debug.Log("light_intensity: "+ light_intensity);
			last_light_message_time = time_now;
			float r = 255 * light_intensity;
			float g = 255 * light_intensity;
			
			uint color = ((uint)r << 24 | (uint)g << 16 ) | 0x00FF;
			if( goldExtractingObject != null ) goldExtractingObject.SetActiveObjectLEDs(color, 0, 0x33);
		}
	}

	void UpdateDirectionLights(Vector2 target_position)
	{
		float time_now = Time.realtimeSinceStartup;
		
		if (time_now - last_light_message_time > light_messaging_delay) 
		{
			Vector3 heading3 = robot.Forward;
			Vector2 heading = new Vector2(heading3.x, heading3.y);
			heading = heading.normalized;
			Vector2 to_target = target_position - new Vector2 (robot.WorldPosition.x, robot.WorldPosition.y);
			to_target = to_target.normalized;
			float dot_product = Vector2.Dot(heading, to_target);
			float angle_between = MathUtil.DotProductAngle(heading, to_target);
			
			Debug.Log ("dot_product: " + dot_product.ToString () + ", " + "angle_between: " + angle_between.ToString ());
			
			
			last_light_message_time = time_now;
			byte which_leds = 1; // front face, right side
			byte relative_mode = 1;
			// need to check angle variance in each of four directions
			// our allowed delta is ~5 degrees

			float allowed_delta = .18f;
			if( (angle_between <= allowed_delta && angle_between >= -allowed_delta) // facing
			   || (angle_between <= (Math.PI) + allowed_delta && angle_between >= (Math.PI) - allowed_delta) // behind
			   || (angle_between <= (Math.PI/2) + allowed_delta && angle_between >= (Math.PI/2) - allowed_delta) // behind
			   || (angle_between <= (Math.PI/3) + allowed_delta && angle_between >= (Math.PI/3) - allowed_delta)) // behind
			{
				which_leds = 0x11; // front face, both sides
				relative_mode = 2;
			}
			/*
			else if( (angle_between <= Math.PI/4 && Vector3.Dot(heading3, robot.Right) < 0)) // robot's forward
				        //|| (angle_between <= Math.PI/4 && Vector3.Dot(heading3, robot.Right) < 0)
			{
					which_leds = 2;
			}
			*/

			
			uint color =  COLOR_RED;
			if( /*last_leds != which_leds &&*/ goldExtractingObject != null ) goldExtractingObject.SetActiveObjectLEDsRelative(target_position, color, 0, which_leds, relative_mode);
			last_leds = which_leds;
		}
		
	}

	void SetEnergyBars(int num_bars, uint color = 0)
	{

		Anki.Cozmo.U2G.SetBackpackLEDs msg = new Anki.Cozmo.U2G.SetBackpackLEDs ();
		msg.robotID = robot.ID;
		for(int i=0; i<5; ++i)
		{
			if( i < num_bars )
			{
				msg.onColor[i] = color;
			}
			else
			{
				msg.onColor[i] = 0;
			}
			msg.offColor[i] = 0;
			msg.onPeriod_ms[i] = 1000;
			msg.offPeriod_ms[i] = 0;
			msg.transitionOnPeriod_ms[i] = 0;
			msg.transitionOffPeriod_ms[i] = 0;
		}
		
		Anki.Cozmo.U2G.Message msgWrapper = new Anki.Cozmo.U2G.Message{SetBackpackLEDs = msg};
		RobotEngineManager.instance.channel.Send (msgWrapper);
	}
	#endregion
}
