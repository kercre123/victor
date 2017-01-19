package com.anki.cozmo;

import android.content.Intent;
import android.util.Log;

import android.support.v4.app.ActivityCompat;
import com.unity3d.player.UnityPlayerActivity;

import com.anki.util.AnkitivityDispatcher;

/**
 * Custom Activity implementation designed to work with AnkitivityDispatcher, so that results of permission requests
 * and other activities can be dispatched to code in the anki-util repository that doesn't know about CozmoActivity
 */
public class CozmoActivity extends UnityPlayerActivity implements ActivityCompat.OnRequestPermissionsResultCallback {

  private AnkitivityDispatcher mDispatcher = new AnkitivityDispatcher();

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
    super.onActivityResult(requestCode, resultCode, intent);
    mDispatcher.onActivityResult(requestCode, resultCode, intent);
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    mDispatcher.onRequestPermissionsResult(requestCode, permissions, grantResults);
  }

  public AnkitivityDispatcher getDispatcher() {
    return mDispatcher;
  }

}
