using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

namespace Anki.UI {
  /// <summary>
  /// Labels are graphics that display text.
  /// </summary>

  [AddComponentMenu("Anki/Anki Text", 01)]
  public class AnkiTextLegacy : UnityEngine.UI.Text {

    [SerializeField]
    private bool _AllUppercase = false;

    public bool GlowText = false;

    private string _LocalizedTextKey = String.Empty;
    private string _DisplayText = String.Empty;

    private object[] _FormattingArgs = null;

    public bool IsRaycastBlocker = false;

    // m_Text is a protected member that holds the string to display

    public override string text {
      get {
        return m_Text;
      }
      set {
        if (String.IsNullOrEmpty(value)) {
          if (String.IsNullOrEmpty(m_Text))
            return;
          m_Text = "";
          _DisplayText = "";
          SetVerticesDirty();
        }
        else if (m_Text != value) {
          m_Text = value;
          if (SetLocalizedText(m_Text)) {
            SetVerticesDirty();
            SetLayoutDirty();
          }
        }
      }
    }

    public virtual string DisplayText {
      get {
        if (String.IsNullOrEmpty(_DisplayText)) {
          return m_Text;
        }
        else {
          return _DisplayText;
        }
      }
    }

    public object[] FormattingArgs {
      get {
        return _FormattingArgs;
      }
      set {
        _FormattingArgs = value;
        Start();
      }
    }

    private bool IsLocalizationKey(string text) {
      return ((text != String.Empty) && (text.Length > 1) && ('#' == text[0]));
    }

    private bool SetLocalizedText(string text) {
      string displayText;
      if (!IsLocalizationKey(text)) {
        m_Text = text;
        displayText = text;
      }
      else {
        _LocalizedTextKey = text.Substring(1);
        displayText = Localization.Get(_LocalizedTextKey);
      }

      if (_FormattingArgs != null) {
        displayText = string.Format(displayText, _FormattingArgs);
      }

      if (_AllUppercase) {
        displayText = displayText.ToUpper();
      }

      if (_DisplayText != displayText) {
        _DisplayText = displayText;
        return true;
      }
      return false;
    }

    protected override void Start() {
      if (SetLocalizedText(m_Text)) {
        SetVerticesDirty();
        SetLayoutDirty();
      }
      this.raycastTarget = IsRaycastBlocker;
    }

    //
    // Override UI.Text methods to use LocalizedString
    //
    protected override void OnPopulateMesh(UnityEngine.UI.VertexHelper vh) {
#if UNITY_EDITOR
      if (m_Text != DisplayText && !UnityEditor.EditorApplication.isPlaying) {
        SetLocalizedText(m_Text);
      }

#endif

      string saveText = null;

      if (m_Text != DisplayText) {
        saveText = m_Text;
        m_Text = _DisplayText;
      }

      OnPopulateMeshInternal(vh);

      if (null != saveText) {
        m_Text = saveText;
      }
    }

    public override float preferredWidth {
      get {
        var settings = GetGenerationSettings(Vector2.zero);
        return cachedTextGeneratorForLayout.GetPreferredWidth(DisplayText, settings) / pixelsPerUnit;
      }
    }

    public override float preferredHeight {
      get {
        var settings = GetGenerationSettings(new Vector2(rectTransform.rect.size.x, 0.0f));
        return cachedTextGeneratorForLayout.GetPreferredHeight(DisplayText, settings) / pixelsPerUnit;
      }
    }

    public override Material GetModifiedMaterial(Material baseMaterial) {
      Material newMaterial = base.GetModifiedMaterial(baseMaterial);
      // newMaterial = baseMaterial;
      return newMaterial;
    }

    protected void OnPopulateMeshInternal(UnityEngine.UI.VertexHelper vertexHelper) {
      base.OnPopulateMesh(vertexHelper);

      if (GlowText) {
        List<UIVertex> stream = new List<UIVertex>();
        vertexHelper.GetUIVertexStream(stream);
        vertexHelper.Clear();

        UIVertex[] quad = new UIVertex[4];

        for (int i = 0; i < stream.Count; i += 6) {
          // the array is in triangles, but we want quads
          var topLeft = stream[i];
          var topRight = stream[i + 1];
          var bottomRight = stream[i + 2];
          // can ignore 3 and 5 since they are duplicates
          var bottomLeft = stream[i + 4];

          // Add 5 units to all sides;

          float outlineWidth = fontSize / 10f;

          var uvMaxX = Mathf.Max(Mathf.Max(topLeft.uv0.x, topRight.uv0.x), Mathf.Max(bottomLeft.uv0.x, bottomRight.uv0.x));
          var uvMinX = Mathf.Min(Mathf.Min(topLeft.uv0.x, topRight.uv0.x), Mathf.Min(bottomLeft.uv0.x, bottomRight.uv0.x));
          var uvMaxY = Mathf.Max(Mathf.Max(topLeft.uv0.y, topRight.uv0.y), Mathf.Max(bottomLeft.uv0.y, bottomRight.uv0.y));
          var uvMinY = Mathf.Min(Mathf.Min(topLeft.uv0.y, topRight.uv0.y), Mathf.Min(bottomLeft.uv0.y, bottomRight.uv0.y));

          var uvDiag = new Vector2(uvMaxX - uvMinX, uvMaxY - uvMinY);
          var sizeDiag = topRight.position - bottomLeft.position;

          var uvDelta = new Vector2(outlineWidth * uvDiag.x / sizeDiag.x, outlineWidth * uvDiag.y / sizeDiag.y);

          topLeft.position += new Vector3(-outlineWidth, outlineWidth, 0);
          topRight.position += new Vector3(outlineWidth, outlineWidth, 0);
          bottomLeft.position += new Vector3(-outlineWidth, -outlineWidth, 0);
          bottomRight.position += new Vector3(outlineWidth, -outlineWidth, 0);

          topLeft.uv0 += new Vector2(topLeft.uv0.x == uvMinX ? -uvDelta.x : uvDelta.x, topLeft.uv0.y == uvMinY ? -uvDelta.y : uvDelta.y);
          topRight.uv0 += new Vector2(topRight.uv0.x == uvMinX ? -uvDelta.x : uvDelta.x, topRight.uv0.y == uvMinY ? -uvDelta.y : uvDelta.y);
          bottomLeft.uv0 += new Vector2(bottomLeft.uv0.x == uvMinX ? -uvDelta.x : uvDelta.x, bottomLeft.uv0.y == uvMinY ? -uvDelta.y : uvDelta.y);
          bottomRight.uv0 += new Vector2(bottomRight.uv0.x == uvMinX ? -uvDelta.x : uvDelta.x, bottomRight.uv0.y == uvMinY ? -uvDelta.y : uvDelta.y);

          var uv1 = new Vector2(2 * (uvMaxX - uvMinX) / sizeDiag.x, 2 * (uvMaxY - uvMinY) / sizeDiag.y);

          var tangent = new Vector4(uvMinX, uvMinY, uvMaxX, uvMaxY);

          topLeft.tangent = tangent;
          topRight.tangent = tangent;
          bottomLeft.tangent = tangent;
          bottomRight.tangent = tangent;

          topLeft.uv1 = uv1;
          topRight.uv1 = uv1;
          bottomLeft.uv1 = uv1;
          bottomRight.uv1 = uv1;

          quad[0] = topLeft;
          quad[1] = topRight;
          quad[2] = bottomRight;
          quad[3] = bottomLeft;

          vertexHelper.AddUIVertexQuad(quad);
        }
      }
    }
  }
}