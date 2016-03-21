using UnityEngine;
using System;

namespace Anki.AppResources
{
  public static partial class SpriteCache {

    public static Sprite LoadSprite(string path) {
      Sprite sprite = Resources.Load<Sprite>(path);
      return sprite;
    }

    public static void RemoveUnusedSprites() {
      Resources.UnloadUnusedAssets();
    }

  }
}
 // namespace Anki.AppResources
