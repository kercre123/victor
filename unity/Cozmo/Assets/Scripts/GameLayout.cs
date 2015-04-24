using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class GameLayout : MonoBehaviour {

	[SerializeField] public Camera previewCamera;
	[SerializeField] public string gameName = "Unknown";
	[SerializeField] public int levelNumber = 1;
	[SerializeField] public List<BuildInstructionsCube> blocks = new List<BuildInstructionsCube>();

	public void Initialize () {

		//init cubes in order, so their canvas draw order is apt
		for(int i=0; i<blocks.Count; i++) {
			blocks[i].Initialize();
		}

	}

}
