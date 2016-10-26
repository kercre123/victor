using UnityEngine;
using UnityEngine.UI;
using Crosstales.BWF.Model;

namespace Crosstales.BWF.Demo
{
    /// <summary>Wrapper for sources.</summary>
    public class SourceEntry : MonoBehaviour
    {

        #region Variables

        public Text Text;
        public Image Icon;
        public Image Main;

        public Source Source;

        public GUIMain GuiMain;

        public Color32 EnabledColor = new Color32(0, 255, 0, 192);
        private Color32 disabledColor;

        #endregion

        #region MonoBehaviour methods

        void Start()
        {
            disabledColor = Main.color;
        }

        void Update()
        {
            Text.text = Source.Name;
            Icon.sprite = Source.Icon;

            if (GuiMain.Sources.Contains(Source.Name))
            {
                Main.color = EnabledColor;
            }
            else
            {
                Main.color = disabledColor;
            }
        }

        #endregion

        #region Public methods

        public void Click()
        {
            if (GuiMain.Sources.Contains(Source.Name))
            {
                GuiMain.Sources.Remove(Source.Name);
            }
            else
            {
                GuiMain.Sources.Add(Source.Name);
            }
        }

        #endregion
    }
}
// Copyright 2015-2016 www.crosstales.com