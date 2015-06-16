using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;

/// <summary>
/// Cozmo Animator window; based loosely on uSequencer source.
/// </summary>
[Serializable]
public class CozmoAnimatorWindow : EditorWindow
{
	private static readonly Vector2 MinWindowSize = new Vector2(750.0f, 250.0f);

	private static readonly GUIContent createNewText = new GUIContent("Create New Animation");
	private static readonly GUIContent nothingSelectedNotification = new GUIContent ("Please select an existing CozmoAnimation asset or create a new one.");

	public const string RootFolderPath = "Assets/CozmoAnimation";
	public const string AnimationFolderPath = RootFolderPath + "/Animation";

	[SerializeField]
	private CozmoAnimatorContent contentDrawer;
	public CozmoAnimatorContent ContentDrawer
	{
		get {
			return contentDrawer;
		}
		set {
			contentDrawer = value;
		}
	}

	public CozmoAnimation CurrentAnimation {
		get {
			return ContentDrawer.CurrentAnimation;
		}
	}
	
	private Rect TopBar
	{
		get;
		set;
	}
	
	private Rect Content
	{
		get;
		set;
	}
	
	private Rect BottomBar
	{
		get;
		set;
	}

	[MenuItem("Window/Cozmo Animator %y")]
	public static void OpenWindow()
	{
		CozmoAnimatorWindow window = EditorWindow.GetWindow (typeof(CozmoAnimatorWindow), false, "CozmoAnimator") as CozmoAnimatorWindow;
		window.Show ();
	}

	void OnEnable()
	{
		autoRepaintOnSceneChange = true;
		minSize = MinWindowSize;
		hideFlags = HideFlags.HideAndDontSave;
		
		if (ContentDrawer == null) {
			ContentDrawer = ScriptableObject.CreateInstance<CozmoAnimatorContent> ();
			ContentDrawer.EditorWindow = this;
		}

		Undo.undoRedoPerformed -= UndoRedoCallback;
		Undo.undoRedoPerformed += UndoRedoCallback;

//			thisWindow = this;

//			SceneView.onSceneGUIDelegate -= OnScene;
//			SceneView.onSceneGUIDelegate += OnScene;
//			EditorApplication.playmodeStateChanged -= PlayModeStateChanged;
//			EditorApplication.playmodeStateChanged += PlayModeStateChanged;
//			EditorApplication.update -= SequenceUpdate;
//			EditorApplication.update += SequenceUpdate;
	}

	void OnDestroy()
	{
		Undo.undoRedoPerformed -= UndoRedoCallback;

//			thisWindow = null;
//			SceneView.onSceneGUIDelegate -= OnScene;
//			EditorApplication.playmodeStateChanged -= PlayModeStateChanged;
//			EditorApplication.update -= SequenceUpdate;
		
//			StopProcessingAnimationMode();
		
//			if(CurrentSequence)
//				CurrentSequence.Stop();
		if (ContentDrawer != null) {
			DestroyImmediate (ContentDrawer);
		}
	}
	
	private void UndoRedoCallback()
	{
//			// This hack ensures that we process and update our in progress Sequence when Undo / Redoing
//			if(currentAnimation != null)
//			{
//				var previousRunningTime = CurrentSequence.RunningTime;
//				
//				// if we undo we always revert to our base state.
//				ContentDrawer.RestoreBaseState();
//				
//				// if our running time is then > 0.0f, we process the timeline
//				if(previousRunningTime > 0.0f)
//				{
//					CurrentSequence.RunningTime = previousRunningTime;
//				}
//			}
		
		Repaint();
	}

//		private void PlayModeStateChanged()
//		{
//			StopProcessingAnimationMode();
//			IsArmed = false;
//		}
	
	void OnGUI()
	{
		if (CurrentAnimation == null) {
			ShowNotification (nothingSelectedNotification);
		}
		else {
			RemoveNotification ();
		}

		EditorGUILayout.BeginVertical ();
		DisplayTopBar ();
		DisplayEditableArea();
		DisplayBottomBar();
		EditorGUILayout.EndVertical ();

		/*
		using(new WellFired.Shared.GUIBeginVertical())
		{
			DisplayTopBar();
			DisplayEdittableArea();
			DisplayBottomBar();
		}
		
		ProcessHotkeys();
		
		if(Event.current.type == EventType.DragUpdated)
		{
			if(!WellFired.Animation.IsInAnimationMode)
			{
				DragAndDrop.visualMode = DragAndDropVisualMode.Link;
				Event.current.Use();
			}
			else
			{
				showAnimationModeTime = EditorApplication.timeSinceStartup;
				ShowNotification(new GUIContent(USEditorUtility.AddingNewAffectedObjectWhilstInAnimationMode));
			}
		}
		
		if(Event.current.type == EventType.DragPerform)
		{
			if(!WellFired.Animation.IsInAnimationMode)
			{
				foreach(UnityEngine.Object dragObject in DragAndDrop.objectReferences)
				{
					GameObject GO = dragObject as GameObject;
					if(GO != CurrentSequence.gameObject)
					{					
						DragAndDrop.AcceptDrag();
						
						//Do we already have a timeline for this object
						foreach(USTimelineContainer timelineContainer in CurrentSequence.TimelineContainers)
						{
							if(timelineContainer.AffectedObject == GO.transform)
								return;
						}
						
						var newTimelineContainer = CurrentSequence.CreateNewTimelineContainer(GO.transform);
						USUndoManager.RegisterCreatedObjectUndo(newTimelineContainer.gameObject, "Add New Timeline Container");
						ContentDrawer.AddNewTimelineContainer(newTimelineContainer);
					}
				}
				
				Event.current.Use();
			}
		}
		*/
	}

	private const float TopBarHeight = 18.0f;
	private const float BottomBarHeight = 18.0f;

	private void DisplayTopBar()
	{
		GUILayout.BeginArea(new Rect(0f, 0f, base.position.width, TopBarHeight));
		GUILayout.BeginHorizontal(EditorStyles.toolbar);

		if(GUILayout.Button(createNewText, EditorStyles.toolbarButton)) 
		{
			CreateNew ();
			EditorGUIUtility.ExitGUI();
		}

		CozmoAnimation newAnimation = (CozmoAnimation)EditorGUILayout.ObjectField(CurrentAnimation, typeof(CozmoAnimation), false);
		if (newAnimation != CurrentAnimation) {
			ContentDrawer.SwitchAnimation(newAnimation);
		}

		GUILayout.FlexibleSpace();
		
		if (CurrentAnimation != null)
		{
			EditorGUILayout.LabelField(new GUIContent("Keyframe Snap", "The amount keyframes will snap to when dragged in the Cozmo Animator window. (Left Shift to activate)"), GUILayout.MaxWidth(100.0f));
			WellFired.USUndoManager.BeginChangeCheck();
			float snapAmount = EditorGUILayout.FloatField(new GUIContent("", "The amount keyframes will snap to when dragged in the Cozmo Animator window. (Left Shift to activate)"), ContentDrawer.SnapAmount, GUILayout.MaxWidth(40.0f));
			if(WellFired.USUndoManager.EndChangeCheck())
			{
				WellFired.USUndoManager.PropertyChange(this, "Snap Amount");
				ContentDrawer.SnapAmount = snapAmount;
			}
		}

		GUILayout.EndHorizontal();
		GUILayout.EndArea();
	}

	private void DisplayEditableArea()
	{
		Rect position = new Rect (0f, TopBarHeight, base.position.width, base.position.height - BottomBarHeight - TopBarHeight);
		GUILayout.BeginArea(position);
		if (CurrentAnimation != null) {
			ContentDrawer.DoGUI (position);
		}
		GUILayout.EndArea ();
	}

	private void DisplayBottomBar()
	{
		if(CurrentAnimation == null)
			return;
		
		GUILayout.BeginArea(new Rect(0f, base.position.height - BottomBarHeight, base.position.width, BottomBarHeight));
		EditorGUILayout.BeginHorizontal(EditorStyles.toolbar);

		EditorGUILayout.LabelField("", "Duration", GUILayout.MaxWidth(50.0f));
		WellFired.USUndoManager.BeginChangeCheck();
		float duration = EditorGUILayout.FloatField("", CurrentAnimation.duration, GUILayout.MaxWidth(50.0f));
		if(WellFired.USUndoManager.EndChangeCheck())
		{
			WellFired.USUndoManager.PropertyChange(CurrentAnimation, "duration");
			CurrentAnimation.duration = duration;
			ContentDrawer.UpdateCachedMarkerInformation();
		}
		
		GUILayout.FlexibleSpace();
		EditorGUILayout.EndHorizontal ();
		GUILayout.EndArea ();
	}

	private void ProcessHotkeys()
	{
		// TODO: Playback
//			if(Event.current.rawType == EventType.KeyDown && Event.current.keyCode == KeyCode.P)
//			{
//				PlayOrPause();
//				Event.current.Use();
//			}
//			
//			if(Event.current.rawType == EventType.KeyDown && Event.current.keyCode == KeyCode.S)
//			{
//				Stop();
//				Event.current.Use();
//			}

		// TODO: Selection
//			if(Event.current.rawType == EventType.KeyDown && (Event.current.keyCode == KeyCode.Backspace || Event.current.keyCode == KeyCode.Delete))
//			{
//				foreach(var timelineContainer in contentDrawer.USHierarchy.RootItems)
//				{
//					foreach(var timeline in timelineContainer.Children)
//					{
//						var ISelectableContainers = timeline.GetISelectableContainers();
//						foreach(var ISelectableContainer in ISelectableContainers)
//							ISelectableContainer.DeleteSelection();
//					}
//				}
//				Event.current.Use();
//			}

		if(Event.current.shift)
			ContentDrawer.Snap = true;
		else
			ContentDrawer.Snap = false;

		// TODO: Fast navigation
//			if(Event.current.rawType == EventType.KeyDown && Event.current.keyCode == KeyCode.Period && Event.current.alt)
//			{
//				GoToNextKeyframe();
//				Event.current.Use();
//			}
//			else if(Event.current.rawType == EventType.KeyDown && Event.current.keyCode == KeyCode.Comma && Event.current.alt)
//			{
//				GoToPrevKeyframe();
//				Event.current.Use();
//			}
//			else if(Event.current.type == EventType.KeyDown && Event.current.keyCode == KeyCode.Period)
//			{
//				SetRunningTime((float)Math.Round(CurrentSequence.RunningTime + 0.01f, 2));
//				Event.current.Use();
//			}
//			else if(Event.current.type == EventType.KeyDown && Event.current.keyCode == KeyCode.Comma)
//			{
//				SetRunningTime((float)Math.Round(CurrentSequence.RunningTime - 0.01f, 2));
//				Event.current.Use();
//			}
	}

	private void CreateNew()
	{
		CozmoAnimation animation = (CozmoAnimation)CozmoAnimatorContent.CreateNewComponent (typeof(CozmoAnimation), "Animation", "Animation");
		if (animation == null) {
			return;
		}

		ContentDrawer.SwitchAnimation (animation);

		Repaint();
	}
}
