using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class GameLayout : MonoBehaviour {

	[SerializeField] public string gameName = "Unknown";
	[SerializeField] public int levelNumber = 1;
	[SerializeField] public List<BuildInstructionsCube> blocks = new List<BuildInstructionsCube>();
	[SerializeField] public Transform startPositionMarker;

	[TextArea(5,30)]
	[SerializeField] public string initialInstruction = null;

	[TextArea(5,30)]
	[SerializeField] public string secondInstruction = null;

	public void Initialize () {

		//init cubes in order, so their canvas draw order is apt
		for(int i=0; i<blocks.Count; i++) {
			blocks[i].Initialize();
		}

	}

}
