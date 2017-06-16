using System;
using System.Collections.Generic;
using System.Linq;
using UnityEditor;
using UnityEditor.VersionControl;
using UnityEditorInternal;
using UnityEngine;
using Object = UnityEngine.Object;
using System.Reflection;
using System.Collections;

// Source from reflection of Unity's GameObjectInspector
/// <summary>
/// Modified for the PrefabProxy system. Allows us to listen to "Apply"
/// events so that we can save modifications to any PrefabProxy scripts
/// that the gameObject has. Also hooks up to Select.
///
/// Search for "BEGIN ANKI EDIT"
/// </summary>
namespace UnityEditor {
  [CanEditMultipleObjects, CustomEditor(typeof(GameObject))]
  internal class GameObjectInspector : Editor {
    //
    // Static Fields
    //
    private const float kToggleSize = 14;

    public static GameObject dragObject;

    private static GameObjectInspector.Styles s_styles;

    private const float kLeft = 52;

    private const float kIconSize = 24;

    private const float kTop3 = 44;

    private const float kTop2 = 24;

    private const float kTop = 4;

    //
    // Fields
    //
    private Vector2 previewDir;

    private PreviewRenderUtility m_PreviewUtility;

    private bool m_HasInstance;

    private List<GameObject> m_PreviewInstances;

    private SerializedProperty m_Icon;

    private SerializedProperty m_StaticEditorFlags;

    private SerializedProperty m_Tag;

    private SerializedProperty m_Layer;

    private SerializedProperty m_IsActive;

    private SerializedProperty m_Name;

    private bool m_AllOfSamePrefabType = true;

    #region Anki Reflected Methods

    private static readonly IDAS sDAS = DAS.GetInstance(typeof(GameObjectInspector));

    private static MethodInfo _EditorUtility_InstantiateForAnimatorPreview;

    private static PropertyInfo _Editor_referenceTargetIndex;

    private static PropertyInfo _Editor_targetTitle;

    private static PropertyInfo _EditorStyles_inspectorBig;

    private static PropertyInfo _Camera_PreviewCullingLayer;

    private static FieldInfo _HandleUtility_ignoreRaySnapObjects;

    private static MethodInfo _EditorGUI_ObjectIconDropDown;

    private static MethodInfo _EditorGUI_EnumMaskField;

    private static MethodInfo _EditorGUIUtility_TempContent;

    private static MethodInfo _EditorGUIUtility_TextContent;

    private static MethodInfo _GameObjectUtility_DisplayUpdateChildrenDialogIfNeeded;

    private static MethodInfo _Provider_PromptAndCheckoutIfNeeded;

    private static MethodInfo _PreviewGUI_Drag2D;

    private static MethodInfo _HandleUtility_CalcRayPlaceOffset;

    private enum GameObjectUtility_ShouldIncludeChildren {
      HasNoChildren = -1,
      IncludeChildren,
      DontIncludeChildren,
      Cancel
    }

    static GameObjectInspector() {
      _EditorUtility_InstantiateForAnimatorPreview = typeof(EditorUtility).GetMethod("InstantiateForAnimatorPreview", BindingFlags.NonPublic | BindingFlags.Static);
      if (_EditorUtility_InstantiateForAnimatorPreview == null) {
        sDAS.Error("Error Reflecting _EditorUtility_InstantiateForAnimatorPreview");
      }
      _EditorGUI_ObjectIconDropDown = typeof(EditorGUI).GetMethod("ObjectIconDropDown", BindingFlags.NonPublic | BindingFlags.Static);
      if (_EditorGUI_ObjectIconDropDown == null) {
        sDAS.Error("Error Reflecting _EditorGUI_ObjectIconDropDown");
      }
      _Editor_referenceTargetIndex = typeof(Editor).GetProperty("referenceTargetIndex", BindingFlags.NonPublic | BindingFlags.Instance);
      if (_Editor_referenceTargetIndex == null) {
        sDAS.Error("Error Reflecting _Editor_referenceTargetIndex");
      }
      _Editor_targetTitle = typeof(Editor).GetProperty("targetTitle", BindingFlags.NonPublic | BindingFlags.Instance);
      if (_Editor_targetTitle == null) {
        sDAS.Error("Error Reflecting _Editor_targetTitle");
      }
      _EditorStyles_inspectorBig = typeof(EditorStyles).GetProperty("inspectorBig", BindingFlags.NonPublic | BindingFlags.Static);
      if (_EditorStyles_inspectorBig == null) {
        sDAS.Error("Error Reflecting _EditorStyles_inspectorBig");
      }
      _Camera_PreviewCullingLayer = typeof(Camera).GetProperty("PreviewCullingLayer", BindingFlags.NonPublic | BindingFlags.Static);
      if (_Camera_PreviewCullingLayer == null) {
        sDAS.Error("Error Reflecting _Camera_PreviewCullingLayer");
      }
      _HandleUtility_ignoreRaySnapObjects = typeof(HandleUtility).GetField("ignoreRaySnapObjects", BindingFlags.NonPublic | BindingFlags.Static);
      if (_HandleUtility_ignoreRaySnapObjects == null) {
        sDAS.Error("Error Reflecting _HandleUtility_ignoreRaySnapObjects");
      }
      _EditorGUI_EnumMaskField = typeof(EditorGUI).GetMethod("EnumMaskField", BindingFlags.NonPublic | BindingFlags.Static, null, new Type[] {
        typeof(Rect),
        typeof(Enum),
        typeof(GUIStyle),
        typeof(int).MakeByRefType(),
        typeof(bool).MakeByRefType()
      }, null);
      if (_EditorGUI_EnumMaskField == null) {
        sDAS.Error("Error Reflecting _EditorGUI_EnumMaskField");
      }
      _EditorGUIUtility_TempContent = typeof(EditorGUIUtility).GetMethod("TempContent", BindingFlags.NonPublic | BindingFlags.Static, null, new Type[]{ typeof(string) }, null);
      if (_EditorGUIUtility_TempContent == null) {
        sDAS.Error("Error Reflecting _EditorGUIUtility_TempContent");
      }
      _EditorGUIUtility_TextContent = typeof(EditorGUIUtility).GetMethod("TextContent", BindingFlags.NonPublic | BindingFlags.Static, null, new Type[]{ typeof(string) }, null);
      if (_EditorGUIUtility_TextContent == null) {
        sDAS.Error("Error Reflecting _EditorGUIUtility_TextContent");
      }
      _GameObjectUtility_DisplayUpdateChildrenDialogIfNeeded = typeof(GameObjectUtility).GetMethod("DisplayUpdateChildrenDialogIfNeeded", BindingFlags.NonPublic | BindingFlags.Static);
      if (_GameObjectUtility_DisplayUpdateChildrenDialogIfNeeded == null) {
        sDAS.Error("Error Reflecting _GameObjectUtility_DisplayUpdateChildrenDialogIfNeeded");
      }
      _Provider_PromptAndCheckoutIfNeeded = typeof(Provider).GetMethod("PromptAndCheckoutIfNeeded", BindingFlags.NonPublic | BindingFlags.Static);
      if (_Provider_PromptAndCheckoutIfNeeded == null) {
        sDAS.Error("Error Reflecting _Provider_PromptAndCheckoutIfNeeded");
      }

      _PreviewGUI_Drag2D = Assembly.GetAssembly(typeof(Editor)).GetType("PreviewGUI").GetMethod("Drag2D", BindingFlags.Public | BindingFlags.Static);
      if (_PreviewGUI_Drag2D == null) {
        sDAS.Error("Error Reflecting _PreviewGUI_Drag2D");
      }
      _HandleUtility_CalcRayPlaceOffset = typeof(HandleUtility).GetMethod("CalcRayPlaceOffset", BindingFlags.NonPublic | BindingFlags.Static);
      if (_HandleUtility_CalcRayPlaceOffset == null) {
        sDAS.Error("Error Reflecting _HandleUtility_CalcRayPlaceOffset");
      }
    }

    private static GameObject EditorUtility_InstantiateForAnimatorPreview(Object obj) {
      return (GameObject)_EditorUtility_InstantiateForAnimatorPreview.Invoke(null, new[] { obj });
    }

    private static void EditorGUI_ObjectIconDropDown(Rect position, Object[] targets, bool showLabelIcons, Texture2D nullIcon, SerializedProperty iconProperty) {
      _EditorGUI_ObjectIconDropDown.Invoke(null, new object[] {
        position,
        targets,
        showLabelIcons,
        nullIcon,
        iconProperty
      });
    }

    private static string EditorGUI_DelayedTextField(Rect position, string value, string allowedLetters, GUIStyle style) {
      
      return (string)EditorGUI.DelayedTextField(position, value, allowedLetters, style);
    }

    private static Enum EditorGUI_EnumMaskField(Rect position, Enum enumValue, GUIStyle style, out int changedFlags, out bool changedToValue) {
      var args = new object[] { position, enumValue, style, 0, false };
      var returnVal = (Enum)_EditorGUI_EnumMaskField.Invoke(null, args);
      changedFlags = (int)args[3];
      changedToValue = (bool)args[4];
      return returnVal;
    }

    private static GUIContent EditorGUIUtility_TempContent(string t) {
      return (GUIContent)_EditorGUIUtility_TempContent.Invoke(null, new object[]{ t });
    }

    private static GUIContent EditorGUIUtility_TextContent(string t) {
      return (GUIContent)_EditorGUIUtility_TextContent.Invoke(null, new object[]{ t });
    }

    private static GameObjectUtility_ShouldIncludeChildren GameObjectUtility_DisplayUpdateChildrenDialogIfNeeded(IEnumerable<GameObject> gameObjects, string title, string message) {
      return (GameObjectUtility_ShouldIncludeChildren)_GameObjectUtility_DisplayUpdateChildrenDialogIfNeeded.Invoke(null, new object[] {
        gameObjects,
        title,
        message
      });
    }

    private static bool Provider_PromptAndCheckoutIfNeeded(string[] assets, string promptIfCheckoutIsNeeded) {
      return (bool)_Provider_PromptAndCheckoutIfNeeded.Invoke(null, new object[] { assets, promptIfCheckoutIsNeeded });
    }

    private static Vector2 PreviewGUI_Drag2D(Vector2 scrollPosition, Rect position) {
      return (Vector2)_PreviewGUI_Drag2D.Invoke(null, new object[]{ scrollPosition, position });
    }

    private static float HandleUtility_CalcRayPlaceOffset(Transform[] objects, Vector3 normal) {
      return (float)_HandleUtility_CalcRayPlaceOffset.Invoke(null, new object[] { objects, normal });
    }

    public int referenceTargetIndex { 
      get {
        return (int)_Editor_referenceTargetIndex.GetValue(this, null);
      }
      set {
        _Editor_referenceTargetIndex.SetValue(this, value, null);
      }
    }

    public string targetTitle { 
      get {
        return (string)_Editor_targetTitle.GetValue(this, null);
      }
      set {
        _Editor_targetTitle.SetValue(this, value, null);
      }
    }

    public static GUIStyle EditorStyles_inspectorBig {
      get {
        return (GUIStyle)_EditorStyles_inspectorBig.GetValue(null, null);
      }
    }

    public static int Camera_PreviewCullingLayer {
      get {
        return (int)_Camera_PreviewCullingLayer.GetValue(null, null);
      }
    }

    public static Transform[] HandleUtility_ignoreRaySnapObjects {
      get {
        return (Transform[])_HandleUtility_ignoreRaySnapObjects.GetValue(null);
      }
      set {
        _HandleUtility_ignoreRaySnapObjects.SetValue(null, value);
      }
    }

    #endregion

    #region Anki Prefab Proxy Methods

    private class LocalPrefabReferenceMapping {
      public string FieldName;
      public Transform SourceTransform;

      public GameObject SourcePrefab;
           
      public Transform DestinationTransform;

      public GameObject DestinationPrefab;
    }

    private class GlobalPrefabReferenceMapping {
      public string FieldName;
      public Transform SourceTransform;

      public GameObject SourcePrefab;

      public Object Destination;
    }

    private class PrefabValueMapping {
      public string FieldName;
      public Transform SourceTransform;

      public GameObject SourcePrefab;

      public object Value;
    }

    private bool IsChild(Transform parent, Transform child) {
      var t = child;
      while (t != null) {
        if (t == parent) {
          return true;
        }
        t = t.parent;
      }
      return false;
    }

    private void GenerateLocalReferences(PrefabProxy proxy, List<LocalPrefabReferenceMapping> referenceMappings) {
      proxy.ModifiedLocalReferences.Clear();

      for (int i = referenceMappings.Count - 1; i >= 0; i--) {
        var refMap = referenceMappings[i];
        if (IsChild(proxy.transform, refMap.SourceTransform) || IsChild(proxy.transform, refMap.DestinationTransform)) {

          proxy.ModifiedLocalReferences.Add(new PrefabProxy.ModifiedLocalReferenceField() {
            ObjectPath = RelativePath(proxy.transform, refMap.SourceTransform),
            FieldName = refMap.FieldName,
            ReferencePath = RelativePath(proxy.transform, refMap.DestinationTransform)
          });

          referenceMappings.RemoveAt(i);
        }
      }
    }

    private void GenerateGlobalReferences(PrefabProxy proxy, List<GlobalPrefabReferenceMapping> referenceMappings) {
      proxy.ModifiedGlobalReferences.Clear();

      for (int i = referenceMappings.Count - 1; i >= 0; i--) {
        var refMap = referenceMappings[i];
        if (IsChild(proxy.transform, refMap.SourceTransform)) {

          proxy.ModifiedGlobalReferences.Add(new PrefabProxy.ModifiedGlobalReferenceField() {
            ObjectPath = RelativePath(proxy.transform, refMap.SourceTransform),
            FieldName = refMap.FieldName,
            Reference = refMap.Destination
          });

          referenceMappings.RemoveAt(i);
        }
      }
    }

    private void GenerateValues(PrefabProxy proxy, List<PrefabValueMapping> valueMappings) {
      proxy.ModifiedValues.Clear();

      for (int i = valueMappings.Count - 1; i >= 0; i--) {
        var refMap = valueMappings[i];
        if (IsChild(proxy.transform, refMap.SourceTransform)) {

          var entry = new PrefabProxy.ModifiedValueField() {
            ObjectPath = RelativePath(proxy.transform, refMap.SourceTransform),
            FieldName = refMap.FieldName
          };
          entry.SetObjectValue(refMap.Value);

          proxy.ModifiedValues.Add(entry);

          valueMappings.RemoveAt(i);
        }
      }
    }

    private string FullPath(Transform a) {
      string path = "";
      while (a != null) {
        if (a.parent == null || a.parent.GetComponent<PrefabProxy>() == null) {
          path = "/" + a.name + path;
        }
        a = a.parent;
      }
      return path;
    }

    private string RelativePath(Transform a, Transform b) {
      string fulla = FullPath(a);
      string fullb = FullPath(b);

      var splitFullB = fullb.Split('/');
      var splitFullA = fulla.TrimEnd('/').Split('/');

      var len = Mathf.Min(splitFullB.Length, splitFullA.Length);
      int indexOfFirstDifferent = 0;
      for (; 
        indexOfFirstDifferent < len &&
      splitFullB[indexOfFirstDifferent] == splitFullA[indexOfFirstDifferent]; 
        indexOfFirstDifferent++)
        ;

      var copy = new string[splitFullB.Length - indexOfFirstDifferent];
      System.Array.Copy(splitFullB, indexOfFirstDifferent, copy, 0, copy.Length);
      var rightPath = string.Join("/", copy);
      var leftFolderCount = splitFullA.Length - indexOfFirstDifferent;

      for (int i = 0; i < leftFolderCount; i++) {
        rightPath = "../" + rightPath;
      }

      return rightPath;
    }

    private Transform GetTransformAtFullPath(string fullPath) {
      string[] path = fullPath.Split('/');
      var node = GameObject.Find(path[1]).transform;

      for (int i = 2; i < path.Length; i++) {
        node = node.FindChild(path[i]);
        while (node.GetComponent<PrefabProxy>()) {
          node = node.GetChild(0);
        }
      }
      return node;
    }

    private void GenerateReferenceMappings(GameObject gameObject, List<LocalPrefabReferenceMapping> localReferences, List<GlobalPrefabReferenceMapping> globalReferences, List<PrefabValueMapping> values) {
      GenerateReferenceMappingsRecursive(gameObject.transform, gameObject.transform, null, null, localReferences, globalReferences, values);
    }

    public static object GetDefault(Type type) {
      if (type.IsValueType) {
        return Activator.CreateInstance(type);
      }
      return null;
    }

    private static readonly Dictionary<Type, FieldInfo[]> _FieldInfoCache = new Dictionary<Type, FieldInfo[]>();

    private Transform GetPrefabProxyTransform(Transform node) {
      var t = node;
      while (t != null) {
        if (t.GetComponent<PrefabProxy>()) {
          return t;
        }
        t = t.parent;
      }
      return null;
    }

    private bool GenerateReferenceMappingsRecursiveForField(object obj, object prefabObj, Type type, string typeString, Transform root, Transform node, GameObject prefab, Transform prefabNode, List<LocalPrefabReferenceMapping> localReferences, List<GlobalPrefabReferenceMapping> globalReferences, List<PrefabValueMapping> values, int depth) {

      if (depth > 40) {
        throw new Exception("Hit depth 40 " + typeString);
      }
      // Renames sometimes cause null children
      if (obj == null) {
        return false;
      }

      var fields = GetFields(type);

      if (fields.Length == 0) {
        return false;
      }

      foreach (var field in fields) {

        var newTypeString = typeString + ":" + field.Name;


        if (typeof(Object).IsAssignableFrom(field.FieldType)) {
          Transform objectTransform = null;
          Transform prefabObjectTransform = null;
          var val = (Object)field.GetValue(obj);
          var prefabVal = prefabObj != null ? (Object)field.GetValue(prefabObj) : null;

          if (field.FieldType == typeof(GameObject)) {
            var go = (GameObject)val;

            if (go != null) {
              objectTransform = go.transform;
            }

            go = (GameObject)prefabVal;
            if (go != null) {
              prefabObjectTransform = go.transform;
            }
          }
          else if (typeof(Component).IsAssignableFrom(field.FieldType)) {
            var component = (Component)val;
            if (component != null) {
              objectTransform = component.transform;
            }

            component = (Component)prefabVal;
            if (component != null) {
              prefabObjectTransform = component.transform;
            }
          }

          GameObject DestinationPrefabProxyPrefab = null;
          bool isLocal = false;
          bool changed = false;

          if (objectTransform != null && IsChild(root, objectTransform)) {
            var prefabProxy = GetPrefabProxyTransform(node);
            var fieldPrefabProxy = GetPrefabProxyTransform(objectTransform);

            if (prefabProxy != null || fieldPrefabProxy != null) {
              isLocal = true;
              if (fieldPrefabProxy != null) {
                DestinationPrefabProxyPrefab = fieldPrefabProxy.GetComponent<PrefabProxy>().Prefab;
              }

              // two different prefab proxies
              if (prefabProxy != fieldPrefabProxy ||
              // a null field in the original
                  prefabObjectTransform == null ||
              // the same prefab proxy but a modified field
                  (prefabObjectTransform != null &&
                  RelativePath(node, objectTransform) != RelativePath(prefabNode, prefabObjectTransform))) {
                changed = true;
              }
            }
          }

          if (isLocal) {
            if (changed) {
              localReferences.Add(new LocalPrefabReferenceMapping() {
                FieldName = newTypeString,
                SourceTransform = node,
                SourcePrefab = prefab,
                DestinationPrefab = DestinationPrefabProxyPrefab,
                DestinationTransform = objectTransform
              });
            }
          }
          else {
            if (prefab != null && val != prefabVal) {
              globalReferences.Add(new GlobalPrefabReferenceMapping() {
                SourceTransform = node,
                SourcePrefab = prefab,
                FieldName = newTypeString,
                Destination = (Object)val
              });
            }
          }

          if (prefab != null) {
            // clear out the object reference so it doesn't get sucked up by json serializer
            field.SetValue(obj, null);
          }
          continue;
        }
        else if (typeof(IList).IsAssignableFrom(field.FieldType)) {
          Type elementType;
          if (field.FieldType.IsArray) {
            elementType = field.FieldType.GetElementType();
          }
          else {
            elementType = field.FieldType.GetGenericArguments().FirstOrDefault() ?? typeof(object);
          }

          var childList = (IList)field.GetValue(obj);
          var childPrefabList = (IList)(prefabObj != null ? field.GetValue(prefabObj) : null);

          if (childList != null) {

            for (int i = 0; i < childList.Count; i++) {           
              GenerateReferenceMappingsRecursiveForField(
                childList[i], 
                childPrefabList != null && i < childPrefabList.Count ? childPrefabList[i] : null, 
                elementType, 
                newTypeString + ":" + i, 
                root,
                node,
                prefab,
                prefabNode,
                localReferences,
                globalReferences,
                values,
                depth + 1);
            }

            if (((childList == null ^ childPrefabList == null) ||
                (childList != null && childPrefabList != null &&
                childList.Count != childPrefabList.Count)) && prefab != null) {
              values.Add(new PrefabValueMapping() {
                FieldName = newTypeString,
                SourcePrefab = prefab,
                SourceTransform = node,
                Value = childList
              });
            }
          }

        }
        else if (field.FieldType.IsValueType || field.FieldType == typeof(string) || field.FieldType.GetCustomAttributes(typeof(System.SerializableAttribute), true).Length > 0) {

          var childObj = field.GetValue(obj);
          var childPrefabObj = prefabObj != null ? field.GetValue(prefabObj) : null;

          bool childFields = 
            GenerateReferenceMappingsRecursiveForField(
              childObj, 
              childPrefabObj, 
              field.FieldType, 
              newTypeString, 
              root,
              node,
              prefab,
              prefabNode,
              localReferences,
              globalReferences,
              values,
              depth + 1);          

          if (!childFields) {

            if (!EqualityComparer<object>.Default.Equals(childObj, childPrefabObj) && prefab != null) {
              values.Add(new PrefabValueMapping() {
                FieldName = newTypeString,
                SourcePrefab = prefab,
                SourceTransform = node,
                Value = childObj
              });
            }
          }
        }
      }
      return true;
    }

    private FieldInfo[] GetFields(Type type) {
      FieldInfo[] fields;
      if (!_FieldInfoCache.TryGetValue(type, out fields)) {
        fields = type.GetFields(BindingFlags.Public | BindingFlags.Instance)
          .Where(f => f.GetCustomAttributes(typeof(NonSerializedAttribute), true).Length == 0).ToArray();
        
        var subType = type;
        // look for inherited serialized fields
        while (subType != null && subType != typeof(System.Object) && subType != typeof(System.Enum)) {
          var privateFields = subType.GetFields(BindingFlags.NonPublic | BindingFlags.Instance)
            .Where(f => f.GetCustomAttributes(typeof(SerializeField), true).Length > 0);

          privateFields = privateFields.Where(x => !fields.Any(y => y.Name == x.Name));

          fields = fields.Concat(privateFields).ToArray();
          subType = subType.BaseType;
        }
        _FieldInfoCache[type] = fields;
      }
      return fields;
    }

    private void GenerateReferenceMappingsRecursive(Transform root, Transform node, GameObject prefab, Transform prefabNode, List<LocalPrefabReferenceMapping> localReferences, List<GlobalPrefabReferenceMapping> globalReferences, List<PrefabValueMapping> values) {
      var components = node.GetComponents<Component>();

      GameObject nextPrefab = prefab;
      Transform nextPrefabNode = null;
      foreach (var component in components) {
        var type = component.GetType();

        // we don't want to check the properties of the prefab proxy
        if (type == typeof(PrefabProxy)) {
          nextPrefab = ((PrefabProxy)component).Prefab;
          nextPrefabNode = nextPrefab.transform;
          continue;
        }

        Component prefabComponent = null;
        if (prefabNode != null) {
          prefabComponent = prefabNode.GetComponent(type);

          if (prefabComponent == null) {
            values.Add(new PrefabValueMapping() {
              FieldName = type.FullName,
              SourceTransform = node,
              SourcePrefab = prefab,
              Value = null
            });
          }
        }

        GenerateReferenceMappingsRecursiveForField(
          component, 
          prefabComponent, 
          type, 
          type.FullName, 
          root, 
          node, 
          prefab, 
          prefabNode, 
          localReferences,
          globalReferences, 
          values,
          1);
      }

      for (int i = 0; i < node.childCount; i++) {
        var child = node.GetChild(i);
        GenerateReferenceMappingsRecursive(root, 
          node.GetChild(i),
          nextPrefab, 
          nextPrefabNode != null ? nextPrefabNode : (prefabNode != null ? prefabNode.FindChild(child.name) : null), 
          localReferences, 
          globalReferences, 
          values);
      }
    }

    private void CleanupPrefabProxiesForSerialization(GameObject gameObject) {
      var prefabProxies = gameObject.GetComponentsInChildren<PrefabProxy>(true);

      if (prefabProxies.Length == 0)
        return;

      List<LocalPrefabReferenceMapping> localReferences = new List<LocalPrefabReferenceMapping>();
      List<GlobalPrefabReferenceMapping> globalReferences = new List<GlobalPrefabReferenceMapping>();
      List<PrefabValueMapping> values = new List<PrefabValueMapping>();

      GenerateReferenceMappings(gameObject, localReferences, globalReferences, values);

      foreach (var prefabProxy in prefabProxies) {
        GenerateLocalReferences(prefabProxy, localReferences);
        GenerateGlobalReferences(prefabProxy, globalReferences);
        GenerateValues(prefabProxy, values);
      }
      foreach (var prefabProxy in prefabProxies) {          
        for (int i = prefabProxy.transform.childCount - 1; i >= 0; i--) {
          GameObject.DestroyImmediate(prefabProxy.transform.GetChild(i).gameObject);
        }
      }
    }

    private void RestorePrefabProxies(GameObject gameObject) {
      DAS.ClearTargets();
      DAS.AddTarget(new UnityDasTarget());
      var prefabProxies = gameObject.GetComponentsInChildren<PrefabProxy>(true);
      foreach (var prefabProxy in prefabProxies) {
        prefabProxy.Expand();
      }

    }



    #endregion

    //
    // Constructors
    //

    //
    // Static Methods
    //
    public static void GetRenderableBoundsRecurse(ref Bounds bounds, GameObject go) {
      MeshRenderer meshRenderer = go.GetComponent(typeof(MeshRenderer)) as MeshRenderer;
      MeshFilter meshFilter = go.GetComponent(typeof(MeshFilter)) as MeshFilter;
      if (meshRenderer && meshFilter && meshFilter.sharedMesh) {
        if (bounds.extents == Vector3.zero) {
          bounds = meshRenderer.bounds;
        }
        else {
          bounds.Encapsulate(meshRenderer.bounds);
        }
      }
      SkinnedMeshRenderer skinnedMeshRenderer = go.GetComponent(typeof(SkinnedMeshRenderer)) as SkinnedMeshRenderer;
      if (skinnedMeshRenderer && skinnedMeshRenderer.sharedMesh) {
        if (bounds.extents == Vector3.zero) {
          bounds = skinnedMeshRenderer.bounds;
        }
        else {
          bounds.Encapsulate(skinnedMeshRenderer.bounds);
        }
      }
      SpriteRenderer spriteRenderer = go.GetComponent(typeof(SpriteRenderer)) as SpriteRenderer;
      if (spriteRenderer && spriteRenderer.sprite) {
        if (bounds.extents == Vector3.zero) {
          bounds = spriteRenderer.bounds;
        }
        else {
          bounds.Encapsulate(spriteRenderer.bounds);
        }
      }
      foreach (Transform transform in go.transform) {
        GameObjectInspector.GetRenderableBoundsRecurse(ref bounds, transform.gameObject);
      }
    }

    public static Vector3 GetRenderableCenterRecurse(GameObject go, int minDepth, int maxDepth) {
      Vector3 vector = Vector3.zero;
      float renderableCenterRecurse = GameObjectInspector.GetRenderableCenterRecurse(ref vector, go, 0, minDepth, maxDepth);
      if (renderableCenterRecurse > 0) {
        vector /= renderableCenterRecurse;
      }
      else {
        vector = go.transform.position;
      }
      return vector;
    }

    private static float GetRenderableCenterRecurse(ref Vector3 center, GameObject go, int depth, int minDepth, int maxDepth) {
      if (depth > maxDepth) {
        return 0;
      }
      float num = 0;
      if (depth > minDepth) {
        MeshRenderer meshRenderer = go.GetComponent(typeof(MeshRenderer)) as MeshRenderer;
        MeshFilter x = go.GetComponent(typeof(MeshFilter)) as MeshFilter;
        SkinnedMeshRenderer skinnedMeshRenderer = go.GetComponent(typeof(SkinnedMeshRenderer)) as SkinnedMeshRenderer;
        SpriteRenderer spriteRenderer = go.GetComponent(typeof(SpriteRenderer)) as SpriteRenderer;
        if (meshRenderer == null && x == null && skinnedMeshRenderer == null && spriteRenderer == null) {
          num = 1;
          center += go.transform.position;
        }
        else if (meshRenderer != null && x != null) {
          if (Vector3.Distance(meshRenderer.bounds.center, go.transform.position) < 0.01f) {
            num = 1;
            center += go.transform.position;
          }
        }
        else if (skinnedMeshRenderer != null) {
          if (Vector3.Distance(skinnedMeshRenderer.bounds.center, go.transform.position) < 0.01f) {
            num = 1;
            center += go.transform.position;
          }
        }
        else if (spriteRenderer != null && Vector3.Distance(spriteRenderer.bounds.center, go.transform.position) < 0.01f) {
          num = 1;
          center += go.transform.position;
        }
      }
      depth++;
      foreach (Transform transform in go.transform) {
        num += GameObjectInspector.GetRenderableCenterRecurse(ref center, transform.gameObject, depth, minDepth, maxDepth);
      }
      return num;
    }

    public static bool HasRenderablePartsRecurse(GameObject go) {
      MeshRenderer exists = go.GetComponent(typeof(MeshRenderer)) as MeshRenderer;
      MeshFilter meshFilter = go.GetComponent(typeof(MeshFilter)) as MeshFilter;
      if (exists && meshFilter && meshFilter.sharedMesh) {
        return true;
      }
      SkinnedMeshRenderer skinnedMeshRenderer = go.GetComponent(typeof(SkinnedMeshRenderer)) as SkinnedMeshRenderer;
      if (skinnedMeshRenderer && skinnedMeshRenderer.sharedMesh) {
        return true;
      }
      SpriteRenderer spriteRenderer = go.GetComponent(typeof(SpriteRenderer)) as SpriteRenderer;
      if (spriteRenderer && spriteRenderer.sprite) {
        return true;
      }
      foreach (Transform transform in go.transform) {
        if (GameObjectInspector.HasRenderablePartsRecurse(transform.gameObject)) {
          return true;
        }
      }
      return false;
    }

    public static void SetEnabledRecursive(GameObject go, bool enabled) {
      Renderer[] componentsInChildren = go.GetComponentsInChildren<Renderer>();
      for (int i = 0; i < componentsInChildren.Length; i++) {
        Renderer renderer = componentsInChildren[i];
        renderer.enabled = enabled;
      }
    }

    private static bool ShowMixedStaticEditorFlags(StaticEditorFlags mask) {
      uint num = 0;
      uint num2 = 0;
      foreach (object current in Enum.GetValues(typeof(StaticEditorFlags))) {
        num2 += 1;
        if ((mask & (StaticEditorFlags)((int)current)) > (StaticEditorFlags)0) {
          num += 1;
        }
      }
      return num > 0 && num != num2;
    }

    //
    // Methods
    //
    private void CalculatePrefabStatus() {
      this.m_HasInstance = false;
      this.m_AllOfSamePrefabType = true;
      PrefabType prefabType = PrefabUtility.GetPrefabType(base.targets[0] as GameObject);
      Object[] targets = base.targets;
      for (int i = 0; i < targets.Length; i++) {
        GameObject target = (GameObject)targets[i];
        PrefabType prefabType2 = PrefabUtility.GetPrefabType(target);
        if (prefabType2 != prefabType) {
          this.m_AllOfSamePrefabType = false;
        }
        if (prefabType2 != PrefabType.None && prefabType2 != PrefabType.Prefab && prefabType2 != PrefabType.ModelPrefab) {
          this.m_HasInstance = true;
        }
      }
    }

    private void CreatePreviewInstances() {
      this.DestroyPreviewInstances();
      if (this.m_PreviewInstances == null) {
        this.m_PreviewInstances = new List<GameObject>(base.targets.Length);
      }
      for (int i = 0; i < base.targets.Length; i++) {
        GameObject gameObject = EditorUtility_InstantiateForAnimatorPreview(base.targets[i]);
        GameObjectInspector.SetEnabledRecursive(gameObject, false);
        this.m_PreviewInstances.Add(gameObject);
      }
    }

    private void DestroyPreviewInstances() {
      if (this.m_PreviewInstances == null || this.m_PreviewInstances.Count == 0) {
        return;
      }
      foreach (GameObject current in this.m_PreviewInstances) {
        Object.DestroyImmediate(current);
      }
      this.m_PreviewInstances.Clear();
    }

    private void DoRenderPreview() {
      GameObject gameObject = this.m_PreviewInstances[this.referenceTargetIndex];
      Bounds bounds = new Bounds(gameObject.transform.position, Vector3.zero);
      GameObjectInspector.GetRenderableBoundsRecurse(ref bounds, gameObject);
      float num = Mathf.Max(bounds.extents.magnitude, 0.0001f);
      float num2 = num * 3.8f;
      Quaternion quaternion = Quaternion.Euler(-this.previewDir.y, -this.previewDir.x, 0);
      Vector3 position = bounds.center - quaternion * (Vector3.forward * num2);
      this.m_PreviewUtility.m_Camera.transform.position = position;
      this.m_PreviewUtility.m_Camera.transform.rotation = quaternion;
      this.m_PreviewUtility.m_Camera.nearClipPlane = num2 - num * 1.1f;
      this.m_PreviewUtility.m_Camera.farClipPlane = num2 + num * 1.1f;
      this.m_PreviewUtility.m_Light[0].intensity = 0.7f;
      this.m_PreviewUtility.m_Light[0].transform.rotation = quaternion * Quaternion.Euler(40, 40, 0);
      this.m_PreviewUtility.m_Light[1].intensity = 0.7f;
      this.m_PreviewUtility.m_Light[1].transform.rotation = quaternion * Quaternion.Euler(340, 218, 177);
      Color ambient = new Color(0.1f, 0.1f, 0.1f, 0);
      InternalEditorUtility.SetCustomLighting(this.m_PreviewUtility.m_Light, ambient);
      bool fog = RenderSettings.fog;
      Unsupported.SetRenderSettingsUseFogNoDirty(false);
      GameObjectInspector.SetEnabledRecursive(gameObject, true);
      this.m_PreviewUtility.m_Camera.Render();
      GameObjectInspector.SetEnabledRecursive(gameObject, false);
      Unsupported.SetRenderSettingsUseFogNoDirty(fog);
      InternalEditorUtility.RemoveCustomLighting();
    }

    internal bool DrawInspector(Rect contentRect) {
      if (GameObjectInspector.s_styles == null) {
        GameObjectInspector.s_styles = new GameObjectInspector.Styles();
      }
      base.serializedObject.Update();
      GameObject gameObject = this.target as GameObject;
      EditorGUIUtility.labelWidth = 52;
      bool enabled = GUI.enabled;
      GUI.enabled = true;
      GUI.Label(new Rect(contentRect.x, contentRect.y, contentRect.width, contentRect.height + 3), GUIContent.none, EditorStyles_inspectorBig);
      GUI.enabled = enabled;
      float width = contentRect.width;
      float y = contentRect.y;
      GUIContent gUIContent = null;
      PrefabType prefabType = PrefabType.None;
      if (this.m_AllOfSamePrefabType) {
        prefabType = PrefabUtility.GetPrefabType(gameObject);
        switch (prefabType) {
        case PrefabType.None:
          gUIContent = GameObjectInspector.s_styles.goIcon;
          break;
        case PrefabType.Prefab:
        case PrefabType.PrefabInstance:
        case PrefabType.DisconnectedPrefabInstance:
          gUIContent = GameObjectInspector.s_styles.prefabIcon;
          break;
        case PrefabType.ModelPrefab:
        case PrefabType.ModelPrefabInstance:
        case PrefabType.DisconnectedModelPrefabInstance:
          gUIContent = GameObjectInspector.s_styles.modelIcon;
          break;
        case PrefabType.MissingPrefabInstance:
          gUIContent = GameObjectInspector.s_styles.prefabIcon;
          break;
        }
      }
      else {
        gUIContent = GameObjectInspector.s_styles.typelessIcon;
      }
      EditorGUI_ObjectIconDropDown(new Rect(3, 4 + y, 24, 24), base.targets, true, gUIContent.image as Texture2D, this.m_Icon);
      EditorGUI.BeginDisabledGroup(prefabType == PrefabType.ModelPrefab);
      EditorGUI.PropertyField(new Rect(34, 4 + y, 14, 14), this.m_IsActive, GUIContent.none);
      float num = GameObjectInspector.s_styles.staticFieldToggleWidth + 15;
      float width2 = width - 52 - num - 5;
      EditorGUI.BeginChangeCheck();
      EditorGUI.showMixedValue = this.m_Name.hasMultipleDifferentValues;
      string name = EditorGUI_DelayedTextField(new Rect(52, 4 + y + 1, width2, 16), gameObject.name, null, EditorStyles.textField);
      EditorGUI.showMixedValue = false;
      if (EditorGUI.EndChangeCheck()) {
        Object[] targets = base.targets;
        for (int i = 0; i < targets.Length; i++) {
          Object @object = targets[i];
          ObjectNames.SetNameSmart(@object as GameObject, name);
        }
      }
      Rect rect = new Rect(width - num, 4 + y, GameObjectInspector.s_styles.staticFieldToggleWidth, 16);
      EditorGUI.BeginProperty(rect, GUIContent.none, this.m_StaticEditorFlags);
      EditorGUI.BeginChangeCheck();
      Rect position = rect;
      EditorGUI.showMixedValue |= GameObjectInspector.ShowMixedStaticEditorFlags((StaticEditorFlags)this.m_StaticEditorFlags.intValue);
      Event current = Event.current;
      EventType type = current.type;
      bool flag = current.type == EventType.MouseDown && current.button != 0;
      if (flag) {
        current.type = EventType.Ignore;
      }
      bool flagValue = EditorGUI.ToggleLeft(position, "Static", gameObject.isStatic);
      if (flag) {
        current.type = type;
      }
      EditorGUI.showMixedValue = false;
      if (EditorGUI.EndChangeCheck()) {
        SceneModeUtility.SetStaticFlags(base.targets, -1, flagValue);
        base.serializedObject.SetIsDifferentCacheDirty();
      }
      EditorGUI.EndProperty();
      EditorGUI.BeginChangeCheck();
      EditorGUI.showMixedValue = this.m_StaticEditorFlags.hasMultipleDifferentValues;
      int changedFlags;
      bool flagValue2;
      EditorGUI_EnumMaskField(new Rect(rect.x + GameObjectInspector.s_styles.staticFieldToggleWidth, rect.y, 10, 14), GameObjectUtility.GetStaticEditorFlags(gameObject), GameObjectInspector.s_styles.staticDropdown, out changedFlags, out flagValue2);
      EditorGUI.showMixedValue = false;
      if (EditorGUI.EndChangeCheck()) {
        SceneModeUtility.SetStaticFlags(base.targets, changedFlags, flagValue2);
        base.serializedObject.SetIsDifferentCacheDirty();
      }
      float num2 = 4;
      float num3 = 4;
      EditorGUIUtility.fieldWidth = (width - num2 - 52 - GameObjectInspector.s_styles.layerFieldWidth - num3) / 2;
      string tag = null;
      try {
        tag = gameObject.tag;
      }
      catch (Exception) {
        tag = "Undefined";
      }
      EditorGUIUtility.labelWidth = GameObjectInspector.s_styles.tagFieldWidth;
      Rect rect2 = new Rect(52 - EditorGUIUtility.labelWidth, 24 + y, EditorGUIUtility.labelWidth + EditorGUIUtility.fieldWidth, 16);
      EditorGUI.BeginProperty(rect2, GUIContent.none, this.m_Tag);
      EditorGUI.BeginChangeCheck();
      string text = EditorGUI.TagField(rect2, EditorGUIUtility_TempContent("Tag"), tag);
      if (EditorGUI.EndChangeCheck()) {
        this.m_Tag.stringValue = text;
        Undo.RecordObjects(base.targets, "Change Tag of " + this.targetTitle);
        Object[] targets2 = base.targets;
        for (int j = 0; j < targets2.Length; j++) {
          Object object2 = targets2[j];
          (object2 as GameObject).tag = text;
        }
      }
      EditorGUI.EndProperty();
      EditorGUIUtility.labelWidth = GameObjectInspector.s_styles.layerFieldWidth;
      rect2 = new Rect(52 + EditorGUIUtility.fieldWidth + num2, 24 + y, EditorGUIUtility.labelWidth + EditorGUIUtility.fieldWidth, 16);
      EditorGUI.BeginProperty(rect2, GUIContent.none, this.m_Layer);
      EditorGUI.BeginChangeCheck();
      int num4 = EditorGUI.LayerField(rect2, EditorGUIUtility_TempContent("Layer"), gameObject.layer);
      if (EditorGUI.EndChangeCheck()) {
        GameObjectUtility_ShouldIncludeChildren shouldIncludeChildren = GameObjectUtility_DisplayUpdateChildrenDialogIfNeeded(base.targets.OfType<GameObject>(), "Change Layer", "Do you want to set layer to " + InternalEditorUtility.GetLayerName(num4) + " for all child objects as well?");
        if (shouldIncludeChildren != GameObjectUtility_ShouldIncludeChildren.Cancel) {
          this.m_Layer.intValue = num4;
          this.SetLayer(num4, shouldIncludeChildren == GameObjectUtility_ShouldIncludeChildren.IncludeChildren);
        }
      }
      EditorGUI.EndProperty();
      if (this.m_HasInstance && !EditorApplication.isPlayingOrWillChangePlaymode) {
        float num5 = (width - 52 - 5) / 3;
        Rect position2 = new Rect(52 + num5 * 0, 44 + y, num5, 15);
        Rect position3 = new Rect(52 + num5 * 1, 44 + y, num5, 15);
        Rect position4 = new Rect(52 + num5 * 2, 44 + y, num5, 15);
        Rect position5 = new Rect(52, 44 + y, num5 * 3, 15);
        GUIContent gUIContent2 = (base.targets.Length <= 1) ? GameObjectInspector.s_styles.goTypeLabel[(int)prefabType] : GameObjectInspector.s_styles.goTypeLabelMultiple;
        if (gUIContent2 != null) {
          float x = GUI.skin.label.CalcSize(gUIContent2).x;
          if (prefabType == PrefabType.DisconnectedModelPrefabInstance || prefabType == PrefabType.MissingPrefabInstance || prefabType == PrefabType.DisconnectedPrefabInstance) {
            GUI.contentColor = GUI.skin.GetStyle("CN StatusWarn").normal.textColor;
            if (prefabType == PrefabType.MissingPrefabInstance) {
              GUI.Label(new Rect(52, 44 + y, width - 52 - 5, 18), gUIContent2, EditorStyles.whiteLabel);
            }
            else {
              GUI.Label(new Rect(52 - x - 5, 44 + y, width - 52 - 5, 18), gUIContent2, EditorStyles.whiteLabel);
            }
            GUI.contentColor = Color.white;
          }
          else {
            Rect position6 = new Rect(52 - x - 5, 44 + y, x, 18);
            GUI.Label(position6, gUIContent2);
          }
        }
        if (base.targets.Length > 1) {
          GUI.Label(position5, "Instance Management Disabled", GameObjectInspector.s_styles.instanceManagementInfo);
        }
        else {
          if (prefabType != PrefabType.MissingPrefabInstance && GUI.Button(position2, "Select", "MiniButtonLeft")) {
            Selection.activeObject = PrefabUtility.GetPrefabParent(this.target);
            EditorGUIUtility.PingObject(Selection.activeObject);
          }
          if ((prefabType == PrefabType.DisconnectedModelPrefabInstance || prefabType == PrefabType.DisconnectedPrefabInstance) && GUI.Button(position3, "Revert", "MiniButtonMid")) {
            Undo.RegisterFullObjectHierarchyUndo(gameObject, "Revert to prefab");
            PrefabUtility.ReconnectToLastPrefab(gameObject);
            PrefabUtility.RevertPrefabInstance(gameObject);
            this.CalculatePrefabStatus();
            Undo.RegisterCreatedObjectUndo(gameObject, "Reconnect prefab");
            GUIUtility.ExitGUI();
          }
          bool enabled2 = GUI.enabled;
          GUI.enabled = (GUI.enabled && !AnimationMode.InAnimationMode());
          if ((prefabType == PrefabType.ModelPrefabInstance || prefabType == PrefabType.PrefabInstance) && GUI.Button(position3, "Revert", "MiniButtonMid")) {
            Undo.RegisterFullObjectHierarchyUndo(gameObject, "Revert Prefab Instance");
            PrefabUtility.RevertPrefabInstance(gameObject);
            this.CalculatePrefabStatus();
            GUIUtility.ExitGUI();
          }
          if (prefabType == PrefabType.PrefabInstance || prefabType == PrefabType.DisconnectedPrefabInstance) {
            GameObject gameObject2 = PrefabUtility.FindValidUploadPrefabInstanceRoot(gameObject);
            GUI.enabled = (gameObject2 != null && !AnimationMode.InAnimationMode());
            if (GUI.Button(position4, "Apply", "MiniButtonRight")) {
              Object prefabParent = PrefabUtility.GetPrefabParent(gameObject2);
              string assetPath = AssetDatabase.GetAssetPath(prefabParent);
              bool flag2 = Provider_PromptAndCheckoutIfNeeded(new string[] {
                assetPath
              }, "The version control requires you to check out the prefab before applying changes.");
              if (flag2) {
                // ************************** BEGIN ANKI EDIT ******************************
                var expansionSettings = HierarchyExpansionUtility.GetCurrentStatusForGameObject(gameObject2);
                CleanupPrefabProxiesForSerialization(gameObject2);
                // *************************** END ANKI EDIT *******************************
                PrefabUtility.ReplacePrefab(gameObject2, prefabParent, ReplacePrefabOptions.ConnectToPrefab);
                this.CalculatePrefabStatus();

                // ************************** BEGIN ANKI EDIT ******************************
                RestorePrefabProxies(gameObject2);
                HierarchyExpansionUtility.SetCurrentStatusForGameObject(gameObject2, expansionSettings);
                // *************************** END ANKI EDIT *******************************

                GUIUtility.ExitGUI();
              }
            }
          }
          GUI.enabled = enabled2;
          if ((prefabType == PrefabType.DisconnectedModelPrefabInstance || prefabType == PrefabType.ModelPrefabInstance) && GUI.Button(position4, "Open", "MiniButtonRight")) {
            AssetDatabase.OpenAsset(PrefabUtility.GetPrefabParent(this.target));
            GUIUtility.ExitGUI();
          }
        }
      }

      // ************************** BEGIN ANKI EDIT ******************************
      var prefabProxyTransform = GetPrefabProxyTransform(gameObject != null ? gameObject.transform.parent : null);
      if (prefabProxyTransform != null && !EditorApplication.isPlayingOrWillChangePlaymode) {

        float num5 = (width - 52 - 5) / 3;
        Rect position2 = new Rect(52 + num5 * 0, 44 + y, num5, 15);
        Rect position3 = new Rect(52 + num5 * 1, 44 + y, num5, 15);
        Rect position4 = new Rect(52 + num5 * 2, 44 + y, num5, 15);

        GUIContent gUIContent2 = EditorGUIUtility_TextContent("Prefab Proxy");
        float x = GUI.skin.label.CalcSize(gUIContent2).x;

        Rect position6 = new Rect(52 - x - 5, 44 + y, x, 18);
        GUI.Label(position6, gUIContent2);

        var fullPath = FullPath(gameObject.transform);

        var prefabProxy = prefabProxyTransform.GetComponent<PrefabProxy>();
        Transform lastParent = null;
        while (prefabProxyTransform != null) {
          lastParent = prefabProxyTransform.parent;
          prefabProxyTransform = GetPrefabProxyTransform(lastParent);
        }
        bool enabled2 = GUI.enabled;
        GameObject gameObject2 = PrefabUtility.FindValidUploadPrefabInstanceRoot(lastParent.gameObject);
        GUI.enabled = (gameObject2 != null && !AnimationMode.InAnimationMode());


        if (GUI.Button(position2, "Select", "MiniButtonLeft")) {
          Selection.activeObject = prefabProxy;
          EditorGUIUtility.PingObject(Selection.activeObject);
        }

        if (GUI.Button(position3, "Revert", "MiniButtonMid")) {
          this.CalculatePrefabStatus();

          var expansionSettings = HierarchyExpansionUtility.GetCurrentStatusForGameObject(gameObject2);
          // Refresh all prefab proxy values first
          CleanupPrefabProxiesForSerialization(gameObject2);
          RestorePrefabProxies(gameObject2);

          // now clear the prefab proxy for the current proxy
          for (int i = prefabProxy.transform.childCount - 1; i >= 0; i--) {
            GameObject.DestroyImmediate(prefabProxy.transform.GetChild(i).gameObject);
          }
          // wipe out the modified fields
          prefabProxy.ModifiedValues.Clear();
          prefabProxy.ModifiedGlobalReferences.Clear();
          // remove all local references that point from this object to the parent,
          // but leave all references from the parent to this object
          prefabProxy.ModifiedLocalReferences.RemoveAll(r => !r.ObjectPath.StartsWith(".."));

          // now reexpand the proxy.
          prefabProxy.Expand();

          Selection.activeGameObject = GetTransformAtFullPath(fullPath).gameObject;
          HierarchyExpansionUtility.SetCurrentStatusForGameObject(gameObject2, expansionSettings);

          GUIUtility.ExitGUI();
        }

        if (GUI.Button(position4, "Apply", "MiniButtonRight")) {
          Object prefabParent = PrefabUtility.GetPrefabParent(gameObject2);
          string assetPath = AssetDatabase.GetAssetPath(prefabParent);
          bool flag2 = Provider_PromptAndCheckoutIfNeeded(new string[] {
            assetPath
          }, "The version control requires you to check out the prefab before applying changes.");
          if (flag2) {
            var expansionSettings = HierarchyExpansionUtility.GetCurrentStatusForGameObject(gameObject2);
            CleanupPrefabProxiesForSerialization(gameObject2);
            PrefabUtility.ReplacePrefab(gameObject2, prefabParent, ReplacePrefabOptions.ConnectToPrefab);
            RestorePrefabProxies(gameObject2);

            Selection.activeGameObject = GetTransformAtFullPath(fullPath).gameObject;
            HierarchyExpansionUtility.SetCurrentStatusForGameObject(gameObject2, expansionSettings);

            GUIUtility.ExitGUI();
          }
          GUI.enabled = enabled2;
        }
      }
      // *************************** END ANKI EDIT *******************************

      EditorGUI.EndDisabledGroup();
      base.serializedObject.ApplyModifiedProperties();
      return true;
    }

    private Object[] GetObjects(bool includeChildren) {
      return SceneModeUtility.GetObjects(base.targets, includeChildren);
    }

    public override bool HasPreviewGUI() {
      return EditorUtility.IsPersistent(this.target) && this.HasStaticPreview();
    }

    private bool HasStaticPreview() {
      if (base.targets.Length > 1) {
        return true;
      }
      if (this.target == null) {
        return false;
      }
      GameObject gameObject = this.target as GameObject;
      Camera exists = gameObject.GetComponent(typeof(Camera)) as Camera;
      return exists || GameObjectInspector.HasRenderablePartsRecurse(gameObject);
    }

    private void InitPreview() {
      if (this.m_PreviewUtility == null) {
        this.m_PreviewUtility = new PreviewRenderUtility(true);
        this.m_PreviewUtility.m_CameraFieldOfView = 30;
        this.m_PreviewUtility.m_Camera.cullingMask = 1 << Camera_PreviewCullingLayer;
        this.CreatePreviewInstances();
      }
    }

    public void OnDestroy() {
      this.DestroyPreviewInstances();
      if (this.m_PreviewUtility != null) {
        this.m_PreviewUtility.Cleanup();
        this.m_PreviewUtility = null;
      }
    }

    private void OnDisable() {
    }

    public void OnEnable() {
      this.m_Name = base.serializedObject.FindProperty("m_Name");
      this.m_IsActive = base.serializedObject.FindProperty("m_IsActive");
      this.m_Layer = base.serializedObject.FindProperty("m_Layer");
      this.m_Tag = base.serializedObject.FindProperty("m_TagString");
      this.m_StaticEditorFlags = base.serializedObject.FindProperty("m_StaticEditorFlags");
      this.m_Icon = base.serializedObject.FindProperty("m_Icon");
      this.CalculatePrefabStatus();

      if (EditorSettings.defaultBehaviorMode == EditorBehaviorMode.Mode2D) {
        this.previewDir = new Vector2(0, 0);
      }
      else {
        this.previewDir = new Vector2(120, -20);
      }
    }

    protected override void OnHeaderGUI() {
      // ************************** BEGIN ANKI EDIT ******************************
      //Rect rect = GUILayoutUtility.GetRect(0, (float)((!this.m_HasInstance) ? 40 : 60));
      var go = target as GameObject;
      if (go == null) {
        return;
      }
      Rect rect = GUILayoutUtility.GetRect(0, (float)((!this.m_HasInstance && !GetPrefabProxyTransform(go != null ? go.transform.parent : null)) ? 40 : 60));
      // *************************** END ANKI EDIT *******************************
      this.DrawInspector(rect);
    }

    public override void OnInspectorGUI() {
    }

    public override void OnPreviewGUI(Rect r, GUIStyle background) {
      if (!ShaderUtil.hardwareSupportsRectRenderTexture) {
        if (Event.current.type == EventType.Repaint) {
          EditorGUI.DropShadowLabel(new Rect(r.x, r.y, r.width, 40), "Preview requires render texture support");
        }
        return;
      }
      this.InitPreview();
      this.previewDir = PreviewGUI_Drag2D(this.previewDir, r);
      if (Event.current.type != EventType.Repaint) {
        return;
      }
      this.m_PreviewUtility.BeginPreview(r, background);
      this.DoRenderPreview();
      this.m_PreviewUtility.EndAndDrawPreview(r);
    }

    public override void OnPreviewSettings() {
      if (!ShaderUtil.hardwareSupportsRectRenderTexture) {
        return;
      }
      GUI.enabled = true;
      this.InitPreview();
    }

    public void OnSceneDrag(SceneView sceneView) {
      GameObject gameObject = this.target as GameObject;
      PrefabType prefabType = PrefabUtility.GetPrefabType(gameObject);
      if (prefabType != PrefabType.Prefab && prefabType != PrefabType.ModelPrefab) {
        return;
      }
      Event current = Event.current;
      EventType type = current.type;
      if (type != EventType.DragUpdated) {
        if (type != EventType.DragPerform) {
          if (type == EventType.DragExited) {
            if (GameObjectInspector.dragObject) {
              Object.DestroyImmediate(GameObjectInspector.dragObject, false);
              HandleUtility_ignoreRaySnapObjects = null;
              GameObjectInspector.dragObject = null;
              current.Use();
            }
          }
        }
        else {
          string uniqueNameForSibling = GameObjectUtility.GetUniqueNameForSibling(null, GameObjectInspector.dragObject.name);
          GameObjectInspector.dragObject.hideFlags = HideFlags.None;
          Undo.RegisterCreatedObjectUndo(GameObjectInspector.dragObject, "Place " + GameObjectInspector.dragObject.name);
          EditorUtility.SetDirty(GameObjectInspector.dragObject);
          DragAndDrop.AcceptDrag();
          Selection.activeObject = GameObjectInspector.dragObject;
          HandleUtility_ignoreRaySnapObjects = null;
          EditorWindow.mouseOverWindow.Focus();
          GameObjectInspector.dragObject.name = uniqueNameForSibling;
          GameObjectInspector.dragObject = null;
          current.Use();
        }
      }
      else {
        if (GameObjectInspector.dragObject == null) {
          GameObjectInspector.dragObject = (GameObject)PrefabUtility.InstantiatePrefab(PrefabUtility.FindPrefabRoot(gameObject));
          HandleUtility_ignoreRaySnapObjects = GameObjectInspector.dragObject.GetComponentsInChildren<Transform>();
          GameObjectInspector.dragObject.hideFlags = HideFlags.HideInHierarchy;
          GameObjectInspector.dragObject.name = gameObject.name;
        }
        DragAndDrop.visualMode = DragAndDropVisualMode.Copy;
        object obj = HandleUtility.RaySnap(HandleUtility.GUIPointToWorldRay(current.mousePosition));
        if (obj != null) {
          RaycastHit raycastHit = (RaycastHit)obj;
          float d = 0;
          if (Tools.pivotMode == PivotMode.Center) {
            float num = HandleUtility_CalcRayPlaceOffset(HandleUtility_ignoreRaySnapObjects, raycastHit.normal);
            if (!float.IsInfinity(num)) {
              d = Vector3.Dot(GameObjectInspector.dragObject.transform.position, raycastHit.normal) - num;
            }
          }
          GameObjectInspector.dragObject.transform.position = Matrix4x4.identity.MultiplyPoint(raycastHit.point + raycastHit.normal * d);
        }
        else {
          GameObjectInspector.dragObject.transform.position = HandleUtility.GUIPointToWorldRay(current.mousePosition).GetPoint(10);
        }
        if (sceneView.in2DMode) {
          Vector3 position = GameObjectInspector.dragObject.transform.position;
          position.z = PrefabUtility.FindPrefabRoot(gameObject).transform.position.z;
          GameObjectInspector.dragObject.transform.position = position;
        }
        current.Use();
      }
    }

    public override void ReloadPreviewInstances() {
      this.CreatePreviewInstances();
    }

    public override Texture2D RenderStaticPreview(string assetPath, Object[] subAssets, int width, int height) {
      if (!this.HasStaticPreview() || !ShaderUtil.hardwareSupportsRectRenderTexture) {
        return null;
      }
      this.InitPreview();
      this.m_PreviewUtility.BeginStaticPreview(new Rect(0, 0, (float)width, (float)height));
      this.DoRenderPreview();
      return this.m_PreviewUtility.EndStaticPreview();
    }

    private void SetLayer(int layer, bool includeChildren) {
      Object[] objects = this.GetObjects(includeChildren);
      Undo.RecordObjects(objects, "Change Layer of " + this.targetTitle);
      Object[] array = objects;
      for (int i = 0; i < array.Length; i++) {
        GameObject gameObject = (GameObject)array[i];
        gameObject.layer = layer;
      }
    }

    //
    // Nested Types
    //
    private class Styles {
      public GUIContent goIcon = EditorGUIUtility.IconContent("GameObject Icon");

      public GUIContent typelessIcon = EditorGUIUtility.IconContent("Prefab Icon");

      public GUIContent prefabIcon = EditorGUIUtility.IconContent("PrefabNormal Icon");

      public GUIContent modelIcon = EditorGUIUtility.IconContent("PrefabModel Icon");

      public GUIContent dataTemplateIcon = EditorGUIUtility.IconContent("PrefabNormal Icon");

      public GUIContent dropDownIcon = EditorGUIUtility.IconContent("Icon Dropdown");

      public float staticFieldToggleWidth = EditorStyles.toggle.CalcSize(EditorGUIUtility_TempContent("Static")).x + 6;

      public float tagFieldWidth = EditorStyles.boldLabel.CalcSize(EditorGUIUtility_TempContent("Tag")).x;

      public float layerFieldWidth = EditorStyles.boldLabel.CalcSize(EditorGUIUtility_TempContent("Layer")).x;

      public float navLayerFieldWidth = EditorStyles.boldLabel.CalcSize(EditorGUIUtility_TempContent("Nav Layer")).x;

      public GUIStyle staticDropdown = "StaticDropdown";

      public GUIStyle instanceManagementInfo = new GUIStyle(EditorStyles.helpBox);

      public GUIContent goTypeLabelMultiple = new GUIContent("Multiple");

      public GUIContent[] goTypeLabel = new GUIContent[] {
        null,
        EditorGUIUtility_TextContent("Prefab"),
        EditorGUIUtility_TextContent("Model"),
        EditorGUIUtility_TextContent("Prefab"),
        EditorGUIUtility_TextContent("Model"),
        EditorGUIUtility_TextContent("Missing|The source Prefab or Model has been deleted."),
        EditorGUIUtility_TextContent("Prefab|You have broken the prefab connection. Changes to the prefab will not be applied to this object before you Apply or Revert."),
        EditorGUIUtility_TextContent("Model|You have broken the prefab connection. Changes to the model will not be applied to this object before you Revert.")
      };

      public Styles() {
        GUIStyle gUIStyle = "MiniButtonMid";
        this.instanceManagementInfo.padding = gUIStyle.padding;
        this.instanceManagementInfo.alignment = gUIStyle.alignment;
      }
    }
  }
}
