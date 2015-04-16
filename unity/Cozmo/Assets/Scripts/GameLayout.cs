using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using Anki.Cozmo;
using G2U = Anki.Cozmo.G2U;
using U2G = Anki.Cozmo.U2G;

public class GameLayout : MonoBehaviour {

	[SerializeField] public string title;
	[SerializeField] public BuildInstructionsCube[] blocks;
	[SerializeField] public string gameTypeFilter = null;

	public void Initialize () {

		//init cubes in order, so their canvas draw order is apt
		for(int i=0; i<blocks.Length; i++) {
			blocks[i].Initialize();
		}

	}

}
