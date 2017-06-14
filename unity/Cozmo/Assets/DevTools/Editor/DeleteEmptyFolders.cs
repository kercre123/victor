using System.IO;
using System.Linq;
using UnityEditor;
using UnityEngine;

public class DeleteEmptyFolders {

  // http://docs.unity3d.com/ScriptReference/AssetPostprocessor.OnPostprocessAllAssets.html
  // https://gist.github.com/liortal53/780075ddb17f9306ae32
  [MenuItem("Cozmo/Remove empty folders")]
  static void ProcessDeleteEmptyFolders() {
    var index = Application.dataPath.IndexOf("/Assets");
    var projectSubfolders = Directory.GetDirectories(Application.dataPath, "*", SearchOption.AllDirectories);

    // Create a list of all the empty subfolders under Assets.
    var emptyFolders = projectSubfolders.Where(path => IsEmptyRecursive(path)).ToArray();

    int numFoldersDeleted = 0;
    foreach (var folder in emptyFolders) {
      // Verify that the folder exists (may have been already removed).
      if (Directory.Exists(folder)) {
        Debug.Log("Deleting : " + folder);

        // Remove dir (recursively)
        Directory.Delete(folder, true);
        numFoldersDeleted++;

        // Sync AssetDatabase with the delete operation.
        AssetDatabase.DeleteAsset(folder.Substring(index + 1));
      }
    }

    // Refresh the asset database once we're done.
    AssetDatabase.Refresh();

    Debug.Log("Finished deleting empty folders. Folders deleted: " + numFoldersDeleted);
  }

  /// <summary>
  /// A helper method for determining if a folder is empty or not.
  /// </summary>
  private static bool IsEmptyRecursive(string path) {
    // A folder is empty if it (and all its subdirs) have no files (ignore .meta files)
    return Directory.GetFiles(path).Select(file => !file.EndsWith(".meta")).Count() == 0
    && Directory.GetDirectories(path, string.Empty, SearchOption.AllDirectories).All(IsEmptyRecursive);
  }
}
