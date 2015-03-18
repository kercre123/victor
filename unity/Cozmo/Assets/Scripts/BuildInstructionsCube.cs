using UnityEngine;
using System.Collections;
using Vectrosity;

public class BuildInstructionsCube : MonoBehaviour {

	[SerializeField] VectrosityCube vCube;
	[SerializeField] MeshRenderer meshCube;
	
	[SerializeField] Color[] typeColors = new Color[10];
	[SerializeField] Material invalidatedLineMaterial;
	[SerializeField] Material validatedLineMaterial;
	[SerializeField] int propType = 0;

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
	
}
