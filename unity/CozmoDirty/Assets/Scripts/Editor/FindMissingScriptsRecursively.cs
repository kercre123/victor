using UnityEngine;
using UnityEditor;

public class FindMissingScriptsRecursively : EditorWindow {
  static int go_count = 0, components_count = 0, missing_count = 0;

  [MenuItem("Window/FindMissingScriptsRecursively")]
  public static void ShowWindow() {
    EditorWindow.GetWindow(typeof(FindMissingScriptsRecursively));
  }

  public void OnGUI() {
    if (GUILayout.Button("Find Missing Scripts in selected GameObjects")) {
      FindInSelected();
    }
  }

  private static void FindInSelected() {
    GameObject[] go = Selection.gameObjects;
    go_count = 0;
    components_count = 0;
    missing_count = 0;
    foreach (GameObject it in go) {
      GameObject g = it; // because foreach is borked
      FindInGO(g);
    }
    DAS.Debug("FindMissingScriptsRecrusively", string.Format("Searched {0} GameObjects, {1} components, found {2} missing", go_count, components_count, missing_count));
  }

  private static void FindInGO(GameObject g) {
    go_count++;
    Component[] components = g.GetComponents<Component>();
    for (int i = 0; i < components.Length; i++) {
      components_count++;
      if (components[i] == null) {
        missing_count++;
        string s = g.name;
        Transform t = g.transform;
        while (t.parent != null) {
          s = t.parent.name + "/" + s;
          t = t.parent;
        }
        DAS.Debug("FindMissingScriptsRecrusively", s + " has an empty script attached in position: " + i);
      }
    }
    // Now recurse through each child GO (if there are any):
    foreach (Transform it in g.transform) {
      Transform childT = it; // because foreach is borked
      FindInGO(childT.gameObject);
    }
  }
}