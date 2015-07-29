using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Vectrosity;

/// <summary>
/// this component wraps the graphic assets used to display 3d unity representations for cozmo's Cubes
/// </summary>
[ExecuteInEditMode]
public class BuildInstructionsCube : MonoBehaviour
{

	#region INSPECTOR FIELDS

	[SerializeField] bool debug = false;
	[SerializeField] VectrosityCube vCube;
	[SerializeField] MeshRenderer meshCube;
	[SerializeField] public CubeType cubeType = 0;
	[SerializeField] public ActiveBlock.Mode activeBlockMode = ActiveBlock.Mode.Off;
	[SerializeField] public Color baseColor = Color.blue;
	[SerializeField] public bool isHeld = false;
	[SerializeField] public bool ignorePosition = false;
	[SerializeField] SpriteRenderer[] symbols;
	[SerializeField] SpriteRenderer[] symbolFrames;
	[SerializeField] MeshRenderer[] activeLights;
	[SerializeField] public float invalidAlpha = 0.25f;
	[SerializeField] Material originalBlockMaterial = null;
	[SerializeField] Material originalCornerMaterial = null;
	[SerializeField] GameObject checkMarkPrefab = null;

	#endregion

	#region PUBLIC MEMBERS

	//if this cube is stacked on another, we store a reference to simplify build validation
	public BuildInstructionsCube cubeBelow = null;
	public BuildInstructionsCube cubeAbove = null;

	public bool isActive { get { return cubeType == CubeType.LIGHT_CUBE; } }

	//translates between cozmo space and unity space for cube
	public Vector3 WorldPosition
	{
		get { return (CozmoUtil.Vector3UnityToCozmoSpace(transform.position) / Size) * CozmoUtil.BLOCK_LENGTH_MM; }
		set { transform.position = (CozmoUtil.Vector3CozmoToUnitySpace(value) / CozmoUtil.BLOCK_LENGTH_MM) * Size; }
	}

	public bool Validated = false;
	public bool Highlighted = false;
	public bool Hidden = false;
	public ObservedObject AssignedObject = null;
	public float Size = 1f;

	public static List<uint> IDsInUse = new List<uint>();

	#endregion

	#region PRIVATE MEMBERS

	GameObject checkMark = null;

	CubeType lastCubeType = CubeType.BULLS_EYE;
	ActiveBlock.Mode lastActiveBlockMode = ActiveBlock.Mode.Off;
	bool lastValidated = false;
	bool lastHighlighted = false;
	bool lastHidden = false;
	Color lastBaseColor = Color.blue;

	Material clonedBlockMaterial = null;
	Material clonedCornerMaterial = null;

	bool initialized = false;
	
	uint visualizationID_01 = 100;
	uint visualizationID_02 = 101;
	uint visualizationID_03 = 102;
	uint visualizationID_04 = 103;

	#endregion

	#region MONOBEHAVIOUR CALLBACKS

	void Awake()
	{
		if(Application.isPlaying)
		{
			checkMark = (GameObject)GameObject.Instantiate(checkMarkPrefab);
			checkMark.transform.SetParent(transform, false);
			
			//ClaimUIDsForVisualization();
		}
	}

	void OnEnable()
	{
		if(!Application.isPlaying) Initialize(); //this is just for previewing in the editor
	}

	
	void Update()
	{
		if(!initialized) return;
		if(Dirty()) Refresh();
		
		if(checkMark != null)
		{
			checkMark.SetActive(Validated);
		}
	}

	void OnDestroy()
	{
		//if(Application.isPlaying) {
		//ReleaseUIDsForVisualization();
		//}
	}

	#endregion

	#region PRIVATE METHODS

	bool Dirty()
	{
		if(lastCubeType != cubeType) return true;
		if(lastValidated != Validated) return true;
		if(lastHighlighted != Highlighted) return true;
		if(lastHidden != Hidden) return true;
		if(lastActiveBlockMode != activeBlockMode) return true;
		if(lastBaseColor != baseColor) return true;

		return false;
	}

	void Refresh()
	{

		if(Application.isPlaying)
		{
			if(debug) Debug.Log("vCube.SetColor(" + baseColor.ToString() + ")");
			vCube.SetColor(baseColor);

			if(Highlighted)
			{
				vCube.Show();
			} else
			{
				vCube.Hide();
			}
		}

		meshCube.enabled = !Hidden;
		if(Hidden)
		{
			if(symbols != null)
			{
				for(int i = 0; i < symbols.Length; i++)
				{
					symbols[i].gameObject.SetActive(false);
				}
			}
			
			if(symbolFrames != null)
			{
				for(int i = 0; i < symbolFrames.Length; i++)
				{
					symbolFrames[i].gameObject.SetActive(false);
				}
			}
			
			if(activeLights != null)
			{
				for(int i = 0; i < activeLights.Length; i++)
				{
					activeLights[i].gameObject.SetActive(false);
				}
			}
		} else
		{

			float alpha = (Validated || !Application.isPlaying) ? 1f : invalidAlpha;

			Color color = baseColor;
			color.a = alpha; //Highlighted ? 0f : 1f;
			clonedBlockMaterial.color = color;
		
			if(symbols != null)
			{

				Sprite sprite = null;
				if(CozmoPalette.instance != null) sprite = CozmoPalette.instance.GetSpriteForObjectType(cubeType);
				for(int i = 0; i < symbols.Length; i++)
				{
					symbols[i].sprite = sprite;
					color = symbols[i].color;
					color.a = alpha;
					symbols[i].color = color;
					symbols[i].gameObject.SetActive(true);
				}
			}

			if(symbolFrames != null)
			{
				for(int i = 0; i < symbolFrames.Length; i++)
				{
					color = symbolFrames[i].color;
					color.a = alpha;
					symbolFrames[i].color = color;
					symbolFrames[i].gameObject.SetActive(true);
				}
			}

			if(activeLights != null)
			{
				if(isActive)
				{
					Color activeColor = Color.black;
					if(CozmoPalette.instance != null) activeColor = CozmoPalette.instance.GetColorForActiveBlockMode(activeBlockMode);
					for(int i = 0; i < activeLights.Length; i++)
					{
						activeColor.a = alpha;
						clonedCornerMaterial.color = activeColor;
						activeLights[i].gameObject.SetActive(true);
					}
				} else
				{
					for(int i = 0; i < activeLights.Length; i++)
					{
						activeLights[i].gameObject.SetActive(false);
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
	
	//these visualization methods were just for debugging in webots, and as such are not currently in use
	
	void ClaimUIDsForVisualization()
	{
		while(IDsInUse.Contains(visualizationID_01)) visualizationID_01 = (uint)Random.Range(0, uint.MaxValue);
		IDsInUse.Add(visualizationID_01);
		
		while(IDsInUse.Contains(visualizationID_02)) visualizationID_02 = (uint)Random.Range(0, uint.MaxValue);
		IDsInUse.Add(visualizationID_02);
		
		while(IDsInUse.Contains(visualizationID_03)) visualizationID_03 = (uint)Random.Range(0, uint.MaxValue);
		IDsInUse.Add(visualizationID_03);
		
		while(IDsInUse.Contains(visualizationID_04)) visualizationID_04 = (uint)Random.Range(0, uint.MaxValue);
		IDsInUse.Add(visualizationID_04);
	}

	void ReleaseUIDsForVisualization()
	{
		IDsInUse.Remove(visualizationID_01);
		IDsInUse.Remove(visualizationID_02);
		IDsInUse.Remove(visualizationID_03);
		IDsInUse.Remove(visualizationID_04);
	}

	void VisualizeLayoutCube(Vector3 idealPos, Color color)
	{
		
		Vector3 xHalf = CozmoUtil.Vector3UnityToCozmoSpace(transform.right) * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;
		Vector3 yHalf = CozmoUtil.Vector3UnityToCozmoSpace(transform.forward) * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;
		Vector3 zHalf = CozmoUtil.Vector3UnityToCozmoSpace(transform.up) * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;
		
		Vector3[] topCorners = new Vector3[] {
			idealPos + xHalf + yHalf + zHalf,
			idealPos - xHalf + yHalf + zHalf,
			idealPos - xHalf - yHalf + zHalf,
			idealPos + xHalf - yHalf + zHalf
		};
		
		Vector3[] bottomCorners = new Vector3[] {
			idealPos + xHalf + yHalf - zHalf,
			idealPos - xHalf + yHalf - zHalf,
			idealPos - xHalf - yHalf - zHalf,
			idealPos + xHalf - yHalf - zHalf
		};
		
		RobotEngineManager.instance.VisualizeQuad(visualizationID_01, CozmoPalette.ColorToUInt(color), topCorners[0], topCorners[1], topCorners[2], topCorners[3]);
		RobotEngineManager.instance.VisualizeQuad(visualizationID_02, CozmoPalette.ColorToUInt(color), bottomCorners[0], bottomCorners[1], bottomCorners[2], bottomCorners[3]);
	}

	void VisualizeActualCube(ObservedObject obj, Color color)
	{
		
		Vector3 pos = obj.WorldPosition;
		
		Vector3 xHalf = obj.Right * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;
		Vector3 yHalf = obj.Forward * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;
		Vector3 zHalf = obj.Up * CozmoUtil.BLOCK_LENGTH_MM * 0.5f;
		
		Vector3[] topCorners = new Vector3[] {
			pos + xHalf + yHalf + zHalf,
			pos - xHalf + yHalf + zHalf,
			pos - xHalf - yHalf + zHalf,
			pos + xHalf - yHalf + zHalf
		};
		
		Vector3[] bottomCorners = new Vector3[] {
			pos + xHalf + yHalf - zHalf,
			pos - xHalf + yHalf - zHalf,
			pos - xHalf - yHalf - zHalf,
			pos + xHalf - yHalf - zHalf
		};
		
		RobotEngineManager.instance.VisualizeQuad(visualizationID_03, CozmoPalette.ColorToUInt(color), topCorners[0], topCorners[1], topCorners[2], topCorners[3]);
		RobotEngineManager.instance.VisualizeQuad(visualizationID_04, CozmoPalette.ColorToUInt(color), bottomCorners[0], bottomCorners[1], bottomCorners[2], bottomCorners[3]);
	}

	#endregion

	#region PUBLIC METHODS

	public void Initialize()
	{
		if(initialized) return;
		
		BoxCollider box = GetComponent<BoxCollider>();
		Size = box.size.x * transform.lossyScale.x;
		
		if(clonedBlockMaterial == null || clonedBlockMaterial.name != originalBlockMaterial.name)
		{
			clonedBlockMaterial = new Material(originalBlockMaterial);
		}
		
		if(clonedCornerMaterial == null || clonedBlockMaterial.name != originalCornerMaterial.name)
		{
			clonedCornerMaterial = new Material(originalCornerMaterial);
		}
		
		lastCubeType = CubeType.BULLS_EYE;
		lastValidated = false;
		lastHighlighted = false;
		lastHidden = false;
		lastActiveBlockMode = activeBlockMode;
		lastBaseColor = baseColor;
		
		meshCube.material = clonedBlockMaterial;
		foreach(MeshRenderer rend in activeLights) rend.material = clonedCornerMaterial;
		
		if(Application.isPlaying) vCube.CreateWireFrame();
		initialized = true;
		Refresh();
		
		//Debug.Log(gameObject.name + " BuildInstructionsCube.Initialize");
	}

	public bool SatisfiedByObject(ObservedObject obj, float flatFudge, float verticalFudge, float angleFudge, bool allowCardinalAngleOffsets = true, bool debug = false)
	{

		if(!MatchesObject(obj, true, debug))
		{
			return false;
		}
		if(!ignorePosition && !MatchesPosition(obj, obj.WorldPosition, flatFudge, verticalFudge, debug))
		{
			if(debug) Debug.Log("SatisfiedByObject failed because obj(" + obj + ").cubeType(" + obj.cubeType + ") MatchesPosition failed against " + gameObject.name);
			return false;
		}
		if(!MatchesRotation(obj.Rotation, angleFudge, allowCardinalAngleOffsets))
		{
			if(debug) Debug.Log("SatisfiedByObject failed to satisfy because obj(" + obj + ")'s rotation doesnt match " + gameObject.name);
			return false;
		}

		return true;
	}

	public bool PredictSatisfaction(ObservedObject obj, Vector3 predictedPos, Quaternion predictedRot, float flatFudge, float verticalFudge, float angleFudge, bool allowCardinalAngleOffsets = true)
	{
		
		if(!MatchesObject(obj)) return false;
		if(!MatchesPosition(predictedPos, flatFudge, verticalFudge)) return false;
		if(!MatchesRotation(predictedRot, angleFudge, allowCardinalAngleOffsets)) return false;
		
		return true;
	}

	public bool MatchesObject(ObservedObject obj, bool ignoreColor = true, bool debug = false)
	{
		if(obj == null)
		{
			if(debug) Debug.Log("MatchesObject failed because obj is null and therefore cannot possibly match " + gameObject.name);
			return false;
		}
		if(obj.cubeType != cubeType)
		{
			if(debug) Debug.Log("MatchesObject failed because obj(" + obj + ").cubeType(" + obj.cubeType + ") doesn't match " + gameObject.name);
			return false;
		}
		if(!ignoreColor && obj.isActive)
		{
			ActiveBlock activeBlock = obj as ActiveBlock;
			if(activeBlock.mode != activeBlockMode)
			{
				if(debug) Debug.Log("MatchesObject failed because activeBlock(" + obj + ").mode(" + activeBlock.mode + ") doesn't match " + gameObject.name);
				return false;
			}
		}

		return true;
	}

	public bool MatchesPosition(Vector3 actualPos, float flatFudge, float verticalFudge, bool debug = false)
	{
		return MatchesPosition(null, actualPos, flatFudge, verticalFudge, debug);
	}

	//handing through observedObject solely for debugging
	public bool MatchesPosition(ObservedObject obj, Vector3 actualPos, float flatFudge, float verticalFudge, bool debug = false)
	{
	
		Vector3 idealPos = WorldPosition;
		
		Vector3 error = actualPos - idealPos;
		
		if(Mathf.Abs(error.z) > (verticalFudge * CozmoUtil.BLOCK_LENGTH_MM))
		{
			if(debug) Debug.Log("MatchesPosition failed because actualPos.z(" + actualPos.z + ") is too far from idealPos.z(" + idealPos.z + ")");
//			VisualizeLayoutCube(idealPos, Color.magenta);
//			VisualizeActualCube(obj, Color.cyan);
			return false;
		}
		float flatRange = ((Vector2)error).magnitude;
		float rangeFudge = flatFudge * CozmoUtil.BLOCK_LENGTH_MM;
		if(flatRange > rangeFudge)
		{
//			VisualizeLayoutCube(idealPos, Color.magenta);
//			VisualizeActualCube(obj, Color.cyan);
			if(debug) Debug.Log("MatchesPosition failed because flatRange(" + flatRange + ") is higher than rangeFudge(" + rangeFudge + ")");
			return false;
		}
		
		return true;
	}

	public bool MatchesRotation(Quaternion actualRot, float angleFudge, bool allowCardinalAngleOffsets)
	{
		//dmd todo: try to care about rotation

		//Vector3 idealFacing = CozmoUtil.Vector3UnityToCozmoSpace(transform.forward);
		//float idealHeading = MathUtil.SignedVectorAngle(Vector2.right, (Vector2)idealFacing) * Mathf.Deg2Rad;
		//Vector3 actualFacing = obj.Forward;

		return true;
	}

	public Vector3 GetCozmoSpacePose(out float facing_rad)
	{
		facing_rad = 0f;

		Vector3 idealPos = (CozmoUtil.Vector3UnityToCozmoSpace(transform.position) / Size) * CozmoUtil.BLOCK_LENGTH_MM;
		Vector3 idealFacing = CozmoUtil.Vector3UnityToCozmoSpace(transform.forward);
		facing_rad = MathUtil.SignedVectorAngle(Vector2.right, (Vector2)idealFacing, Vector3.forward) * Mathf.Deg2Rad;

		return idealPos;
	}

	#endregion

}
