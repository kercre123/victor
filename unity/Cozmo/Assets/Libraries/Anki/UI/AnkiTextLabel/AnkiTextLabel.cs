using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

namespace Anki.UI {
  /// <summary>
  /// Labels are graphics that display text.
  /// </summary>

  [AddComponentMenu("Anki/Text", 01)]
  public class AnkiTextLabel : UnityEngine.UI.Text {

    private string _LocalizedTextKey = String.Empty;
    private string _DisplayText = String.Empty;

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
          SetLocalizedText(m_Text);

          SetVerticesDirty();
          SetLayoutDirty();
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

    private bool IsLocalizationKey(string text) {
      return ((text != String.Empty) && (text.Length > 1) && ('#' == text[0]));
    }

    private void SetLocalizedText(string text) {
      if (!IsLocalizationKey(text)) {
        m_Text = text;
        _DisplayText = text;
        return;
      }

      _LocalizedTextKey = text.Substring(1);
      _DisplayText = Localization.Get(_LocalizedTextKey);

      // INGO - These lines weren't in the original version from OverDrive
      m_Text = _DisplayText;
      SetVerticesDirty();
      SetLayoutDirty();
      // end new lines
    }

    protected override void Awake() {
      SetLocalizedText(m_Text);
      SetVerticesDirty();
      SetLayoutDirty();
    }

    // INGO - These lines weren't in the original version from OverDrive
    #if UNITY_EDITOR
    private void Update() {
      if (m_Text != DisplayText && !UnityEditor.EditorApplication.isPlaying) {
        SetLocalizedText(m_Text);
      }
    }
    #endif
    // end new lines

    //
    // Override UI.Text methods to use LocalizedString
    //
    // TODO: INGO - Unity 5.2.2 is throwing an error because OnFillVBO is
    // obsolete (and we call base.OnFillVBO in this method). Need to understand
    // what these methods do so that we can update them.
    /*protected void _OnFillVBO(List<UIVertex> vbo) { 
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

      base.OnFillVBO(vbo);

      if (null != saveText) {
        m_Text = saveText;
      }
    }

    // TODO: INGO - Need to override to use OnPopulateMesh(VertexHelper vh)
    // instead. Not sure if it's possible here.
    protected override void OnPopulateMesh(Mesh m) {
      var vbo = new List<UIVertex>();
      _OnFillVBO(vbo);

      using (var vh = new UnityEngine.UI.VertexHelper()) {
        var quad = new UIVertex[4];
        for (int i = 0; i < vbo.Count; i += 4) {
          vbo.CopyTo(i, quad, 0, 4);
          vh.AddUIVertexQuad(quad);
        }
        vh.FillMesh(m);
      }
    }*/

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

  }
}