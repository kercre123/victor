using UnityEngine;
using System.Collections;

public class TestListView_DataSource : Anki.UI.DataSource {

  uint cellCount = 2;
	public override uint NumberOfItems(uint section)
  {
    return cellCount;
  }

  public override object ObjectAtIndexPath(Anki.UI.IndexPath indexPath)
  {
    return "String Obj " + indexPath.ToString();
  }

  public override bool IsIndexPathValid(Anki.UI.IndexPath indexPath)
  {
    return (indexPath.Item < cellCount);
  }



  protected override void OnEnable()
  {
    base.OnEnable();

//    StartCoroutine("AddItems");
  }

  IEnumerator AddItems() {

    yield return new WaitForSeconds(1);

    ++cellCount;
    DataDidUpdate();

    yield return new WaitForSeconds(1);
    ++cellCount;
    ObjectDataDidUpdate(null);

    yield return new WaitForSeconds(1);
    ++cellCount;
    DataDidUpdate();

    yield return new WaitForSeconds(1);
    ++cellCount;
    DataDidUpdate();

    yield return new WaitForSeconds(1);
    ++cellCount;
    DataDidUpdate();
  }

}
