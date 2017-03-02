using UnityEngine;
using System.Collections;
using System.Reflection;
using System;
using Newtonsoft.Json;
using System.Collections.Generic;
using Cozmo.Util;
using UnityEngine.UI;

[ExecuteInEditMode]
public class PrefabProxy : MonoBehaviour {

  [System.Serializable]
  public abstract class ModifiedField {
    // Path relative to the prefab proxy object using delimiter '/'
    public string ObjectPath;

    // in the format Type:Field[:SubField][:Index]
    public string FieldName;

    protected GameObject GetGameObject(GameObject gameObject, string path) {
      var t = gameObject.transform;

      var splitPath = path.Split('/');

      var go = GetGameObjectInternal(t, splitPath, 0);

      // if our path ends on a prefab proxy, expand it and return the expansion.
      if (go != null) {
        var proxy = go.GetComponent<PrefabProxy>();
        if (proxy != null) {
          return proxy.Expand();
        }
      }
      else {
        DAS.Error(this, string.Format("Couldn't find a GameObject with path: '{0}' on GameObject: '{1}'!",
          path, gameObject.name));
      }
      return go;
    }

    private static readonly Dictionary<string, Type> _TypeCache = new Dictionary<string, Type>();

    private struct FieldSetterAndType {
      public FieldSetter Setter;
      public Type Type;
    }

    private static readonly Dictionary<string, FieldSetterAndType> _SetterAndTypeCache = new Dictionary<string, FieldSetterAndType>();


    private GameObject GetGameObjectInternal(Transform t, string[] path, int index) {
      if (t == null) {
        return null;
      }

      if (index == path.Length) {
        return t.gameObject;
      }
      string curr = path[index];

      if (curr == "..") {

        var n = t.parent;

        // prefab proxy still exists in edit mode, which throws off relative paths by one
        if (!Application.isPlaying) {
          if (n.GetComponent<PrefabProxy>()) {
            n = n.parent;
          }
        }

        return GetGameObjectInternal(n, path, index + 1);
      }
      else if (curr == "." || curr == string.Empty) {

        // prefab proxy still exists in edit mode, which throws off relative paths by one
        if (!Application.isPlaying) {
          var proxy = t.GetComponent<PrefabProxy>();
          if (proxy != null) {
            t = proxy.Expand().transform;
          }
        }
        return GetGameObjectInternal(t, path, index + 1);
      }
      else {
        // if we hit a dead end, check if its caused by an
        // unexpanded prefab proxy
        var next = t.FindChild(curr);
        if (next == null) {
          var proxy = t.GetComponent<PrefabProxy>();
          if (proxy != null) {
            next = proxy.Expand().transform.FindChild(curr);
          }
        }
        return GetGameObjectInternal(next, path, index + 1);
      }
    }

    public bool FindField(GameObject instance, out object obj, out Type fieldType, out FieldSetter setter) {
      obj = null;
      fieldType = null;
      setter = null;

      if (string.IsNullOrEmpty(FieldName)) {
        DAS.Error(this, "FieldName is null or empty!");
        return false;
      }

      var typeSplit = FieldName.Split(':');
      var typeName = typeSplit[0];

      Type type;
      if (!_TypeCache.TryGetValue(typeName, out type)) {

        type = Type.GetType(typeName);
        if (type == null) {
          var assemblies = AppDomain.CurrentDomain.GetAssemblies();
          foreach (var asm in assemblies) {
            type = asm.GetType(typeName);
            if (type != null) {
              break;
            }
          }
        }
        _TypeCache[typeName] = type;
      }

      if (type == null) {
        DAS.Error(this, "Could not find type " + typeName + " in " + instance.name);
        return false;
      }

      var gameObject = GetGameObject(instance, ObjectPath);

      if (gameObject == null) {
        DAS.Error(this, "Could not find object at path " + ObjectPath);
        return false;
      }

      if (typeSplit.Length == 1) {
        obj = gameObject;
        fieldType = type;
        setter = (o, v) => {
          ((GameObject)o).AddComponent(type);
          return o;
        };
        return true;
      }

      obj = gameObject.GetComponent(type);

      FieldSetterAndType fsat;
      if (!_SetterAndTypeCache.TryGetValue(FieldName, out fsat)) {
        fieldType = type;
        setter = BuildSetter(typeSplit, 1, ref fieldType);
        _SetterAndTypeCache[FieldName] = new FieldSetterAndType() { Setter = setter, Type = fieldType };
      }
      else {
        setter = fsat.Setter;
        fieldType = fsat.Type;
      }

      return obj != null && setter != null && fieldType != null;
    }
  }

  public delegate object FieldSetter(object obj, object value);

  private static FieldSetter BuildSetter(string[] typeSplit, int i, ref Type type) {
    Func<object, object> newGetter;
    FieldSetter newSetter;

    int index;
    if (int.TryParse(typeSplit[i], out index)) {
      Type elementType;
      if (type.IsArray) {
        elementType = type.GetElementType();
      }
      else {
        elementType = type.GetGenericArguments().FirstOrDefault() ?? typeof(object);
      }
      type = elementType;
      newGetter = (o) => ((IList)o)[index];
      newSetter = (o, v) => {
        var list = (IList)o;
        list[index] = v;
        return list;
      };
    }
    else {
      var field = type.GetField(typeSplit[i]);
      var subType = type;
      while (field == null && subType != null && subType != typeof(System.Object) && subType != typeof(System.Enum)) {
        field = subType.GetField(typeSplit[i], BindingFlags.Instance | BindingFlags.NonPublic);
        subType = subType.BaseType;
      }
      if (field == null) {
        DAS.Error(typeof(ModifiedField), "Could not find field " + typeSplit[i] + " for type " + type.Name);
        return null;
      }
      type = field.FieldType;
      newGetter = (o) => field.GetValue(o);
      if (typeof(UnityEngine.Object).IsAssignableFrom(type)) {
        newSetter = (o, v) => {
          // because unity does weird things with null
          UnityEngine.Object uo = (UnityEngine.Object)v;
          if (uo == null) {
            field.SetValue(o, null);
          }
          else {
            field.SetValue(o, uo);
          }
          return o;
        };

      }
      else {
        newSetter = (o, v) => {
          field.SetValue(o, v);
          return o;
        };
      }
    }

    if (i + 1 < typeSplit.Length) {
      var inner = BuildSetter(typeSplit, i + 1, ref type);
      if (inner == null) {
        return null;
      }

      return (o, v) => {
        return newSetter(o, inner(newGetter(o), v));
      };
    }
    else {
      return newSetter;
    }
  }



  [System.Serializable]
  public class ModifiedLocalReferenceField : ModifiedField {

    public string ReferencePath;

    public object GetReferencedObject(GameObject instance, Type type) {

      var referenceObj = GetGameObject(instance, ReferencePath);

      if (referenceObj == null) {
        return null;
      }

      if (type == typeof(GameObject)) {
        return referenceObj;
      }
      else {
        return referenceObj.GetComponent(type);
      }
    }

    public override string ToString() {
      return string.Format("[ModifiedLocalReferenceField ObjectPath={0}, FieldName={1}, ReferencePath={2}]", ObjectPath, FieldName, ReferencePath);
    }
  }

  [System.Serializable]
  public class ModifiedGlobalReferenceField : ModifiedField {

    public UnityEngine.Object Reference;

    public override string ToString() {
      return string.Format("[ModifiedGlobalReferenceField ObjectPath={0}, FieldName={1}, Reference={2}]", ObjectPath, FieldName, Reference);
    }
  }

  [System.Serializable]
  public class ModifiedValueField : ModifiedField {

    [SerializeField]
    private string JsonValue;

    public object GetObjectValue(Type type) {
      return JsonConvert.DeserializeObject(JsonValue, type, GlobalSerializerSettings.JsonSettings);
    }

    public void SetObjectValue(object value) {
      JsonValue = JsonConvert.SerializeObject(value, Formatting.None, GlobalSerializerSettings.JsonSettings);
    }

    public override string ToString() {
      return string.Format("[ModifiedValueField ObjectPath={0}, FieldName={1}, JsonValue={2}]", ObjectPath, FieldName, JsonValue);
    }
  }

  public GameObject Prefab;

  /// <summary>
  /// Instance values like colors, strings, ints, bools
  /// </summary>
  [HideInInspector]
  public List<ModifiedValueField> ModifiedValues = new List<ModifiedValueField>();

  /// <summary>
  /// References to other gameObjects in the heirarchy or vice versa, like gameObjects, scripts, button references
  /// </summary>
  [HideInInspector]
  public List<ModifiedLocalReferenceField> ModifiedLocalReferences = new List<ModifiedLocalReferenceField>();

  // References to
  /// <summary>
  /// References to files in project, like materials, textures, fonts, asset files
  /// </summary>
  [HideInInspector]
  public List<ModifiedGlobalReferenceField> ModifiedGlobalReferences = new List<ModifiedGlobalReferenceField>();


  // Use this for initialization
  private void Awake() {
    Expand();
  }

  private GameObject _Instance;

  public GameObject Expand() {
    if (Prefab == null) {
      return null;
    }
    // if we already expanded, or are in the process of doing so
    if (_Instance != null) {
      return _Instance;
    }

    if (Application.isPlaying) {

      _Instance = (GameObject)GameObject.Instantiate(Prefab);

      _Instance.name = name;

      _Instance.transform.SetParent(transform.parent);

      var rectTransform = GetComponent<RectTransform>();

      if (rectTransform != null) {
        var instanceRectTransform = _Instance.GetComponent<RectTransform>() ?? _Instance.AddComponent<RectTransform>();

        instanceRectTransform.anchorMin = rectTransform.anchorMin;
        instanceRectTransform.anchorMax = rectTransform.anchorMax;
        instanceRectTransform.offsetMin = rectTransform.offsetMin;
        instanceRectTransform.offsetMax = rectTransform.offsetMax;
        instanceRectTransform.pivot = rectTransform.pivot;
        instanceRectTransform.localScale = rectTransform.localScale;
        instanceRectTransform.localRotation = rectTransform.localRotation;
        instanceRectTransform.localPosition = rectTransform.localPosition;
      }
      else {
        _Instance.transform.localPosition = transform.localPosition;
        _Instance.transform.localScale = transform.localScale;
        _Instance.transform.localRotation = transform.localRotation;
      }

      // layout element is a special case
      var oldLayoutElement = GetComponent<LayoutElement>();

      if (oldLayoutElement != null) {
        var newLayoutElement = _Instance.GetComponent<LayoutElement>() ?? _Instance.AddComponent<LayoutElement>();

        newLayoutElement.ignoreLayout = oldLayoutElement.ignoreLayout;
        newLayoutElement.flexibleWidth = oldLayoutElement.flexibleWidth;
        newLayoutElement.flexibleHeight = oldLayoutElement.flexibleHeight;
        newLayoutElement.minWidth = oldLayoutElement.minWidth;
        newLayoutElement.minHeight = oldLayoutElement.minHeight;
        newLayoutElement.preferredWidth = oldLayoutElement.preferredWidth;
        newLayoutElement.preferredHeight = oldLayoutElement.preferredHeight;
      }

      _Instance.transform.SetSiblingIndex(transform.GetSiblingIndex());
      GameObject.DestroyImmediate(gameObject);

      ExpandReferences(_Instance);
      return _Instance;
    }
    else {
      _Instance = (GameObject)GameObject.Instantiate(Prefab);

      _Instance.name = name + "(Proxy)";

      // Fix duplicate bug
      for (int i = transform.childCount - 1; i >= 0; i--) {
        DestroyImmediate(transform.GetChild(i).gameObject);
      }

      _Instance.transform.SetParent(transform);

      var rectTransform = GetComponent<RectTransform>();

      if (rectTransform != null) {
        var instanceRectTransform = _Instance.GetComponent<RectTransform>() ?? _Instance.AddComponent<RectTransform>();

        instanceRectTransform.anchorMin = Vector2.zero;
        instanceRectTransform.anchorMax = Vector2.one;
        instanceRectTransform.offsetMin = Vector2.zero;
        instanceRectTransform.offsetMax = Vector2.zero;
        instanceRectTransform.pivot = rectTransform.pivot;
        instanceRectTransform.localScale = Vector3.one;
        instanceRectTransform.localRotation = Quaternion.identity;
        instanceRectTransform.localPosition = Vector3.zero;
      }
      else {
        _Instance.transform.localPosition = Vector3.zero;
        _Instance.transform.localScale = Vector3.one;
        _Instance.transform.localRotation = Quaternion.identity;
      }

      _Instance.hideFlags = HideFlags.DontSave;
      ExpandReferences(_Instance);

      return _Instance;
    }
  }

  private void ExpandReferences(GameObject instance) {
    for (int i = 0; i < ModifiedValues.Count; i++) {
      Type type;
      FieldSetter setter;
      object component;

      if (ModifiedValues[i].FindField(instance, out component, out type, out setter)) {
        try {
          setter(component, ModifiedValues[i].GetObjectValue(type));
        }
        catch (Exception ex) {
          DAS.Error(this, "Error Applying Modified Value " + ModifiedValues[i]);
          DAS.Error(this, ex);
        }
      }
      else {
        DAS.Error(this, "Invalid ModifiedValue: " + ModifiedValues[i].ToString());
      }
    }

    for (int i = 0; i < ModifiedLocalReferences.Count; i++) {
      Type type;
      FieldSetter setter;
      object component;

      if (ModifiedLocalReferences[i].FindField(instance, out component, out type, out setter)) {
        try {
          setter(component, ModifiedLocalReferences[i].GetReferencedObject(instance, type));
        }
        catch (Exception ex) {
          DAS.Error(this, "Error Applying Modified Local Reference " + ModifiedLocalReferences[i]);
          DAS.Error(this, ex);
        }
      }
      else {
        DAS.Error(this, "Invalid ModifiedLocalReference: " + ModifiedLocalReferences[i].ToString());
      }
    }

    for (int i = 0; i < ModifiedGlobalReferences.Count; i++) {
      Type type;
      FieldSetter setter;
      object component;

      if (ModifiedGlobalReferences[i].FindField(instance, out component, out type, out setter)) {
        try {
          setter(component, ModifiedGlobalReferences[i].Reference);
        }
        catch (Exception ex) {
          DAS.Error(this, "Error Applying Modified Global Reference " + ModifiedGlobalReferences[i]);
          DAS.Error(this, ex);
        }
      }
      else {
        DAS.Error(this, "Invalid ModifiedGlobalReference: " + ModifiedGlobalReferences[i].ToString());
      }
    }
  }
}
