package com.anki.cozmo;

import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import android.support.v4.app.ActivityCompat;

import java.io.File;
import java.util.UUID;

import com.unity3d.player.UnityPlayer;
import com.unity3d.player.UnityPlayerActivity;

import com.anki.hockeyappandroid.NativeCrashManager;
import com.anki.util.AnkitivityDispatcher;
import com.anki.util.PermissionUtil;

/**
 * Custom Activity implementation designed to work with AnkitivityDispatcher, so that results of permission requests
 * and other activities can be dispatched to code in the anki-util repository that doesn't know about CozmoActivity
 */
public class CozmoActivity extends UnityPlayerActivity implements ActivityCompat.OnRequestPermissionsResultCallback {

  private final AnkitivityDispatcher mDispatcher = new AnkitivityDispatcher();

  public static String APP_RUN_ID;
  public static String HOCKEY_APP_ID;

  static {
    System.loadLibrary("DAS");
    System.loadLibrary("cozmoEngine");
  }

  @Override
  protected void onCreate(final android.os.Bundle bundle) {
    PermissionUtil.initialize(this, mDispatcher);
    super.onCreate(bundle);

    APP_RUN_ID = UUID.randomUUID().toString();
    HOCKEY_APP_ID = getManifestProperty("HOCKEYAPP_APP_ID");

    if (HOCKEY_APP_ID.length() > 0) {
      installBreakpad(NativeCrashManager.getDumpsDirectory(this) + File.separator
                      + APP_RUN_ID + NativeCrashManager.DMP_EXTENSION);
    }
  }

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

  // request a permission and specify the game object/method name in unity to receive the results
  public void unityRequestPermission(final String permission, final String gameObject, final String methodCallback) {
    AnkitivityDispatcher.PermissionListener listener = new AnkitivityDispatcher.PermissionListener() {
      @Override public void onRequestPermissionsResult(String[] permissions, int[] grantResults) {
        final String result = grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED ?
          "true" : "false";
        UnityPlayer.UnitySendMessage(gameObject, methodCallback, result);
      }
    };
    PermissionUtil.askForPermission(permission, listener);
  }

  private String getManifestProperty(String propertyName) {
    try {
      ApplicationInfo appInfo = getPackageManager().getApplicationInfo(getPackageName(), PackageManager.GET_META_DATA);
      if (appInfo != null && appInfo.metaData != null) {
        return appInfo.metaData.getString(propertyName);
      }
    } catch (Exception e) {}

    return "";
  }

  private static native void installBreakpad(String path);

}
