using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class BuildInstructionsController : MonoBehaviour {

	[SerializeField] BuildInstructions[] layouts;
	[SerializeField] Text title;
	[SerializeField] Text instructions;
	[SerializeField] Text note;
	[SerializeField] Button buttonPrevLayout;
	[SerializeField] Button buttonNextLayout;
	[SerializeField] Button buttonPrevStep;
	[SerializeField] Button buttonNextStep;

	int currentLayout = 0;
	int currentPage = 0;
	void OnEnable () {
		currentLayout = 0;
		currentPage = 0;

		for(int i=0; i<layouts.Length; i++) {
			layouts[i].Initialize();
		}

		RefreshPage();
	}

	void RefreshPage () {

		for(int i=0; i<layouts.Length; i++) {
			layouts[i].gameObject.SetActive(currentLayout == i);
		}

		BuildInstructions layout = layouts[currentLayout];

		title.text = layout.title;

		for(int i=0; i<layout.steps.Length; i++) {
			if(currentPage == 0) {
				layout.steps[i].cube.Hidden = false;
				layout.steps[i].cube.Highlighted = false;
				layout.steps[i].cube.Validated = true;
			}
			else {
				layout.steps[i].cube.Hidden = i >= (currentPage-1);
				layout.steps[i].cube.Highlighted = i == (currentPage-1);

				//dmd2do make cozmo confirm this cubes existance and position
				layout.steps[i].cube.Validated = false;
			}
		}



		if(instructions != null) {
			if(currentPage <= 0) {
				instructions.text = "Press >> to start building this game's layout!";
			}
			else if(currentPage >= layouts[currentLayout].steps.Length+1) {
				instructions.text = "Press >> to validate the build and start playing!";
			}
			else {
				BuildInstructionStep step = layout.steps[currentPage-1];
				instructions.text = "Step " + currentPage + ": Place a " + step.cube.GetPropTypeName() + " " + step.cube.GetPropFamilyName() + "." ;

				if(!string.IsNullOrEmpty(step.explicitInstruction)) {
					note.text = "note: " + step.explicitInstruction;
				}
				else {
					note.text = "";
				}
			}

		}

		if(buttonPrevLayout != null) buttonPrevLayout.interactable = currentLayout > 0;
		if(buttonNextLayout != null) buttonNextLayout.interactable = currentLayout < layouts.Length-1;
		if(buttonPrevStep != null) buttonPrevStep.interactable = currentPage > 0;
		if(buttonNextStep != null) buttonNextStep.interactable = true;//currentPage <= layouts[currentLayout].steps.Length+1;

	}

	public void SetLayout(int index) {
		currentLayout = Mathf.Clamp(index, 0, layouts.Length-1);
		currentPage = 0;
		RefreshPage();
	}

	public void NextLayout() {
		currentLayout++;
		if(currentLayout >= layouts.Length) currentLayout = 0;
		currentPage = 0;
		RefreshPage();
	}
	
	public void PreviousLayout() {
		currentLayout--;
		if(currentLayout < 0) currentLayout = layouts.Length - 1;
		currentPage = 0;
		RefreshPage();
	}

	public void NextStep() {
		currentPage = Mathf.Clamp(currentPage + 1, 0, layouts[currentLayout].steps.Length+1);
		RefreshPage();
	}

	public void PreviousStep() {
		currentPage = Mathf.Clamp(currentPage - 1, 0, layouts[currentLayout].steps.Length+1);
		RefreshPage();
	}

	public void ValidateBuild() {

		
	}
}
