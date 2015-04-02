using UnityEngine;
using System.Collections;
using Vectrosity;

public class BuildInstructionsCube : MonoBehaviour {

	[SerializeField] VectrosityCube vCube;
	[SerializeField] MeshRenderer meshCube;
	
	[SerializeField] Color[] typeColors = new Color[10];
	[SerializeField] public int propType = 0;

	int lastPropType = -1;
	bool lastValidated = false;
	bool lastHighlighted = false;
	bool lastHidden = false;

	public bool Validated = false;
	public bool Highlighted = false;
	public bool Hidden = false;

	Material meshMaterial = null;
	bool initialized = false;

	public void Initialize() {
		lastPropType = -1;
		lastValidated = false;
		lastHighlighted = false;
		lastHidden = false;

		meshMaterial = meshCube.material;
		vCube.CreateWireFrame();
		initialized = true;
		Refresh();
	}

	void Update () {
		if(!initialized) return;
		if(Dirty()) Refresh();
	}

	bool Dirty() {
		if(lastPropType != propType) return true;
		if(lastValidated != Validated) return true;
		if(lastHighlighted != Highlighted) return true;
		if(lastHidden != Hidden) return true;

		return false;
	}

	void Refresh() {

		int colorIndex = Mathf.Clamp(propType, 0, 9);
		vCube.SetColor(typeColors[colorIndex]);

		if(Highlighted) {
			vCube.Show();
		}
		else {
			vCube.Hide();
		}

		meshCube.enabled = !Hidden;
		if(!Hidden) {
			Color color = typeColors[colorIndex];
			color.a = Validated ? 1f : 0.25f;
			meshMaterial.color = color;
		}

		lastPropType = propType;
		lastValidated = Validated;
		lastHighlighted = Highlighted;
		lastHidden = Hidden;
	}

	public string GetPropTypeName() {

		switch(propType) {
			case 0: return "Basic";
			case 1: return "Bomb";
			case 2: return "Green";
			case 3: return "Blue";
			case 4: return "Cyan";
			case 5: return "Gold";
			case 6: return "Pink";
			case 7: return "Purple";
			case 8: return "Orange";
			case 9: return "White";
		}

		return "Basic";
	}

	public string GetPropFamilyName() {
		return "Cube";
	}

}
