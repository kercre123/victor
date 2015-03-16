using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class GoldRushController : GameController {

	[SerializeField] float hideRadius;	//radius around cube's initial position in which its gold will be buried
	[SerializeField] float findRadius;	//dropping cube within find radius will trigger transmutation/score
	[SerializeField] float detectRadius; //pulsing will accelerate from detect to find ranges

	List<ObservedObject> goldCubes = new List<ObservedObject>();
	Dictionary<int, Vector2> buriedLocations = new Dictionary<int, Vector2>();

	Robot robot;
	//int lastHeldID;

	protected override void Enter_PLAYING() {
		base.Enter_PLAYING();

		goldCubes.Clear();
		robot = RobotEngineManager.instance.current;

		foreach(ObservedObject obj in robot.knownObjects) {
			//if(obj.objectType == ObservedObjectType.Gold) {
				goldCubes.Add(obj);
			//}
		}

		buriedLocations.Clear();
		for(int i=0; i<goldCubes.Count; i++) {
			buriedLocations[goldCubes[i].ID] = (Vector2)goldCubes[i].WorldPosition + Random.insideUnitCircle * hideRadius;
			//set goldCubes[i] to lead mode, whatever that entails

		}
	}

	protected override void Update_PLAYING() {
		base.Update_PLAYING();

		if(robot.carryingObjectID != 0) {
			Vector2 buriedLocation;
			if(buriedLocations.TryGetValue(robot.carryingObjectID, out buriedLocation)) {
				float distance = (buriedLocation - (Vector2)robot.WorldPosition).magnitude;
				if(distance <= findRadius) {
					//show 'found' light pattern
				}
				else if(distance <= detectRadius) {
					//float warmthFactor = Mathf.Clamp01((distance - findRadius) / (detectRadius - findRadius));
					//show 'warmer' light pattern to indicate proximity
				}
			}
		}
		//else if lead cube dropped within findRadius, Transmute(lastHeldID)

		//lastHeldID = robot.carryingObjectID;
	}

	protected override bool IsGameReady() {
		if(!base.IsGameReady()) return false;
		
		//game specific start conditions...
		//are sufficient gold cubes in play?

		return true;
	}

	protected override bool IsGameOver() {
		if(base.IsGameOver()) return true;

		//game specific end conditions...

		for(int i=0; i<goldCubes.Count; i++) {
			//if goldCubes[i] is still lead, return false
		}
		return false;
	}

	void Transmute(int id) {
		//if bad id or not lead cube, return
		//set cube back to gold
		scores[0]++;
	}
}
