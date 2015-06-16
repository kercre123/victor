using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	/// <summary>
	/// Unity cannot serialize an ScriptableObject that implements an interface correctly, so this is an Abstract class rather than an Interface.
	/// 
	/// This Interface is the base object that everything wishing to render in the uSequencer window must derive from.
	/// </summary>
	public abstract class IUSHierarchyItem : ScriptableObject
	{
		[SerializeField]
		private List<IUSHierarchyItem> _children;
		public List<IUSHierarchyItem> Children
		{
			get { return _children; }
			set { _children = value; }
		}
		
		[SerializeField]
		private float currentXMarkerDist;
		public float CurrentXMarkerDist
		{
			get { return currentXMarkerDist; }
			set { currentXMarkerDist = value; }
		}
		
		[SerializeField]
		public EditorWindow EditorWindow 
		{
			get;
			set;
		}

		[SerializeField]
		private bool isExpanded;
		/// <summary>
		/// Gets or sets a value indicating whether this instance is expanded.
		/// </summary>
		/// <value><c>true</c> if this instance is expanded; otherwise, <c>false</c>.</value>
		public bool IsExpanded 
		{
			get { return isExpanded; }
			set { isExpanded = value; }
		}
		
		[SerializeField]
		/// <summary>
		/// Gets or sets the X scale. This is changed when the user zooms in or out on the uSequencer window.
		/// </summary>
		/// <value>The X scale.</value>
		public float XScale
		{
			get;
			set;
		}
		
		[SerializeField]
		/// <summary>
		/// Gets or sets the X scroll. This is changed when the user scrolls around in the uSequencer window.
		/// </summary>
		/// <value>The X scroll.</value>
		public float XScroll
		{
			get;
			set;
		}
		
		[SerializeField]
		/// <summary>
		/// Gets or sets the Y scroll. This is changed when the user scrolls around in the uSequencer window.
		/// </summary>
		/// <value>The Y scroll.</value>
		public float YScroll
		{
			get;
			set;
		}

		/// <summary>
		/// On a GUI.Repaint, we cache the rect of this hierarchy item. This happens automatically, you don't need to do anything.
		/// </summary>
		/// <value>The Rect that represents our visible area in the uSequencer window.</value>
		public Rect LastRect
		{
			get;
			set;
		}

		public virtual int SortOrder { get; set; }
		public virtual string SortName { get; set;}
		public float FloatingWidth { get; set; }
		public bool ShowOnlyAnimated { get; set; }
		public bool ForceContextClick { get; set; }
		protected Rect FloatingBackgroundRect { get; set; }
		protected Rect ContentBackgroundRect { get; set; }
		private Rect FoldOutRect { get; set; }
		public abstract bool ShouldDisplayMoreButton { get; set; }
		
		/// <summary>
		/// Adds a child item and sorts the hierarchy
		/// </summary>
		/// <param name="child">Child.</param>
		public void AddChild(IUSHierarchyItem child)
		{
			Children.Add(child);
			Children.Sort(IUSHierarchyItem.Sorter);
		}

		/// <summary>
		/// Removes a child and DOES NOT sort the hierarchy.
		/// </summary>
		/// <param name="child">Child.</param>
		public void RemoveChild(IUSHierarchyItem child)
		{
			Children.Remove(child);
		}
		
		/// <summary>
		/// If you want your hierarchy item to have selectable or draggable objects, you should build a new object and implement the ISelectableContainer interface, returning that object.
		/// </summary>
		/// <returns>A list of all selectable containers this class </returns>
		public virtual List<ISelectableContainer> GetISelectableContainers()
		{
			return null;
		}
		
		/// <summary>
		/// If your timeline item has selectable objects, you should return true from this method. By default, this method returns false if the hierarchy item is not expanded.
		/// If you return true from this, you should also implement GetISelectableContainers.
		/// </summary>
		/// <returns><c>true</c> if this instance is I selectable; otherwise, <c>false</c>.</returns>
		public virtual bool IsISelectable()
		{
			return IsExpanded;
		}

		/// <summary>
		/// This is out background style, cached in the base, so the visuals can be consistant.
		/// </summary>
		/// <value>The floating background.</value>
		public GUIStyle FloatingBackground
		{
			get
			{
				return USEditorUtility.USeqSkin.GetStyle("TimelineLeftPaneBackground");
			}
		}
	
		/// <summary>
		/// This is out background style, cached in the base, so the visuals can be consistant.
		/// </summary>
		/// <value>The timeline background.</value>
		public GUIStyle TimelineBackground
		{
			get
			{
				return USEditorUtility.USeqSkin.GetStyle("TimelinePaneBackground");
			}
		}
	
		/// <summary>
		/// This will return you a consistant and deterministic X Offset depending on how deep you are in the hierarchy, the depth is passed to the DoGUI method.
		/// </summary>
		/// <returns>The X offset for depth.</returns>
		/// <param name="depth">Depth.</param>
		protected float GetXOffsetForDepth(int depth)
		{
			return depth * 10.0f;
		}
	
		public virtual void OnEnable()
		{
			hideFlags = HideFlags.HideAndDontSave;
			
			if(Children == null)
				Children = new List<IUSHierarchyItem>();
		}
	
		/// <summary>
		/// If you need to implement any logic that should be updated periodically, or after a relatively heavy change (GUI Resize, do it here.)
		/// </summary>
		public virtual void CheckConsistency()
		{
			if(Children == null)
				return;
	
			if(EditorWindow == null)
				throw new Exception("Must Set Editor Window on GUI Hierarchy Root, so that it gets passed to all children.");

			foreach(var child in Children)
			{
				child.XScale = XScale;
				child.XScroll = XScroll;
				child.YScroll = YScroll;
				child.CurrentXMarkerDist = CurrentXMarkerDist;
				if(child.EditorWindow != EditorWindow)
				{
					child.EditorWindow = EditorWindow;
					EditorWindow.Repaint();
				}
				child.CheckConsistency();
			}
		}

		/// <summary>
		/// This method will store all base states of any objects in the uSequencer hierarchy, this will happen as soon as the user presses the Play Button in the uSequencer Window.
		/// </summary>
		public virtual void StoreBaseState()
		{
			foreach(var child in Children)
				child.StoreBaseState();
		}

		/// <summary>
		/// This method will restore all base states of any objects in the uSequencer hierarchy, this will happen as soon as the user presses the Stop Button in the uSequencer Window.
		/// </summary>
		public virtual void RestoreBaseState()
		{
			foreach(var child in Children)
				child.RestoreBaseState();
		}
	
		public virtual void Expand()
		{
			USUndoManager.RegisterCompleteObjectUndo(this, "Expand All");
			IsExpanded = true;
			foreach(var child in Children)
				child.Expand();
		}
		
		public virtual void HideAll()
		{
			USUndoManager.RegisterCompleteObjectUndo(this, "Hide All");
			IsExpanded = false;
			foreach(var child in Children)
				child.HideAll();
		}

		public virtual void ExternalModification()
		{
			foreach(var child in Children)
				child.ExternalModification();
		}
	
		public virtual void OnSceneGUI()
		{
			if(!IsExpanded)
				return;
	
			foreach(var child in Children)
				child.OnSceneGUI();
		}
	
		public virtual void Initialize(USTimelineBase timeline) { ; }

		/// <summary>
		/// You can implement this method to add custom menut items to the context menu for this Timeline as such.
		/// 
		/// contextMenu.AddItem(new GUIContent("My Context Item/My Sub Context Menu Item"), false, MyDelegate, null);
		/// 
		/// More information can be found at Unity's official page : https://docs.unity3d.com/353/Documentation/ScriptReference/GenericMenu.html
		/// 
		/// </summary>
		/// <param name="contextMenu">The contextMenu that will be executed.</param>
		public virtual void AddContextItems(GenericMenu contextMenu) { ; }

		/// <summary>
		/// This method should be implemented to draw the visualisation of your timeline
		/// </summary>
		/// <param name="depth">Depth.</param>
		public abstract void DoGUI(int depth);
		
		protected virtual void FloatingOnGUI(int depth)
		{
			if(Event.current.type == EventType.Repaint)
				FoldOutRect = GUILayoutUtility.GetLastRect();

			FloatingOnGUI(depth, FoldOutRect);
		}

		protected virtual void FloatingOnGUI(int depth, Rect parentRect)
		{
			if(ShouldDisplayMoreButton)
			{
				float size = 16.0f;
				var moreButtonRect = new Rect(parentRect.width - size, parentRect.y, size, size);
				
				if(GUI.Button(moreButtonRect, new GUIContent(USEditorUtility.MoreButton, "Options for this Timeline Container"), USEditorUtility.ButtonNoOutline))
					ForceContextClick = true;
			}
		}

		public static int Sorter(IUSHierarchyItem x, IUSHierarchyItem y)
		{
			int result = x.SortOrder.CompareTo(y.SortOrder);

			if(result == 0 && x.SortName != null && y.SortName != null) 
				result = x.SortName.CompareTo(y.SortName);
		
			return result;
		}
	}
}