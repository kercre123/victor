using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;

public class GoldRushController : GameController {

	[SerializeField] protected AudioClip foundBeep;
	[SerializeField] protected AudioClip collectedSound;
	[SerializeField] protected AudioSource timerAudio;
	[SerializeField] protected AudioClip[] timerSounds;
	[SerializeField] protected int[] timerEventTimes = {60,30,10,9,8,7,6,5,4,3,2,1};
	[SerializeField] protected Button extractButton = null;
	public float detectRangeDelayFar = 2.0f;
	public float detectRangeDelayClose = .2f;
	private float lastPlayTime = 0;
	private int timerEventIndex = 0;

	[SerializeField] float hideRadius;	//radius around cube's initial position in which its gold will be buried
	[SerializeField] float findRadius;	//dropping cube within find radius will trigger transmutation/score
	[SerializeField] float detectRadius; //pulsing will accelerate from detect to find ranges
	[SerializeField] float extractionTime = 1.5f; //pulsing will accelerate from detect to find ranges

	[SerializeField] Text resultsScore; //for the results screen

	//List<ObservedObject> goldCubes = new List<ObservedObject>();
	Dictionary<int, Vector2> buriedLocations = new Dictionary<int, Vector2>();
	List<int> foundItems = new List<int>();
	bool attemptingStack = false;
	int lastCarriedObjectId = -1;
	int goldCarryingObjectId = -1;
	int baseObjectId = -1;
	ScreenMessage hintMessage;

	enum PlayState
	{
		IDLE,
		SEARCHING,
		EXTRACTING,
		READY_TO_RETURN,
		RETURNING,
		RETURNED,
		NUMSTATES
	};

	private PlayState playState = PlayState.IDLE;
	float playStateTimer = 0;
	float totalActiveTime = 0; // only increments when the robot is searching for or returing gold

	Robot robot;
	//int lastHeldID;

	void Awake()
	{
		hintMessage = GetComponentInChildren<ScreenMessage> ();
		extractButton.gameObject.SetActive (false);
	}

	protected override void RefreshHUD ()
	{
		base.RefreshHUD ();

		// need timer to reflect our games unique use of it
		if (textTime != null && state == GameState.PLAYING) 
		{
			textTime.text = Mathf.FloorToInt (maxPlayTime - totalActiveTime).ToString ();
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
		timerEventIndex = 0;
		scores [0] = 0;



	}

	protected override void Exit_PLAYING()
	{
		base.Exit_PLAYING();
		resultsScore.text = "Score: " + scores [0];
		CozmoVision.EnableDing(); // just in case we were in searching mode
		extractButton.gameObject.SetActive (false);
		ActionButton.DROP = null;
		audio.Stop ();
	}

	protected override void Update_PLAYING() 
	{
		base.Update_PLAYING();

		playStateTimer += Time.deltaTime;

		UpdateTimerEvents (totalActiveTime);

		extractButton.gameObject.SetActive (false);

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
		//else if lead cube dropped within findRadius, Transmute(lastHeldID)
		//lastHeldID = robot.carryingObjectID;
	}

	protected override void Enter_BUILDING ()
	{
		base.Enter_BUILDING ();
		lastCarriedObjectId = -1;
		goldCarryingObjectId = -1;
		playButton.gameObject.SetActive (false);
		attemptingStack = false;

		robot = RobotEngineManager.instance.current;
	}

	protected override void Exit_BUILDING ()
	{
		base.Exit_BUILDING ();
	}

	protected override void Update_BUILDING ()
	{
		base.Update_BUILDING ();
		// looking to see if we've created our stack
		if (robot != null 
		    && !attemptingStack
		    && robot.Status (Robot.StatusFlag.IS_PICKING_OR_PLACING) 
		    && robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK) ) 
		{
			Debug.Log("Attempting stack...");
			attemptingStack = true;
		}
		else if ( robot != null 
		         && attemptingStack
		         && (!robot.Status (Robot.StatusFlag.IS_PICKING_OR_PLACING) 
		         || !robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) ) 
		{
			Debug.Log("Not attempting stack...");
			attemptingStack = false;
		}
	}

	protected override bool IsGameReady() 
	{
		//if(!base.IsGameReady()) return false;
		if(RobotEngineManager.instance == null) return false;
		if(RobotEngineManager.instance.current == null) return false;
		
		//game specific start conditions...
		//are sufficient gold cubes in play?

		return true;
	}

	protected override bool IsGameOver() 
	{
		//if(base.IsGameOver()) return true;
		if(maxPlayTime > 0f && totalActiveTime >= maxPlayTime) return true;

		//game specific end conditions...
		return false;
	}

	void Transmute(int id) 
	{
		//if bad id or not lead cube, return
		//set cube back to gold
		scores[0]++;
	}

	void UpdateTimerEvents (float timer)
	{
		if (timerEventIndex < timerEventTimes.Length) 
		{

			// get our next event
			int next_event_time = timerEventTimes[timerEventIndex];
			if( (int)(maxPlayTime-timer) <= next_event_time )
			{
				timerAudio.PlayOneShot(timerSounds[timerEventIndex]);
				timerEventIndex++;
			}
		}
	}

	void UpdateLocatorSound(float current_rate) 
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

	void EnterPlayState(PlayState new_state)
	{
		ExitPlayState (playState);
		playStateTimer = 0;
		playState = new_state;
		switch (playState) {
		case PlayState.IDLE:
			break;
		case PlayState.SEARCHING:
			foundItems.Clear();
			buriedLocations.Clear();
			buriedLocations[robot.carryingObjectID] = (Vector2)robot.WorldPosition + Random.insideUnitCircle * hideRadius;
			CozmoVision.EnableDing(false);
			break;
		case PlayState.EXTRACTING:
			StartCoroutine(StartExtracting());
			break;
		case PlayState.READY_TO_RETURN:
			hintMessage.ShowMessage("Pick up the gold and bring it back!", Color.black);
			break;
		case PlayState.RETURNING:
			hintMessage.ShowMessageForDuration("Get that gold back to the base!", 3.0f, Color.black);
			break;
		case PlayState.RETURNED:
			ActionButton.DROP = "COLLECT";
			hintMessage.ShowMessage("Drop off the gold to collect points!", Color.black);
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
			CozmoVision.EnableDing();
			break;
		case PlayState.EXTRACTING:
			hintMessage.KillMessage();
			break;
		case PlayState.READY_TO_RETURN:
			hintMessage.KillMessage();
			break;
		case PlayState.RETURNING:
			ActionButton.DROP = null;
			break;
		case PlayState.RETURNED:
			hintMessage.KillMessage();
			break;
		default:
			break;
		}
	}

	void UpdateSearching()
	{
		if(robot.carryingObjectID != -1) 
		{
			lastCarriedObjectId = robot.carryingObjectID;
			Vector2 buriedLocation;
			if(buriedLocations.TryGetValue(robot.carryingObjectID, out buriedLocation)) 
			{
				float distance = (buriedLocation - (Vector2)robot.WorldPosition).magnitude;
				if( !foundItems.Contains(robot.carryingObjectID) ) 
				{
					if(distance <= findRadius) 
					{
						//show 'found' light pattern
						audio.Stop();
						audio.loop = true;
						gameObject.audio.PlayOneShot(foundBeep);
						foundItems.Add(robot.carryingObjectID);
						extractButton.gameObject.SetActive (true);
						Debug.Log("found!");
					}
					else if(distance <= detectRadius) 
					{
						//float warmthFactor = Mathf.Clamp01((distance - findRadius) / (detectRadius - findRadius));
						//show 'warmer' light pattern to indicate proximity 
						float dist_percent = 1 - ((detectRadius-findRadius)-(distance-findRadius))/(detectRadius-findRadius);
						float current_rate = Mathf.Lerp(detectRangeDelayClose, detectRangeDelayFar, dist_percent);
						UpdateLocatorSound(current_rate);
						hintMessage.KillMessage();
					}
				}
				else if(distance <= findRadius)
				{
					extractButton.gameObject.SetActive (true);
				}
				else if( foundItems.Contains(robot.carryingObjectID) && distance > findRadius )
				{
					// remove it from our found list if we exit the find radius without dropping it
					foundItems.Remove(robot.carryingObjectID);
					hintMessage.KillMessage();
				}
			}
			
		}

		totalActiveTime += Time.deltaTime;
	}

	void UpdateIdle()
	{
		if (robot.carryingObjectID != -1) 
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
		if (robot.carryingObjectID != -1) 
		{
			EnterPlayState(PlayState.RETURNING);
		}
	}

	void UpdateReturning()
	{
		float distance = (Vector2.zero - (Vector2)robot.WorldPosition).magnitude;
		if (distance < findRadius) 
		{
			EnterPlayState(PlayState.RETURNED);
		}

		totalActiveTime += Time.deltaTime;
	}

	void UpdateReturned()
	{
		if (robot.carryingObjectID == -1) 
		{
			// award points
			scores[0]+= 10;
			audio.Stop();
			audio.PlayOneShot(collectedSound);
			EnterPlayState(PlayState.IDLE);
		}

		// todo: add code to make sure the player doen't leave the box before dropping it off
	}

	public void BeginExtracting()
	{
		// start extracting
		audio.loop = false;
		audio.Stop();
		foundItems.Remove(lastCarriedObjectId);
		goldCarryingObjectId = lastCarriedObjectId;
		lastCarriedObjectId = -1;
		hintMessage.KillMessage();
		EnterPlayState(PlayState.EXTRACTING);
	}

	IEnumerator StartExtracting()
	{
		yield return new WaitForSeconds(extractionTime);
		EnterPlayState (PlayState.RETURNING);
	}
}
