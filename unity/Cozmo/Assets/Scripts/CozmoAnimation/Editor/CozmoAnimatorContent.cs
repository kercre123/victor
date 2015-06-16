using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;

/// <summary>
/// Cozmo Animator content area; based loosely on uSequencer source.
/// </summary>
[Serializable]
public class CozmoAnimatorContent : ScriptableObject
{
	private struct ComponentInfo {
		public readonly Type type;
		public readonly string name;
		public readonly string folder;
		public readonly Type drawerType;

		public ComponentInfo(Type type, string name, Type drawerType)
		{
			this.type = type;
			this.name = name;
			this.folder = name.Replace (" ", "");
			this.drawerType = drawerType;
		}
	}
	
	private static readonly ComponentInfo[] ComponentTypes = new ComponentInfo[]
	{
		new ComponentInfo(typeof(HeadAngleComponent), "Head Angle", typeof(CozmoAnimatorFloatDrawer)),
		new ComponentInfo(typeof(LiftHeightComponent), "Lift Height", typeof(CozmoAnimatorFloatDrawer)),
		new ComponentInfo(typeof(VelocityComponent), "Velocity", typeof(CozmoAnimatorFloatDrawer)),
		new ComponentInfo(typeof(TurnRadiusComponent), "Turn Radius", typeof(CozmoAnimatorFloatDrawer)),
		new ComponentInfo(typeof(FaceImageComponent), "Face Image", typeof(CozmoAnimatorTextureDrawer)),
		new ComponentInfo(typeof(RobotAudioComponent), "Robot Audio", typeof(CozmoAnimatorAudioDrawer)),
		new ComponentInfo(typeof(BasestationAudioComponent), "Basestation Audio", typeof(CozmoAnimatorAudioDrawer)),
		new ComponentInfo(typeof(RgbLightComponent), "RGB Light Front", typeof(CozmoAnimatorColorDrawer)),
		new ComponentInfo(typeof(RgbLightComponent), "RGB Light Middle", typeof(CozmoAnimatorColorDrawer)),
		new ComponentInfo(typeof(RgbLightComponent), "RGB Light Back", typeof(CozmoAnimatorColorDrawer)),
		new ComponentInfo(typeof(BlueLightComponent), "Blue Light Left", typeof(CozmoAnimatorColorDrawer)),
		new ComponentInfo(typeof(BlueLightComponent), "Blue Light Right", typeof(CozmoAnimatorColorDrawer)),
	};
	
	private const float MarginWidth = 192;
	private const float LabelWidth = 126;
	private const float ButtonWidth = 64;
	private const float ItemHeight = 36;

	[SerializeField]
	private float timelineZoom = 1.0f;
	public float TimelineZoom {
		get {
			return timelineZoom;
		}
	}

	[SerializeField]
	private float timelineOffset = 0.0f;
	public float TimelineOffset {
		get {
			return timelineOffset;
		}
	}

	[SerializeField]
	private CozmoAnimatorComponentDrawer[] drawers;

	[SerializeField]
	private EditorWindow editorWindow;
	public EditorWindow EditorWindow
	{
		get { return editorWindow; }
		set { editorWindow = value; }
	}
	
	[SerializeField]
	private CozmoAnimation currentAnimation;

	public CozmoAnimation CurrentAnimation {
		get {
			return currentAnimation;
		}
	}

	[SerializeField]
	private bool snap;
	public bool Snap
	{
		get { return snap; }
		set { snap = value; }
	}
	
	public float SnapAmount
	{
		get { return EditorPrefs.GetFloat("Anki/Cozmo/SnapAmout", 1.0f); }
		set { EditorPrefs.SetFloat("Anki/Cozmo/SnapAmout", value); }
	}
	
	private void OnEnable() 
	{
		if (currentAnimation != null) {
			UpdateCachedMarkerInformation ();
		}
	}
	
	private void OnDestroy()
	{
	}
	
	public void DoGUI(Rect position)
	{
		EditorGUILayout.BeginVertical ();
			
		Rect drawerPosition = position;
		drawerPosition.xMin += MarginWidth;
		drawerPosition.height = ItemHeight;
		drawerPosition.y += 18;
		Debug.Log ("position: " + position + " drawer: " + drawerPosition);

		// TODO: Fix messed up components
		Array.Resize (ref CurrentAnimation.components, ComponentTypes.Length);
		Array.Resize (ref drawers, ComponentTypes.Length);
		for (int i = 0; i < ComponentTypes.Length; ++i) {

			ComponentInfo info = ComponentTypes[i];

			EditorGUILayout.BeginHorizontal();

			CozmoAnimationComponent component = CurrentAnimation.components [i];

			EditorGUILayout.BeginVertical (WellFired.USEditorUtility.USeqSkin.GetStyle("TimelineLeftPaneBackground"), GUILayout.Width (MarginWidth));

			EditorGUILayout.BeginHorizontal();
			EditorGUILayout.LabelField (ComponentTypes [i].name, EditorStyles.boldLabel, GUILayout.MaxWidth(LabelWidth));
			GUILayout.FlexibleSpace();
			if (GUILayout.Button("Add New", EditorStyles.miniButton, GUILayout.Width(ButtonWidth))) {
				component = (CozmoAnimationComponent)CreateNewComponent(info.type, info.name, info.folder);
				EditorGUIUtility.ExitGUI();
			}
			EditorGUILayout.EndHorizontal();

			component = (CozmoAnimationComponent)EditorGUILayout.ObjectField (component, ComponentTypes [i].type, false, GUILayout.Width(MarginWidth));
			CurrentAnimation.components [i] = component;

			EditorGUILayout.EndVertical ();

			if (component != null) {
				if (drawers[i] == null || drawers[i].GetType () != info.drawerType) {
					drawers[i] = (CozmoAnimatorComponentDrawer)ScriptableObject.CreateInstance(info.drawerType);
				}
				drawers[i].SetComponent(this, drawerPosition, component);

				drawers[i].DoGUI();
			}

			EditorGUILayout.EndHorizontal ();

			drawerPosition.y += ItemHeight;
		}

		GUILayout.FlexibleSpace();
		EditorGUILayout.EndVertical ();
	}

	public static ScriptableObject CreateNewComponent(Type type, string name, string folder)
	{
		string filename = EditorUtility.SaveFilePanelInProject (string.Format ("Create new Cozmo {0}...", name), "Untitled", "asset", CozmoAnimatorWindow.RootFolderPath + "/" + folder);
		if (string.IsNullOrEmpty (filename)) {
			return null;
		}

		if (AssetDatabase.AssetPathToGUID (filename) != null) {
			AssetDatabase.DeleteAsset(filename);
		}
		
		ScriptableObject newComponent = (ScriptableObject)ScriptableObject.CreateInstance(type);
		newComponent.name = Path.GetFileNameWithoutExtension (filename);
		AssetDatabase.CreateAsset (newComponent, filename);
		Undo.RegisterCreatedObjectUndo (newComponent, "Created new Cozmo " + name);

		return newComponent;
	}

	public void UpdateCachedMarkerInformation()
	{

	}

	public void SwitchAnimation(CozmoAnimation newAnimation)
	{
		Undo.RegisterCompleteObjectUndo(new UnityEngine.Object[] { this, EditorWindow }, "Select new animation");

		currentAnimation = newAnimation;
		UpdateCachedMarkerInformation();

		EditorWindow.Repaint();
	}

	public float GetXForTime(float time)
	{
		if (CurrentAnimation == null || CurrentAnimation.duration <= 0) {
			return 0;
		}

		float x = time / (CurrentAnimation.duration * timelineZoom) + timelineOffset;
		x = (x * (EditorWindow.position.width - MarginWidth)) + MarginWidth;
		return x;
	}

	public float GetTimeForX(float x)
	{
		if (CurrentAnimation == null || CurrentAnimation.duration <= 0) {
			return 0;
		}

		float time = (x - MarginWidth) / (EditorWindow.position.width - MarginWidth);
		time = (time - timelineOffset) * (CurrentAnimation.duration * timelineZoom);
		return time;
	}
}
