package com.anki.util;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;

import android.support.v4.app.ActivityCompat;

public final class PermissionUtil {

  private static Activity mActivity;
  private static AnkitivityDispatcher mDispatcher;

  public static void initialize(final Activity activity, final AnkitivityDispatcher dispatcher) {
    mActivity = activity;
    mDispatcher = dispatcher;
  }

  public static void askForPermission(final String permission,
                                      final AnkitivityDispatcher.PermissionListener callback) {
    final int requestCode = mDispatcher.subscribePermissionResult(callback);
    ActivityCompat.requestPermissions(mActivity, new String[]{permission}, requestCode);
  }

  public static boolean hasPermission(final String permission) {
    return ActivityCompat.checkSelfPermission(mActivity, permission) == PackageManager.PERMISSION_GRANTED;
  }

  public static boolean shouldShowRationale(final String permission) {
    return ActivityCompat.shouldShowRequestPermissionRationale(mActivity, permission);
  }

  public static void openAppSettings() {
    Intent intent = new Intent();
    intent.setAction(android.provider.Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
    intent.setData(Uri.fromParts("package", mActivity.getPackageName(), null));
    mActivity.startActivity(intent);
  }

}
