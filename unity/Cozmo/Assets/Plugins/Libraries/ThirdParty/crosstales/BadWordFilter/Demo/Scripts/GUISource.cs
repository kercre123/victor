using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Crosstales.BWF.Model;

namespace Crosstales.BWF.Demo
{
    /// <summary>Generates a scrollable list of sources.</summary>
    public class GUISource : MonoBehaviour
    {

        #region Variables

        public GameObject ItemPrefab;
        public GameObject Target;
        public Scrollbar Scroll;
        public GUIMain GuiMain;
        public int ColumnCount = 1;
        public Vector2 SpaceWidth = new Vector2(8, 8);
        public Vector2 SpaceHeight = new Vector2(8, 8);

        #endregion

        #region MonoBehaviour methods

        void Start()
        {
            StartCoroutine(buildLanguageList());
        }

        #endregion

        #region Private methods

        private IEnumerator buildLanguageList()
        {

            while (!BWFManager.isReady)
            {
                yield return null;
            }

            RectTransform rowRectTransform = ItemPrefab.GetComponent<RectTransform>();
            RectTransform containerRectTransform = Target.GetComponent<RectTransform>();

            for (var i = Target.transform.childCount - 1; i >= 0; i--)
            {
                Transform child = Target.transform.GetChild(i);
                child.SetParent(null);
                Destroy(child.gameObject);
                // Optionally destroy the objectA if not longer needed
            }

            List<Source> items = BWFManager.Sources(ManagerMask.BadWord);
            //Debug.Log("ITEMS: " + items.Count);

            //calculate the width and height of each child item.
            float width = containerRectTransform.rect.width / ColumnCount - (SpaceWidth.x + SpaceWidth.y) * ColumnCount;
            //float ratio = width / rowRectTransform.rect.width;
            //float height = rowRectTransform.rect.height * ratio;
            float height = rowRectTransform.rect.height - (SpaceHeight.x + SpaceHeight.y);

            int rowCount = items.Count / ColumnCount;

            if (rowCount > 0 && items.Count % rowCount > 0)
            {
                rowCount++;
            }

            //adjust the height of the container so that it will just barely fit all its children
            float scrollHeight = height * rowCount;
            containerRectTransform.offsetMin = new Vector2(containerRectTransform.offsetMin.x, -scrollHeight / 2);
            containerRectTransform.offsetMax = new Vector2(containerRectTransform.offsetMax.x, scrollHeight / 2);

            int j = 0;
            for (int ii = 0; ii < items.Count; ii++)
            {
                //this is used instead of a double for loop because itemCount may not fit perfectly into the rows/columns
                if (ii % ColumnCount == 0)
                {
                    j++;
                }

                //create a new item, name it, and set the parent
                GameObject newItem = Instantiate(ItemPrefab) as GameObject;
                newItem.name = Target.name + " item at (" + ii + "," + j + ")";
                newItem.transform.SetParent(Target.transform);
                newItem.transform.localScale = Vector3.one;
                //newItem.GetComponent<Image>().color = ii%2 == 0 ? evenColor : oddColor;

                //newItem.GetComponent<Image>().sprite = ;

                newItem.GetComponent<SourceEntry>().Source = items[ii];
                newItem.GetComponent<SourceEntry>().GuiMain = GuiMain;


                //move and size the new item
                RectTransform rectTransform = newItem.GetComponent<RectTransform>();

                float x = -containerRectTransform.rect.width / 2 + (width + SpaceWidth.x) * (ii % ColumnCount) + SpaceWidth.x * ColumnCount;
                float y = containerRectTransform.rect.height / 2 - height * j;
                rectTransform.offsetMin = new Vector2(x, y);

                x = rectTransform.offsetMin.x + width;
                y = rectTransform.offsetMin.y + height;
                rectTransform.offsetMax = new Vector2(x, y);
            }

            Scroll.value = 1f;
        }

        #endregion
    }
}
// Copyright 2015-2016 www.crosstales.com