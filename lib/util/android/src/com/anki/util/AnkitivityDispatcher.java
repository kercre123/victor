package com.anki.util;

import android.content.Intent;

import java.util.HashMap;


/**
 * This class dispatches results from an Activity to objects that subscribe to the dispatcher
 * to receive those updates. An AnkitivityDispatcher should be created in an Activity subclass
 * and injected into dependent modules. This allows code in the anki-util repository to receive
 * activity results from activities defined in a parent repository, without having any knowledge
 * of what those Activity subclasses are.
 *
 * This class is currently set up to dispatch two kinds of activity results:
 * - requests for application permissions
 * - requests for activity results (return codes from new activities started by this activity)
 */
public class AnkitivityDispatcher {

  public interface ResultListener {
    public void onActivityResult(int resultCode, Intent data);
  }

  public interface PermissionListener {
    public void onRequestPermissionsResult(String[] permissions, int[] grantResults);
  }

  private static final String TAG = "AnkiUtil";

  private static int requestCounter = 0;

  private final HashMap<Integer, ResultListener> resultMap = new HashMap<Integer, ResultListener>();
  private final HashMap<Integer, PermissionListener> permissionMap = new HashMap<Integer, PermissionListener>();

  // Generates a unique request code for use
  public int subscribeActivityResult(final ResultListener l) {
    final int requestCode = requestCounter++;
    resultMap.put(requestCode, l);
    return requestCode;
  }

  public void onActivityResult(final int requestCode, final int resultCode, final Intent data) {
    final ResultListener l = resultMap.remove(requestCode);
    if (l != null) {
      l.onActivityResult(resultCode, data);
    }
  }

  public int subscribePermissionResult(final PermissionListener l) {
    final int requestCode = requestCounter++;
    permissionMap.put(requestCode, l);
    return requestCode;
  }

  public void onRequestPermissionsResult(final int requestCode, final String[] permissions, final int[] grantResults) {
    final PermissionListener l = permissionMap.remove(requestCode);
    if (l != null) {
      l.onRequestPermissionsResult(permissions, grantResults);
    }
  }

}
