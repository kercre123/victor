using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class CozmoBuildInstructions : MonoBehaviour {
	[SerializeField] BuildInstructionsCube[] cubes;
	[SerializeField] Text instructions;

	int currentPage = 0;
	void OnEnable () {
		currentPage = 0;

		//init cubes in order, so their canvas draw order is apt
		for(int i=0; i<cubes.Length; i++) {
			cubes[i].Initialize();
		}

		RefreshPage();
	}

	void RefreshPage () {
		for(int i=0; i<cubes.Length; i++) {
			if(currentPage == 0) {
				cubes[i].Hidden = false;
				cubes[i].Highlighted = false;
				cubes[i].Validated = true;
			}
			else {
				cubes[i].Hidden = i >= (currentPage-1);
				cubes[i].Highlighted = i == (currentPage-1);

				//dmd2do make cozmo confirm this cubes existance and position
				cubes[i].Validated = false;
			}
		}
	}

	public void NextInstruction() {
		currentPage = Mathf.Clamp(currentPage + 1, 0, cubes.Length+1);
		RefreshPage();
	}

	public void PreviouesInstruction() {
		currentPage = Mathf.Clamp(currentPage - 1, 0, cubes.Length+1);
		RefreshPage();
	}

	public void ValidateBuild() {

		
	}
}
