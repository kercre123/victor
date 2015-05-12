using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Vectrosity;

[ExecuteInEditMode]
public class BuildInstructionsCube : MonoBehaviour {

	//[SerializeField] bool debug = false;

	[SerializeField] VectrosityCube vCube;
	[SerializeField] MeshRenderer meshCube;
	
	[SerializeField] public int objectType = 0;
	[SerializeField] public int objectFamily = 0;
	[SerializeField] public ActiveBlock.Mode activeBlockMode = ActiveBlock.Mode.Off;
	[SerializeField] public Color baseColor = Color.blue;

	//if this cube is stacked on another, we store a reference to simplify build validation
	[SerializeField] public BuildInstructionsCube cubeBelow = null;

	[SerializeField] SpriteRenderer[] symbols;
	[SerializeField] SpriteRenderer[] symbolFrames;
	[SerializeField] MeshRenderer[] activeCorners;

	[SerializeField] public float invalidAlpha = 0.25f;
	[SerializeField] Material originalBlockMaterial = null;
	[SerializeField] Material originalCornerMaterial = null;

	[SerializeField] GameObject checkMarkPrefab = null;

	GameObject checkMark = null;

	int lastPropType = 0;
	int lastFamilyType = 0;
	bool lastValidated = false;
	bool lastHighlighted = false;
	bool lastHidden = false;
	ActiveBlock.Mode lastActiveBlockMode = ActiveBlock.Mode.Off;
	Color lastBaseColor = Color.blue;

	public bool Validated = false;
	public bool Highlighted = false;
	public bool Hidden = false;
	public ObservedObject AssignedObject = null;
	public float Size = 1f;

	Material clonedBlockMaterial = null;
	Material clonedCornerMaterial = null;

	bool initialized = false;

	void Awake() {
		if(Application.isPlaying) {
			checkMark = (GameObject)GameObject.Instantiate(checkMarkPrefab);
			checkMark.transform.SetParent(transform, false);
		}
	}

	void OnEnable() {
		if(!Application.isPlaying) Initialize();
	}

	public void Initialize() {
		if(initialized) return;

		BoxCollider box = GetComponent<BoxCollider>();
		Size = box.size.x;
		
		if(clonedBlockMaterial == null || clonedBlockMaterial.name != originalBlockMaterial.name) {
			clonedBlockMaterial = new Material(originalBlockMaterial);
		}
		
		if(clonedCornerMaterial == null || clonedBlockMaterial.name != originalCornerMaterial.name) {
			clonedCornerMaterial = new Material(originalCornerMaterial);
		}

		lastPropType = 0;
		lastFamilyType = 0;
		lastValidated = false;
		lastHighlighted = false;
		lastHidden = false;
		lastActiveBlockMode = activeBlockMode;
		lastBaseColor = baseColor;

		meshCube.material = clonedBlockMaterial;
		foreach(MeshRenderer rend in activeCorners) rend.material = clonedCornerMaterial;

		if(Application.isPlaying) vCube.CreateWireFrame();
		initialized = true;
		Refresh();

		//Debug.Log(gameObject.name + " BuildInstructionsCube.Initialize");
	}

	void Update () {
		if(!initialized) return;
		if(Dirty()) Refresh();

		if(checkMark != null) {
			checkMark.SetActive(Validated);
		}
	}

	bool Dirty() {
		if(lastPropType != objectType) return true;
		if(lastFamilyType != objectFamily) return true;
		if(lastValidated != Validated) return true;
		if(lastHighlighted != Highlighted) return true;
		if(lastHidden != Hidden) return true;
		if(lastActiveBlockMode != activeBlockMode) return true;
		if(lastBaseColor != baseColor) return true;

		return false;
	}

	void Refresh() {

		if(Application.isPlaying) {

			vCube.SetColor(baseColor);

			//if(Highlighted) {
			//	vCube.Show();
			//}
			//else {
				vCube.Hide();
			//}
		}

		meshCube.enabled = !Hidden;
		if(Hidden) {
			if(symbols != null) {
				for(int i=0;i<symbols.Length;i++) {
					symbols[i].gameObject.SetActive(false);
				}
			}
			
			if(symbolFrames != null) {
				for(int i=0;i<symbolFrames.Length;i++) {
					symbolFrames[i].gameObject.SetActive(false);
				}
			}
			
			if(activeCorners != null) {
				for(int i=0;i<activeCorners.Length;i++) {
					activeCorners[i].gameObject.SetActive(false);
				}
			}
		}
		else {

			float alpha = (Validated || !Application.isPlaying) ? 1f : invalidAlpha;

			Color color = baseColor;
			color.a = alpha; //Highlighted ? 0f : 1f;
			clonedBlockMaterial.color = color;
		
			if(symbols != null) {
				if(objectFamily == 3) {
					for(int i=0;i<symbols.Length;i++) {
						Sprite sprite = null;
						if(CozmoPalette.instance != null) sprite = CozmoPalette.instance.GetDigitSprite(i+1);
						symbols[i].sprite = sprite;
						color = symbols[i].color;
						color.a = alpha;
						symbols[i].color = color;
						symbols[i].gameObject.SetActive(true);
					}
				}
				else {
					Sprite sprite = null;
					if(CozmoPalette.instance != null) sprite = CozmoPalette.instance.GetSpriteForObjectType(objectType);
					for(int i=0;i<symbols.Length;i++) {
						symbols[i].sprite = sprite;
						color = symbols[i].color;
						color.a = alpha;
						symbols[i].color = color;
						symbols[i].gameObject.SetActive(true);
					}
				}
			}

			if(symbolFrames != null) {
				for(int i=0;i<symbolFrames.Length;i++) {
					color = symbolFrames[i].color;
					color.a = alpha;
					symbolFrames[i].color = color;
					symbolFrames[i].gameObject.SetActive(true);
				}
			}

			if(activeCorners != null) {
				if(objectFamily == 3) {
					Color activeColor = Color.white;
					if(CozmoPalette.instance != null) activeColor = CozmoPalette.instance.GetColorForActiveBlockMode(activeBlockMode);
					for(int i=0;i<activeCorners.Length;i++) {
						activeColor.a = alpha;
						clonedCornerMaterial.color = activeColor;
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
		lastFamilyType = objectFamily;
		lastValidated = Validated;
		lastHighlighted = Highlighted;
		lastHidden = Hidden;
		lastActiveBlockMode = activeBlockMode;
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
