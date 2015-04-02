using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using Anki.Cozmo;

[System.Serializable]
public class BuildInstructionStep {
	public BuildInstructionsCube cube = null;
	public string explicitInstruction = null;

	public void Initialize() {
		if(cube != null) {
			cube.Initialize();
		}
	}
}

public class BuildInstructions : MonoBehaviour {

	[SerializeField] public string title;
	[SerializeField] public BuildInstructionStep[] steps;
	[SerializeField] public string gameTypeFilter = null;

	public void Initialize () {

		//init cubes in order, so their canvas draw order is apt
		for(int i=0; i<steps.Length; i++) {
			steps[i].Initialize();
		}

	}

}
