using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class GameLayout : MonoBehaviour {

	[SerializeField] public string gameName = "Unknown";
	[SerializeField] public int levelNumber = 1;
	[SerializeField] public List<BuildInstructionsCube> blocks = new List<BuildInstructionsCube>();
	[SerializeField] public Transform startPositionMarker;
	[SerializeField] public float scale = 0.044f;

	[TextArea(5,30)]
	[SerializeField] public string initialInstruction = null;

	[TextArea(5,30)]
	[SerializeField] public string secondInstruction = null;

	public void Initialize() {

		//init cubes in order, so their canvas draw order is apt
		for(int i=0; i<blocks.Count; i++) {
			blocks[i].Initialize ();
			scale = blocks[i].transform.lossyScale.x;
		}

		for(int i=0; i<blocks.Count-1; i++) {
			for(int j=i+1; j<blocks.Count; j++) {
				Vector3 offset = blocks[j].transform.position - blocks[i].transform.position;
				Vector2 flat = offset;
				flat.y = 0f;

				if( flat.magnitude > 0.1f) continue;

				if(offset.y > 0.1f) {
					//Debug.Log ("GameLayout " + blocks[j].gameObject.name + " stacked on " + blocks[i].gameObject.name);
					blocks[j].cubeBelow = blocks[i];
					blocks[i].cubeAbove = blocks[j];
				}
				else if(offset.y < -0.1f) {
					//Debug.Log ("GameLayout " + blocks[i].gameObject.name + " stacked on " + blocks[j].gameObject.name);
					blocks[j].cubeAbove = blocks[i];
					blocks[i].cubeBelow = blocks[j];
				}
			}
		}

	}

}
