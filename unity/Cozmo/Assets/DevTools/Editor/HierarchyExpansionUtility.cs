using System;
using System.Collections.Generic;
using UnityEditor;
using System.Reflection;
using UnityEngine;

public static class HierarchyExpansionUtility {

  private static Type _SceneHierarchyWindowType;

  private static Type _TreeViewType;

  private static Type _TreeViewItemType;

  private static Type _ITreeViewDataSourceType;

  private static PropertyInfo _SceneHierarchyWindow_treeView;

  private static PropertyInfo _TreeView_data;

  private static MethodInfo _ITreeViewDataSource_FindItem;

  private static MethodInfo _ITreeViewDataSource_IsExpanded;

  private static MethodInfo _ITreeViewDataSource_SetExpanded;

  private static MethodInfo _ITreeViewDataSource_ReloadData;

  private static EditorWindow _Window;

  public static EditorWindow Window {
    get {
      if (_Window == null) {
        EditorApplication.ExecuteMenuItem("Window/Hierarchy");
        _Window = EditorWindow.focusedWindow;
      }
      return _Window;
    }
  }

  static HierarchyExpansionUtility() {
    var assembly = typeof(EditorWindow).Assembly;

    _SceneHierarchyWindowType = assembly.GetType("UnityEditor.SceneHierarchyWindow");
    _TreeViewType = assembly.GetType("UnityEditor.TreeView");
    _ITreeViewDataSourceType = assembly.GetType("UnityEditor.ITreeViewDataSource");

    _SceneHierarchyWindow_treeView = _SceneHierarchyWindowType.GetProperty("treeView", BindingFlags.NonPublic | BindingFlags.Instance);

    _TreeView_data = _TreeViewType.GetProperty("data");

    _TreeViewItemType = assembly.GetType("UnityEditor.TreeViewItem");

    _ITreeViewDataSource_FindItem = _ITreeViewDataSourceType.GetMethod("FindItem");
    _ITreeViewDataSource_IsExpanded = _ITreeViewDataSourceType.GetMethod("IsExpanded", new Type[] { _TreeViewItemType });
    _ITreeViewDataSource_SetExpanded = _ITreeViewDataSourceType.GetMethod("SetExpanded", new Type[] { _TreeViewItemType, typeof(bool) });

    _ITreeViewDataSource_ReloadData = _ITreeViewDataSourceType.GetMethod("ReloadData");
  }



  public class HierarchyExpansionStatus {
    public string Name;
    public bool Expanded;

    public readonly List<HierarchyExpansionStatus> Children = new List<HierarchyExpansionStatus>();

  }

  public static bool IsExpanded(GameObject obj) {
    var treeView = _SceneHierarchyWindow_treeView.GetValue(Window, null);
    var data = _TreeView_data.GetValue(treeView, null);
    var item = _ITreeViewDataSource_FindItem.Invoke(data, new object[] { obj.GetInstanceID() });

    if (item == null) {
      _ITreeViewDataSource_ReloadData.Invoke(data, null);
    }
    item = _ITreeViewDataSource_FindItem.Invoke(data, new object[] { obj.GetInstanceID() });

    return (bool)_ITreeViewDataSource_IsExpanded.Invoke(data, new object[] { item });
  }

  public static void SetExpanded(GameObject obj, bool expanded) {
    var treeView = _SceneHierarchyWindow_treeView.GetValue(Window, null);
    var data = _TreeView_data.GetValue(treeView, null);
    var item = _ITreeViewDataSource_FindItem.Invoke(data, new object[] { obj.GetInstanceID() });

    if (item == null) {
      _ITreeViewDataSource_ReloadData.Invoke(data, null);
    }
    item = _ITreeViewDataSource_FindItem.Invoke(data, new object[] { obj.GetInstanceID() });

    _ITreeViewDataSource_SetExpanded.Invoke(data, new object[] { item, expanded });
  }

  public static HierarchyExpansionStatus GetCurrentStatusForGameObject(GameObject obj) {

    HierarchyExpansionStatus node = new HierarchyExpansionStatus() {
      Name = obj.name,
      Expanded = IsExpanded(obj)
    };
    for (int i = 0; i < obj.transform.childCount; i++) {
      node.Children.Add(GetCurrentStatusForGameObject(obj.transform.GetChild(i).gameObject));
    }
    return node;
  }

  public static void SetCurrentStatusForGameObject(GameObject obj, HierarchyExpansionStatus status) {

    // Expand children first, because they will open us up
    for (int i = 0; i < status.Children.Count; i++) {
      var child = obj.transform.FindChild(status.Children[i].Name);
      if (child != null) {
        SetCurrentStatusForGameObject(child.gameObject, status.Children[i]);
      }
    }

    SetExpanded(obj, status.Expanded);
  }

}

