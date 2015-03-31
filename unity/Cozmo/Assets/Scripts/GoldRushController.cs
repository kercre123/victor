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

	List<ObservedObject> goldCubes = new List<ObservedObject>();
	Dictionary<int, Vector2> buriedLocations = new Dictionary<int, Vector2>();
	List<int> foundItems = new List<int>();
	int lastCarriedObjectId = -1;
	ScreenMessage dropMessage;

	Robot robot;
	//int lastHeldID;

	void Awake()
	{
		dropMessage = GetComponentInChildren<ScreenMessage> ();
	}

	protected override void Enter_PLAYING() 
	{
		base.Enter_PLAYING();

		goldCubes.Clear();
		foundItems.Clear();
		robot = RobotEngineManager.instance.current;

		foreach(ObservedObject obj in robot.knownObjects) 
		{
			//if(obj.objectType == ObservedObjectType.Gold) {
				goldCubes.Add(obj);
			//}
		}

		buriedLocations.Clear();
		for(int i=0; i<goldCubes.Count; i++) 
		{
			buriedLocations[goldCubes[i].ID] = (Vector2)goldCubes[i].WorldPosition + Random.insideUnitCircle * hideRadius;
			Debug.Log("added loot at: " + buriedLocations[goldCubes[i].ID].ToString());
		}
	}

	protected override void Update_PLAYING() 
	{
		base.Update_PLAYING();

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
						dropMessage.ShowMessage("Drop the extractor to mine the gold!", Color.black);
						Debug.Log("found!");
					}
					else if(distance <= detectRadius) 
					{
						//float warmthFactor = Mathf.Clamp01((distance - findRadius) / (detectRadius - findRadius));
						//show 'warmer' light pattern to indicate proximity 
						float dist_percent = 1 - ((detectRadius-findRadius)-(distance-findRadius))/(detectRadius-findRadius);
						float current_rate = Mathf.Lerp(detectRangeDelayClose, detectRangeDelayFar, dist_percent);
						UpdateLocatorSound(current_rate);
						dropMessage.KillMessage();
					}
				}
				else if( foundItems.Contains(robot.carryingObjectID) && distance > findRadius )
				{
					// remove it from our found list if we exit the find radius without dropping it
					foundItems.Remove(robot.carryingObjectID);
					dropMessage.KillMessage();
				}
			}
			else if( lastCarriedObjectId != -1 && foundItems.Contains(lastCarriedObjectId) )
			{
				// item has been dropped, so score it
				foundItems.Remove(lastCarriedObjectId);
				lastCarriedObjectId = -1;
				dropMessage.KillMessage();

			}
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

		for(int i=0; i<goldCubes.Count; i++) 
		{
			//if goldCubes[i] is still lead, return false
		}
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
}
