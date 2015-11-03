using UnityEngine;
using UnityEditor;
using System.Collections;
using System.IO;

namespace DigitalRuby.ThunderAndLightning.Editor
{
    [InitializeOnLoad]
    public class MoveGizmosScript
    {
        private static readonly string sourcePath = Path.Combine(Application.dataPath, "ProceduralLightning/Prefab/Editor/Gizmos");

        static MoveGizmosScript()
        {
            string destinationPath = Path.Combine(Application.dataPath, "Gizmos");
            Directory.CreateDirectory(destinationPath);
            string[] gizmos = Directory.GetFiles(sourcePath, "*.png");
            foreach (string gizmo in gizmos)
            {
                string destFile = Path.Combine(destinationPath, Path.GetFileName(gizmo));
                FileInfo srcInfo = new FileInfo(gizmo);
                FileInfo dstInfo = new FileInfo(destFile);
                if (!dstInfo.Exists || srcInfo.LastWriteTimeUtc > dstInfo.LastWriteTimeUtc)
                {
                    srcInfo.CopyTo(dstInfo.FullName, true);
                }
            }
        }
    }
}
