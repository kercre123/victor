using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class GameLayout : MonoBehaviour {

	[SerializeField] public string gameName = "Unknown";
	[SerializeField] public int levelNumber = 1;
	[SerializeField] public List<BuildInstructionsCube> blocks = new List<BuildInstructionsCube>();

	[SerializeField] public Transform startPositionMarker;
	[SerializeField] public float scale = 0.044f;
	[SerializeField] public List<int> gateBlocksAtPlayerCount = new List<int>();

	[TextArea(5,30)]
	[SerializeField] public string initialInstruction = null;

	[TextArea(5,30)]
	[SerializeField] public string secondInstruction = null;

	public List<BuildInstructionsCube> Blocks = new List<BuildInstructionsCube>();

	public void Initialize(int players=1) {

		Blocks.Clear();

		for(int i=0;i<blocks.Count;i++) {
			if(i < gateBlocksAtPlayerCount.Count && players < gateBlocksAtPlayerCount[i]) {
				blocks[i].gameObject.SetActive(false);
				continue;
			}
			blocks[i].gameObject.SetActive(true);
			Blocks.Add(blocks[i]);
		}

		//init cubes in order, so their canvas draw order is apt
		for(int i=0; i<Blocks.Count; i++) {
			Blocks[i].Initialize ();
			scale = Blocks[i].transform.lossyScale.x;
		}

		for(int i=0; i<Blocks.Count-1; i++) {
			for(int j=i+1; j<Blocks.Count; j++) {
				Vector3 offset = Blocks[j].transform.position - Blocks[i].transform.position;
				Vector2 flat = offset;
				flat.y = 0f;

				if( flat.magnitude > (scale * 0.1f)) continue;

				if(offset.y > 0.1f) {
					//Debug.Log ("GameLayout " + blocks[j].gameObject.name + " stacked on " + blocks[i].gameObject.name);
					Blocks[j].cubeBelow = Blocks[i];
					Blocks[i].cubeAbove = Blocks[j];
				}
				else if(offset.y < -0.1f) {
					//Debug.Log ("GameLayout " + blocks[i].gameObject.name + " stacked on " + blocks[j].gameObject.name);
					Blocks[j].cubeAbove = Blocks[i];
					Blocks[i].cubeBelow = Blocks[j];
				}
			}
		}

	}

}
