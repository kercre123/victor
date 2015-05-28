using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Vectrosity;

[ExecuteInEditMode]
public class BuildInstructionsCube : MonoBehaviour {

	//[SerializeField] bool debug = false;

	[SerializeField] VectrosityCube vCube;
	[SerializeField] MeshRenderer meshCube;
	
	[SerializeField] public CubeType cubeType = 0;
	[SerializeField] public ActiveBlock.Mode activeBlockMode = ActiveBlock.Mode.Off;
	[SerializeField] public Color baseColor = Color.blue;
	[SerializeField] public bool isHeld = false;
	[SerializeField] public bool ignorePosition = false;

	//if this cube is stacked on another, we store a reference to simplify build validation
	public BuildInstructionsCube cubeBelow = null;
	public BuildInstructionsCube cubeAbove = null;

	[SerializeField] SpriteRenderer[] symbols;
	[SerializeField] SpriteRenderer[] symbolFrames;
	[SerializeField] MeshRenderer[] activeCorners;

	[SerializeField] public float invalidAlpha = 0.25f;
	[SerializeField] Material originalBlockMaterial = null;
	[SerializeField] Material originalCornerMaterial = null;

	[SerializeField] GameObject checkMarkPrefab = null;

	public bool isActive { get { return cubeType == CubeType.LIGHT_CUBE; } }

	GameObject checkMark = null;

	CubeType lastCubeType = CubeType.BULLS_EYE;
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

		lastCubeType = CubeType.BULLS_EYE;
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
		if(lastCubeType != cubeType) return true;
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

				Sprite sprite = null;
				if(CozmoPalette.instance != null) sprite = CozmoPalette.instance.GetSpriteForObjectType(cubeType);
				for(int i=0;i<symbols.Length;i++) {
					symbols[i].sprite = sprite;
					color = symbols[i].color;
					color.a = alpha;
					symbols[i].color = color;
					symbols[i].gameObject.SetActive(true);
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
				if(isActive) {
					Color activeColor = Color.black;
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

		lastCubeType = cubeType;
		lastValidated = Validated;
		lastHighlighted = Highlighted;
		lastHidden = Hidden;
		lastActiveBlockMode = activeBlockMode;
		lastBaseColor = baseColor;
	}

	public bool SatisfiedByObject(ObservedObject obj, float flatFudge, float verticalFudge, float angleFudge, bool allowCardinalAngleOffsets=true) {

		if(!MatchesObject(obj)) return false;
		if(!ignorePosition && !MatchesPosition(obj.WorldPosition, flatFudge, verticalFudge)) return false;
		if(!MatchesRotation(obj.Rotation, angleFudge, allowCardinalAngleOffsets)) return false;

		return true;
	}

	public bool PredictSatisfaction(ObservedObject obj, Vector3 predictedPos, Quaternion predictedRot, float flatFudge, float verticalFudge, float angleFudge, bool allowCardinalAngleOffsets=true) {
		
		if(!MatchesObject(obj)) return false;
		if(!MatchesPosition(predictedPos, flatFudge, verticalFudge)) return false;
		if(!MatchesRotation(predictedRot, angleFudge, allowCardinalAngleOffsets)) return false;
		
		return true;
	}

	public bool MatchesObject(ObservedObject obj, bool ignoreColor=true) {
		if(obj == null) return false;
		if(obj.cubeType != cubeType) return false;
		if(!ignoreColor && obj.isActive) {
			ActiveBlock activeBlock = obj as ActiveBlock;
			if(activeBlock.mode != activeBlockMode) return false;
		}

		return true;
	}
	

	public bool MatchesPosition(Vector3 actualPos, float flatFudge, float verticalFudge) {
	
		Vector3 idealPos = (CozmoUtil.Vector3UnityToCozmoSpace(transform.position) / Size) * CozmoUtil.BLOCK_LENGTH_MM;
		
		Vector3 error = actualPos - idealPos;
		
		if(Mathf.Abs(error.z) > (verticalFudge * CozmoUtil.BLOCK_LENGTH_MM)) return false;
		if(Mathf.Abs(((Vector2)error).magnitude) > (flatFudge * CozmoUtil.BLOCK_LENGTH_MM)) return false;
		
		return true;
	}

	public bool MatchesRotation(Quaternion actualRot, float angleFudge, bool allowCardinalAngleOffsets) {
		//dmd todo: try to care about rotation

		//Vector3 idealFacing = CozmoUtil.Vector3UnityToCozmoSpace(transform.forward);
		//float idealHeading = MathUtil.SignedVectorAngle(Vector2.right, (Vector2)idealFacing) * Mathf.Deg2Rad;
		//Vector3 actualFacing = obj.Forward;

		return true;
	}

	public Vector3 GetCozmoSpacePose(out float facing_rad) {
		facing_rad = 0f;

		Vector3 idealPos = (CozmoUtil.Vector3UnityToCozmoSpace(transform.position) / Size) * CozmoUtil.BLOCK_LENGTH_MM;
		Vector3 idealFacing = CozmoUtil.Vector3UnityToCozmoSpace(transform.forward);
		facing_rad = MathUtil.SignedVectorAngle(Vector2.right, (Vector2)idealFacing, Vector3.forward) * Mathf.Deg2Rad;

		return idealPos;
	}

}
