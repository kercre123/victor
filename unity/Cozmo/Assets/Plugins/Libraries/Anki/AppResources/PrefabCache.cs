using System.Collections;
using System.Collections.Generic;

namespace Anki.AppResources {
  public partial class PrefabCache {

    private Dictionary<string, string> _PrefabPathsByName;

    // Override this method to define platform
    partial void Configure();

    public PrefabCache() {
      _PrefabPathsByName = new Dictionary<string, string>();
      Configure();
    }

    public bool TryGetPath(string prefabName, out string path) {
      return _PrefabPathsByName.TryGetValue(prefabName, out path);
    }
  }
}
