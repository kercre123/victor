using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Vectrosity;

public class BuildInstructionsCube : MonoBehaviour {

	[SerializeField] VectrosityCube vCube;
	[SerializeField] MeshRenderer meshCube;
	
	[SerializeField] public int objectType = 0;
	[SerializeField] public int objectFamily = 0;
	[SerializeField] public ActiveBlockType activeBlockType = ActiveBlockType.Off;
	[SerializeField] public Color baseColor = Color.blue;

	[SerializeField] SpriteRenderer[] symbols;
	[SerializeField] MeshRenderer[] activeCorners;

	int lastPropType = 0;
	bool lastValidated = false;
	bool lastHighlighted = false;
	bool lastHidden = false;
	ActiveBlockType lastActiveBlockType = ActiveBlockType.Off;
	Color lastBaseColor = Color.blue;

	public bool Validated = false;
	public bool Highlighted = false;
	public bool Hidden = false;

	Material meshMaterial = null;
	bool initialized = false;

	public void Initialize() {
		lastPropType = 0;
		lastValidated = false;
		lastHighlighted = false;
		lastHidden = false;
		lastActiveBlockType = activeBlockType;
		lastBaseColor = baseColor;

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
		if(lastPropType != objectType) return true;
		if(lastValidated != Validated) return true;
		if(lastHighlighted != Highlighted) return true;
		if(lastHidden != Hidden) return true;
		if(lastActiveBlockType != activeBlockType) return true;
		if(lastBaseColor != baseColor) return true;

		return false;
	}

	void Refresh() {

		int colorIndex = Mathf.Clamp((int)objectType, 0, 9);
		vCube.SetColor(baseColor);

		if(Highlighted) {
			vCube.Show();
		}
		else {
			vCube.Hide();
		}

		meshCube.enabled = !Hidden;
		if(!Hidden) {
			Color color = baseColor;
			color.a = Validated ? 1f : 0.25f;
			meshMaterial.color = color;

			if(symbols != null) {
				if(objectType == 0) {
					for(int i=0;i<symbols.Length;i++) {
						Sprite sprite = null;
						if(CozmoPalette.instance != null) sprite = CozmoPalette.instance.GetDigitSprite(i+1);
						symbols[i].sprite = sprite;
					}
				}
				else {
					Sprite sprite = null;
					if(CozmoPalette.instance != null) sprite = CozmoPalette.instance.GetSpriteForObjectType(objectType);
					for(int i=0;i<symbols.Length;i++) {
						symbols[i].sprite = sprite;
					}
				}
			}
			if(activeCorners != null) {
				if(objectType == 0) {
					Color activeColor = Color.white;
					if(CozmoPalette.instance != null) activeColor = CozmoPalette.instance.GetColorForActiveBlockType(activeBlockType);
					for(int i=0;i<activeCorners.Length;i++) {
						activeCorners[i].material.color = activeColor;
						activeCorners[i].gameObject.SetActive(true);
					}
				}
				else {
					for(int i=0;i<activeCorners.Length;i++) {
						activeCorners[i].gameObject.SetActive(false);
					}
				}
			}
		}

		lastPropType = objectType;
		lastValidated = Validated;
		lastHighlighted = Highlighted;
		lastHidden = Hidden;
		lastActiveBlockType = activeBlockType;
		lastBaseColor = baseColor;
	}

	public string GetPropTypeName() {

		switch(objectType) {
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
