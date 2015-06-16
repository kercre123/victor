using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public class USHierarchy : ScriptableObject
	{
		public Action<IUSHierarchyItem> OnItemClicked = delegate {};
		public Action<IUSHierarchyItem> OnItemContextClicked = delegate {};
		public Action<IUSHierarchyItem> OnItemDoubleClicked = delegate {};
	
		private List<ISelectableContainer> iSelectableContainers = new List<ISelectableContainer>();
		
		private Vector2 scrollPos = Vector2.zero;
		public Vector2 ScrollPos
		{
			get { return scrollPos; }
			set { scrollPos = value; }
		}
	
		private bool scrollable = false;
		public bool Scrollable
		{
			get { return scrollable; }
			set { scrollable = value; }
		}
	
		[SerializeField]
		private List<IUSHierarchyItem> rootItems;
		public List<IUSHierarchyItem> RootItems
		{
			get { return rootItems; }
			private set { rootItems = value; }
		}
		
		[SerializeField]
		public float XScale
		{
			get;
			set;
		}
	
		[SerializeField]
		private float currentXMarkerDist;
		public float CurrentXMarkerDist
		{
			get { return currentXMarkerDist; }
			set { currentXMarkerDist = value; }
		}
		
		public float FloatingWidth
		{
			get;
			set;
		}
		
		public Rect TotalArea
		{
			get;
			private set;
		}
	
		public Rect VisibleArea
		{
			get;
			private set;
		}
		
		[SerializeField]
		private EditorWindow editorWindow;
		public EditorWindow EditorWindow 
		{
			get { return editorWindow; }
			set { editorWindow = value; }
		}
		
		private void OnEnable()
		{
			hideFlags = HideFlags.HideAndDontSave;
	
			if(RootItems == null)
				RootItems = new List<IUSHierarchyItem>();
		}
	
		private void OnDestroy()
		{
			RootItems.Clear();
		}
	
		public void DoGUI(int depth)
		{
			if(RootItems == null)
				return;
	
			if(Scrollable)
			{
				GUILayout.Box("", USEditorUtility.ContentBackground, GUILayout.ExpandWidth(true), GUILayout.ExpandHeight(true));
				if(Event.current.type == EventType.Repaint)
				{
					if(VisibleArea != GUILayoutUtility.GetLastRect())
					{
						VisibleArea = GUILayoutUtility.GetLastRect();
						EditorWindow.Repaint();
					}
				}
	
				using(new WellFired.Shared.GUIBeginArea(VisibleArea))
				{
					using(new WellFired.Shared.GUIBeginScrollView(scrollPos, GUIStyle.none, GUIStyle.none))
					{
						using(new WellFired.Shared.GUIBeginVertical())
						{
							for(int i = 0; i < RootItems.Count; i++)
								DrawHierarchyItem(RootItems[i], depth);
	
							using(new WellFired.Shared.GUIBeginHorizontal(GUILayout.MaxWidth(FloatingWidth - 5.0f)))
							{
								if(GUILayout.Button("Expand", GUILayout.MaxWidth(FloatingWidth * 0.5f)))
									ExpandAll();
								if(GUILayout.Button("Hide", GUILayout.MaxWidth(FloatingWidth * 0.5f)))
									HideAll();
							}
						}
						
						if(Event.current.type == EventType.Repaint)
						{
							if(TotalArea != GUILayoutUtility.GetLastRect())
							{
								TotalArea = GUILayoutUtility.GetLastRect();
								EditorWindow.Repaint();
							}
						}
					}
				}
			}
			else
			{
				using(new WellFired.Shared.GUIBeginVertical())
				{
					for (int i = 0; i < RootItems.Count; i++)
						DrawHierarchyItem(RootItems[i], depth);
				}
				
				if(Event.current.type == EventType.Repaint)
				{
					if(TotalArea != GUILayoutUtility.GetLastRect())
					{
						TotalArea = GUILayoutUtility.GetLastRect();
						EditorWindow.Repaint();
					}
				}
			}
		}
	
		private void DrawHierarchyItem(IUSHierarchyItem item, int level)
		{
			using(new WellFired.Shared.GUIBeginVertical())
			{
				item.FloatingWidth = FloatingWidth;
				item.DoGUI(level);
				if(item.IsExpanded)
				{
					// Draw all the children of the item
					for (int i = 0; i < item.Children.Count; i++)
						DrawHierarchyItem(item.Children[i], level + 1);
				}
			}
	
			if(Event.current.type == EventType.Repaint)
				item.LastRect = GUILayoutUtility.GetLastRect();
	
			Rect clickableRect = item.LastRect;
			clickableRect.width = FloatingWidth;
			if(Event.current.type == EventType.MouseDown && Event.current.button == 0 && clickableRect.Contains(Event.current.mousePosition))
			{
				OnItemClicked(item);
				Event.current.Use();
			}
			if(Event.current.type == EventType.MouseDown && Event.current.button == 0 && Event.current.clickCount == 2 && clickableRect.Contains(Event.current.mousePosition))
			{
				OnItemDoubleClicked(item);
				Event.current.Use();
			}
			if(Event.current.type == EventType.MouseDown && Event.current.button == 1 && clickableRect.Contains(Event.current.mousePosition))
			{
				OnItemContextClicked(item);
				Event.current.Use();
			}

			if(item.ForceContextClick)
			{
				OnItemContextClicked(item);
				item.ForceContextClick = false;
			}
		}
		
		public List<ISelectableContainer> ISelectableContainers()
		{
			iSelectableContainers.Clear();
	
			for (int i = 0; i < RootItems.Count; i++)
				GetISelectableContainers(RootItems[i], ref iSelectableContainers);
	
			return iSelectableContainers;
		}
		
		private void GetISelectableContainers(IUSHierarchyItem item, ref List<ISelectableContainer> iSelectableContainers)
		{
			if(!item.IsISelectable())
				return;
	
			var containers = item.GetISelectableContainers();
			if(containers != null && containers.Count > 0)
				iSelectableContainers.AddRange(containers);
	
			for (int i = 0; i < item.Children.Count; i++)
			{
				GetISelectableContainers(item.Children[i], ref iSelectableContainers);
			}
		}
	
		public void AddHierarchyItemToRoot(IUSHierarchyItem hierarchyItem)
		{
			RootItems.Add(hierarchyItem);
			RootItems.Sort(IUSHierarchyItem.Sorter);
		}
		
		public void RemoveHierarchyItemFromRoot(IUSHierarchyItem hierarchyItem)
		{
			RootItems.Remove(hierarchyItem);
		}
	
		public IUSHierarchyItem GetParentOf(IUSHierarchyItem item)
		{
			if(RootItems.Contains(item))
				return null;
	
			IUSHierarchyItem itemsParent = null;
			for (int i = 0; i < RootItems.Count; i++)
			{
				itemsParent = GetParentOfChild(RootItems[i], item);
				if(itemsParent != null)
					break;
			}
	
			return itemsParent;
		}
		
		public virtual void CheckConsistency()
		{
			for (int i = 0; i < RootItems.Count; i++)
			{	
				RootItems[i].EditorWindow = EditorWindow;
				RootItems[i].CurrentXMarkerDist = CurrentXMarkerDist;
				RootItems[i].FloatingWidth = FloatingWidth;
				RootItems[i].XScale = XScale;
				RootItems[i].XScroll = ScrollPos.x;
				RootItems[i].YScroll = ScrollPos.y;
	
				RootItems[i].CheckConsistency();
			}
		}

		public void Reset()
		{
			if(RootItems == null)
				return;

			RootItems.Clear();
		}
	
		public IUSHierarchyItem GetParentOfChild(IUSHierarchyItem parent, IUSHierarchyItem item)
		{
			if(parent.Children.Contains(item))
				return parent;
			
			IUSHierarchyItem itemsParent = null;
			for (int i = 0; i < parent.Children.Count; i++)
			{
				itemsParent = GetParentOfChild(parent.Children[i], item);
				if(itemsParent != null)
					break;
			}
	
			return null;
		}
	
		public void StoreBaseState()
		{
			for (int i = 0; i < RootItems.Count; i++)
				RootItems[i].StoreBaseState();
		}
		
		public void RestoreBaseState()
		{
			for (int i = 0; i < RootItems.Count; i++)
				RootItems[i].RestoreBaseState();
		}
	
		public void ExpandAll()
		{
			USUndoManager.RegisterCompleteObjectUndo(this, "Expand All");
			for (int i = 0; i < RootItems.Count; i++)
				RootItems[i].Expand();
		}
	
		public void HideAll()
		{
			USUndoManager.RegisterCompleteObjectUndo(this, "Hide All");
			for (int i = 0; i < RootItems.Count; i++)
				RootItems[i].HideAll();
		}
		
		public void ExternalModification()
		{
			for (int i = 0; i < RootItems.Count; i++)
				RootItems[i].ExternalModification();
		}
	
		public void OnSceneGUI()
		{
			for(int i = 0; i < RootItems.Count; i++)
				RootItems[i].OnSceneGUI();
		}
	}
}