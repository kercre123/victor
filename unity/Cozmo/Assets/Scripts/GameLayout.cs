using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class GameLayout : MonoBehaviour {

	[SerializeField] public string title;
	[SerializeField] public List<BuildInstructionsCube> blocks = new List<BuildInstructionsCube>();
	[SerializeField] public string gameTypeFilter = null;

	public void Initialize () {

		//init cubes in order, so their canvas draw order is apt
		for(int i=0; i<blocks.Count; i++) {
			blocks[i].Initialize();
		}

	}

}
