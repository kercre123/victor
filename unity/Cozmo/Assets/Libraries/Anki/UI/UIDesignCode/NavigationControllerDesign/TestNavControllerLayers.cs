using UnityEngine;
using System.Collections;
using Anki.UI;

public class TestNavControllerLayers : Anki.UI.NavigationController {

  public View PrefabBackgroundView;

  public View PrefabForegroundView;

  protected override void ViewDidLoad()
  {
    base.ViewDidLoad();

    PrefabBackgroundView.gameObject.SetActive(true);
//    RectTransform transform = PrefabBackgroundView.GetComponent<RectTransform>();
//    transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);
    BackgroundView = Instantiate(PrefabBackgroundView);

    ForegroundView = Instantiate(PrefabForegroundView);

//    RectTransform transform = BackgroundView.GetComponent<RectTransform>();
//    transform.localScale = new Vector3(1.0f, 1.0f, 1.0f);

    StartCoroutine("ChangeBackgroundAndForegroundViews");
  }

  IEnumerator ChangeBackgroundAndForegroundViews() {
    yield return new WaitForSeconds(3.0f);
    BackgroundView =  Instantiate(PrefabForegroundView);
    yield return new WaitForSeconds(1.0f);
    ForegroundView = Instantiate(PrefabBackgroundView);
  }
}
