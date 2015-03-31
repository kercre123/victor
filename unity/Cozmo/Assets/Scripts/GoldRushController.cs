using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class GoldRushController : GameController {

	public float detectRangeDelayFar = 2.0f;
	public float detectRangeDelayClose = .2f;
	private float lastPlayTime = 0;

	[SerializeField] float hideRadius;	//radius around cube's initial position in which its gold will be buried
	[SerializeField] float findRadius;	//dropping cube within find radius will trigger transmutation/score
	[SerializeField] float detectRadius; //pulsing will accelerate from detect to find ranges
	[SerializeField] float extractionTime = 1.5f; //pulsing will accelerate from detect to find ranges

	//List<ObservedObject> goldCubes = new List<ObservedObject>();
	Dictionary<int, Vector2> buriedLocations = new Dictionary<int, Vector2>();
	List<int> foundItems = new List<int>();
	int lastCarriedObjectId = -1;
	int goldCarryingObjectId = -1;
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

	private PlayState play_state = PlayState.IDLE;
	float play_state_timer = 0;

	Robot robot;
	//int lastHeldID;

	void Awake()
	{
		hintMessage = GetComponentInChildren<ScreenMessage> ();
	}

	protected override void Enter_PLAYING() 
	{
		base.Enter_PLAYING();
		play_state = PlayState.IDLE;
		play_state_timer = 0;

		scores [0] = 0;


		robot = RobotEngineManager.instance.current;

	}

	protected override void Update_PLAYING() 
	{
		base.Update_PLAYING();

		play_state_timer += Time.deltaTime;

		switch(play_state)
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

	protected override bool IsGameReady() 
	{
		if(!base.IsGameReady()) return false;
		
		//game specific start conditions...
		//are sufficient gold cubes in play?

		return true;
	}

	protected override bool IsGameOver() 
	{
		if(base.IsGameOver()) return true;

		//game specific end conditions...
		return false;
	}

	void Transmute(int id) 
	{
		//if bad id or not lead cube, return
		//set cube back to gold
		scores[0]++;
	}

	void UpdateLocatorSound(float current_rate) 
	{
		float timeSinceLast = Time.realtimeSinceStartup - lastPlayTime;
		if (timeSinceLast >= current_rate) 
		{
			gameObject.audio.Play();

			lastPlayTime = Time.realtimeSinceStartup;
		}
	}

	void EnterPlayState(PlayState new_state)
	{
		ExitPlayState (play_state);
		play_state_timer = 0;
		play_state = new_state;
		switch (play_state) {
		case PlayState.IDLE:
			break;
		case PlayState.SEARCHING:
			foundItems.Clear();
			buriedLocations.Clear();
			buriedLocations[robot.carryingObjectID] = (Vector2)robot.WorldPosition + Random.insideUnitCircle * hideRadius;
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
			hintMessage.ShowMessage("Drop off the gold to collect points!", Color.black);
			break;
		default:
			break;
		}
	}

	void ExitPlayState(PlayState new_state)
	{
		switch(play_state)
		{
		case PlayState.IDLE:
			break;
		case PlayState.SEARCHING:
			break;
		case PlayState.EXTRACTING:
			hintMessage.KillMessage();
			break;
		case PlayState.READY_TO_RETURN:
			hintMessage.KillMessage();
			break;
		case PlayState.RETURNING:
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
						gameObject.audio.PlayOneShot(playerScoreSound);
						foundItems.Add(robot.carryingObjectID);
						hintMessage.ShowMessage("Drop the extractor to mine the gold!", Color.black);
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
				else if( foundItems.Contains(robot.carryingObjectID) && distance > findRadius )
				{
					// remove it from our found list if we exit the find radius without dropping it
					foundItems.Remove(robot.carryingObjectID);
					hintMessage.KillMessage();
				}
			}
			
		}
		else if( lastCarriedObjectId != -1 && foundItems.Contains(lastCarriedObjectId) )
		{
			// item has been dropped, so start extracting

			foundItems.Remove(lastCarriedObjectId);
			goldCarryingObjectId = lastCarriedObjectId;
			lastCarriedObjectId = -1;
			hintMessage.KillMessage();
			EnterPlayState(PlayState.EXTRACTING);
		}
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
		hintMessage.ShowMessage ("Extracting: " + ((int)(100 * (play_state_timer / extractionTime))).ToString () + "%", Color.black);
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
	}

	void UpdateReturned()
	{
		if (robot.carryingObjectID == -1) 
		{
			// award points
			scores[0]+= 10;
			EnterPlayState(PlayState.IDLE);
		}

		// todo: add code to make sure the player doen't leave the box before dropping it off
	}

	IEnumerator StartExtracting()
	{
		yield return new WaitForSeconds(extractionTime);
		EnterPlayState (PlayState.RETURNING);
	}
}
