using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Anki.Cozmo;

public class BuildInstructionsController : MonoBehaviour {

	[SerializeField] List<BuildInstructions> layouts;
	[SerializeField] Text title;
	[SerializeField] Text instructions;
	[SerializeField] Text note;
	[SerializeField] Button buttonPrevLayout;
	[SerializeField] Button buttonNextLayout;
	[SerializeField] Button buttonPrevStep;
	[SerializeField] Button buttonNextStep;
	[SerializeField] Button buttonStartBuild;
	[SerializeField] Button buttonValidateBuild;

	public bool Validated { get; private set; }

	List<BuildInstructions> filteredLayouts;
	string gameTypeFilter = null;

	int currentLayout = 0;
	int currentPage = 0;

	void Awake() {
		for(int i=0; i<layouts.Count; i++) {
			layouts[i].Initialize();
		}
	}

	void OnEnable () {
		currentLayout = 0;
		currentPage = 0;

		Validated = false;

		for(int i=0; i<layouts.Count; i++) {
			layouts[i].gameObject.SetActive(false);
		}

		filteredLayouts = layouts.FindAll(x => string.IsNullOrEmpty(gameTypeFilter) || x.gameTypeFilter == gameTypeFilter);
		currentLayout = Mathf.Clamp(currentLayout, 0, filteredLayouts.Count-1);

		//Debug.Log("OnEnable filterType("+(gameTypeFilter != null ? gameTypeFilter : "null")+" filteredLayouts("+filteredLayouts.Count+")");

		note.text = "";
		RefreshPage();
	}

	void RefreshPage () {

		for(int i=0; i<filteredLayouts.Count; i++) {
			filteredLayouts[i].gameObject.SetActive(currentLayout == i);
		}

		BuildInstructions layout = filteredLayouts[currentLayout];

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
				instructions.text = "Press to start building this game's layout!";
			}
			else if(currentPage >= layout.steps.Length+1) {
				instructions.text = "Press to validate the build and start playing!";
			}
			else {
				BuildInstructionStep step = layout.steps[currentPage-1];
				instructions.text = "Step " + currentPage + ": Place a " + step.cube.GetPropTypeName() + " " + step.cube.GetPropFamilyName() + "." ;

				if(!string.IsNullOrEmpty(step.explicitInstruction)) {
					note.text = step.explicitInstruction;
				}
				else {
					note.text = "";
				}
			}

		}

		if(buttonPrevLayout != null) buttonPrevLayout.interactable = currentLayout > 0;
		if(buttonNextLayout != null) buttonNextLayout.interactable = currentLayout < filteredLayouts.Count-1;
		if(buttonPrevStep != null) buttonPrevStep.interactable = currentPage > 0;
		if(buttonNextStep != null) buttonNextStep.interactable =  currentPage <= layout.steps.Length;

		if(buttonStartBuild != null) buttonStartBuild.gameObject.SetActive(currentPage == 0);
		if(buttonValidateBuild != null) buttonValidateBuild.gameObject.SetActive(currentPage == layout.steps.Length+1);

	}

	public void SetLayoutForGame(string filterType) {
		gameTypeFilter = filterType;
		
		filteredLayouts = layouts.FindAll(x => string.IsNullOrEmpty(filterType) || x.gameTypeFilter == filterType);

		//Debug.Log("SetLayoutForGame filterType("+filterType.ToString()+" filteredLayouts("+filteredLayouts.Count+")");

		currentLayout = 0;
		currentPage = 0;
		Validated = false;

		RefreshPage();
	}

	public void NextLayout() {
		currentLayout++;
		if(currentLayout >= filteredLayouts.Count) currentLayout = 0;
		currentPage = 0;
		Validated = false;
		RefreshPage();
	}
	
	public void PreviousLayout() {
		currentLayout--;
		if(currentLayout < 0) currentLayout = filteredLayouts.Count - 1;
		currentPage = 0;
		Validated = false;
		RefreshPage();
	}

	public void NextStep() {
		currentPage = Mathf.Clamp(currentPage + 1, 0, filteredLayouts[currentLayout].steps.Length+1);
		RefreshPage();
	}

	public void PreviousStep() {
		currentPage = Mathf.Clamp(currentPage - 1, 0, filteredLayouts[currentLayout].steps.Length+1);
		RefreshPage();
	}

	public void ValidateBuild() {
		Validated = true;
	}
}
