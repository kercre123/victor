package com.anki.util;

import android.Manifest;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import android.support.v4.app.ActivityCompat;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;

/**
 * This class contains a number of utility functions for wifi connectivity, including:
 * - initiating a connection to a requested wifi network with an SSID/password (or just
 *   the SSID if the phone already has a configuration saved for that network) and
 *   tracking the result of that connection attempt
 * - if requested, binding the current process to communicate on a given network, which
 *   is necessary on newer Android OS versions for a phone to communicate with a wifi
 *   endpoint that does not provide an internet connection
 * - conducting ping attempts to detect if a given network endpoing is reachable
 * - keeping track of current network connectivity state to be able to report status
 */
public final class WifiUtil {

  private static Activity mActivity;
  private static AnkitivityDispatcher mDispatcher;
  private static WifiManager mWifiManager;
  private static ConnectivityManager mConnectivityManager;
  private static List<ScanResult> mScanResults = new ArrayList<ScanResult>();
  private static final String TAG = "AnkiUtil";
  private static String mCurrentSSID = "";
  private static String mCurrentStatus = "";
  private static NetworkInfo mCurrentNetInfo;
  private static final Handler mHandler = new Handler(Looper.getMainLooper());
  private static PingTester mPingTester;
  private static boolean mPermissionIssue;

  // connection attempt state
  private static WifiConnectionAttempt mConnectionAttempt;

  private enum UpdateType {
    WIFI,
    CONNECTIVITY
  }


  public static void register(final Activity activity, final AnkitivityDispatcher dispatcher) {
    registerInternal(activity, dispatcher);
  }

  public static void unregister() {
    if (mActivity != null) {
      mActivity.unregisterReceiver(mBroadcastReceiver);
      mActivity = null;
    }
  }

  public static Collection<ScanResult> getScanResults() {
    return mScanResults;
  }

  public static String[] getSSIDs() {
    final String[] array = new String[mScanResults.size()];
    int i = 0;
    for (ScanResult result : mScanResults) {
      array[i] = result.SSID;
      i++;
    }
    return array;
  }

  // attempt to connect to the given WPA2 network/password
  // note that a return value of true does not imply connection success,
  // only that it didn't immediately fail
  public static boolean connect(final String SSID, final String password, final int timeoutMs) {
    Log.i(TAG, "Attempting to connect to network " + SSID);

    if (handleAlreadyConnected(SSID)) {
      return true;
    }

    int networkId = findExistingConfig(SSID);

    if (networkId < 0) {
      Log.v(TAG, "Adding new network config");
      final WifiConfiguration config = new WifiConfiguration();
      config.SSID = "\"" + SSID + "\"";
      config.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
      config.preSharedKey = "\"" + password + "\"";
      config.priority = 1;

      networkId = mWifiManager.addNetwork(config);
    }

    return connectExisting(networkId, SSID, timeoutMs);
  }

  public static boolean connectExisting(final String SSID, final int timeoutMs) {
    if (handleAlreadyConnected(SSID)) {
      return true;
    }
    return connectExisting(findExistingConfig(SSID), SSID, timeoutMs);
  }

  public static boolean reconnect(final int timeoutMs) {
    if (mConnectionAttempt == null) {
      Log.d(TAG, "attempting reconnect when no connect attempt is active");
    }
    final boolean result = mConnectionAttempt != null && mConnectionAttempt.connect(timeoutMs);
    return result;
  }

  public static void disconnect() {
    if (mConnectionAttempt != null) {
      mConnectionAttempt.unregister();
      mConnectionAttempt = null;
    }
    final boolean result = mWifiManager.disconnect();
    Log.d(TAG, "disconnect result: " + result);
  }

  public static boolean didPasswordFail() {
    return mConnectionAttempt != null && mConnectionAttempt.didPasswordFail();
  }

  public static boolean isExistingConfigurationForNetwork(final String SSID) {
    return findExistingConfig(SSID) >= 0;
  }

  public static boolean requestScan() {
    return mWifiManager.startScan();
  }

  public static String dequoteSSID(final String ssid) {
    if (ssid == null) {
      return "";
    }
    if (ssid.startsWith("\"") && ssid.endsWith("\"")) {
      return ssid.substring(1, ssid.length() - 1);
    } else {
      return ssid;
    }
  }

  public static String getCurrentSSID() {
    return mCurrentSSID;
  }

  public static String getCurrentStatus() {
    return mCurrentStatus;
  }

  public static void beginPingTest(final String ipAddress, final int timeoutMs) {
    beginPingTest(ipAddress, timeoutMs, 0);
  }

  public static void beginPingTest(final String ipAddress, final int timeoutMs, final int delayMs) {
    if (mPingTester != null) {
      return;
    }
    Log.v(TAG, "starting ping test");
    mPingTester = new PingTester(ipAddress, timeoutMs, delayMs);
    mPingTester.start();
  }

  public static boolean isPingSuccessful() {
    return mPingTester != null && mPingTester.get();
  }

  public static void endPingTest() throws InterruptedException {
    if (mPingTester != null) {
      Log.v(TAG, "ending ping test");
      mPingTester.interrupt();
      mPingTester.join();
      mPingTester = null;
    }
  }

  public static boolean isPermissionIssue() {
    return mPermissionIssue;
  }

  public static boolean hasLocationPermission() {
    return PermissionUtil.hasPermission(Manifest.permission.ACCESS_COARSE_LOCATION);
  }

  public static boolean shouldShowLocationPermissionRequest() {
    return PermissionUtil.shouldShowRationale(Manifest.permission.ACCESS_COARSE_LOCATION);
  }

  public static void requestLocationPermission() {
    final AnkitivityDispatcher.PermissionListener listener = new AnkitivityDispatcher.PermissionListener() {
      @Override public void onRequestPermissionsResult(final String[] permissions, final int[] grantResults) {
        MessageSender.sendMessage("permissionResult",
          new String[] { grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED ? "true" : "false" });
      }
    };
    PermissionUtil.askForPermission(Manifest.permission.ACCESS_COARSE_LOCATION, listener);
  }

  public static boolean needLocationService() {
    final boolean isMarshmallowPlus = isVersionAtLeast(Build.VERSION_CODES.M);
    return isMarshmallowPlus && !LocationUtil.isLocationServiceEnabled(mActivity);
  }

  public static boolean isWifiEnabled() {
    return mWifiManager.isWifiEnabled();
  }

  public static void enableWifi() {
    final boolean result = mWifiManager.setWifiEnabled(true);
    Log.v(TAG, "wifi enable result is: " + result);
  }

  public static interface BindCallback {
    public void run(boolean success);
  }
  /**
   * Binds this process to the specified network. The OS will use this network for all
   * communication by this process after binding. This may require already-established
   * sockets to be reset to take effect.
   *
   * Because this can fail for a couple reasons (sometimes right after a connection
   * happens, the ConnectivityManager does not yet have the corresponding Network for
   * a given NetworkInfo in its list of networks; also, the bindProcessToNetwork call
   * can sometimes return false, indicating the operation failed), a callback is required.
   * If the operation immediately succeeds the callback will be asynchronously invoked with
   * a value of "true"; if it doesn't immediately succeed, an asynchronous task will
   * try again for several times in the next ~1 second. After enough failures the task will
   * give up and invoke the callback with a value of false, or invoke it with a value of
   * true as soon as the operation succeeds.
   */
  public static void bindToNetwork(final NetworkInfo info, final BindCallback callback) {
    // before android 5 the API to do this does not exist (and isn't required)
    if (!isVersionAtLeast(Build.VERSION_CODES.LOLLIPOP)) {
      if (callback != null) {
        callback.run(true);
      }
      return;
    }

    final Network network = findNetworkForInfo(info);
    boolean didBind = false;
    if (network != null) {
      didBind = attemptNetworkBind(network);
    }
    if (didBind) {
      mHandler.post(new Runnable() {
        @Override public void run() { callback.run(true); }
      });
    }
    else {
      // sometimes the network isn't immediately available. wait a little and try again
      Log.d(TAG, network != null ? "couldn't find Network for this NetworkInfo to tie to!!" : "bind operation failed!!");
      mHandler.postDelayed(new Runnable() {
        private int count = 0;
        @Override
        public void run() {
          final Network network = findNetworkForInfo(info);
          Log.v(TAG, "Searching again... result is: " + (network == null ? "null" : "not null"));
          boolean didBind = false;
          if (network != null) {
            Log.v(TAG, "found: " + mConnectivityManager.getNetworkInfo(network).toString());
            didBind = attemptNetworkBind(network);
          }
          if (didBind) {
            Log.v(TAG, "successfully bound network");
            callback.run(true);
          }
          else {
            count++;
            if (count > 10) {
              // give up
              Log.v(TAG, "giving up on ever finding this network");
              callback.run(false);
            }
            else {
              mHandler.postDelayed(this, 100);
            }
          }
        }
      }, 100);
    }
  }

  public static void unbindFromNetwork() {
    // cancel any pending bind operations
    mHandler.removeCallbacksAndMessages(null);

    Log.v(TAG, "untying network");
    if (isVersionAtLeast(Build.VERSION_CODES.M)) {
      mConnectivityManager.bindProcessToNetwork(null);
      NativeBindNetworkCallback(0);
    }
    else if (isVersionAtLeast(Build.VERSION_CODES.LOLLIPOP)) {
      mConnectivityManager.setProcessDefaultNetwork(null);
      NativeBindLollipopCallback();
    }
  }

  /**
   * Determines if the given network state is good enough for us to use for connection purposes.
   * The phone might report a network state of "VERIFYING_POOR_LINK" indicating it's determining
   * if the network has internet available; even though it's not reporting "CONNECTED", however,
   * we can still bind to the network and communicate to an endpoint in this state.
   */
  public static boolean isConnectedState(final NetworkInfo.DetailedState netState) {
    switch (netState) {
      case CONNECTED:
      case VERIFYING_POOR_LINK:
      case CAPTIVE_PORTAL_CHECK:
        return true;
      default:
        return false;
    }
  }

  /**
   * If we're passed a null NetworkInfo in wifi state updates, try to find it from other sources
   */
  public static NetworkInfo findWifiNetworkInfo() {
    for (final Network network : mConnectivityManager.getAllNetworks()) {
      final NetworkInfo info = mConnectivityManager.getNetworkInfo(network);
      if (info != null && info.getType() == ConnectivityManager.TYPE_WIFI) {
        return info;
      }
    }
    return null;
  }

  private static boolean attemptNetworkBind(final Network network) {
    boolean result = false;
    if (isVersionAtLeast(Build.VERSION_CODES.M)) {
      result = mConnectivityManager.bindProcessToNetwork(network);
      Log.v(TAG, "tie result is " + result);
      if (result) {
        NativeBindNetworkCallback(network.getNetworkHandle());
      }
    }
    else if (isVersionAtLeast(Build.VERSION_CODES.LOLLIPOP)) {
      result = mConnectivityManager.setProcessDefaultNetwork(network);
      Log.v(TAG, "lollipop tie result is " + result);
      if (result) {
        NativeBindLollipopCallback();
      }
    }
    else {
      // shouldn't be called in this state
      Log.v(TAG, "no binding required pre-lollipop");
      return true;
    }
    return result;
  }

  private static final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(final Context context, final Intent intent) {
      final String action = intent.getAction();
      if (action.equals(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)) {
        handleScanResults(false);
      }
      else if (action.equals(ConnectivityManager.CONNECTIVITY_ACTION)) {
        final NetworkInfo info = intent.getParcelableExtra(ConnectivityManager.EXTRA_NETWORK_INFO);
        handleNetworkUpdate(info, UpdateType.CONNECTIVITY);
      }
      else if (action.equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {
        final NetworkInfo info = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
        handleNetworkUpdate(info, UpdateType.WIFI);
      }
      else if (WifiManager.WIFI_STATE_CHANGED_ACTION.equals(action)) {
        final int wifiState = intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, WifiManager.WIFI_STATE_UNKNOWN);
        handleWifiStateChange(wifiState);
      }
    }
  };

  private static void registerInternal(Activity activity, AnkitivityDispatcher dispatcher) {
    mActivity = activity;
    mDispatcher = dispatcher;
    mWifiManager = (WifiManager)mActivity.getSystemService(Context.WIFI_SERVICE);
    mConnectivityManager = (ConnectivityManager)mActivity.getSystemService(Context.CONNECTIVITY_SERVICE);
    final IntentFilter intentFilter = new IntentFilter();
    intentFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
    intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
    intentFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
    intentFilter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
    mActivity.registerReceiver(mBroadcastReceiver, intentFilter);

    mWifiManager.startScan();
  }

  /**
   * Given an SSID, search for an existing wifi configuration known to the OS
   * with that SSID and return its integer identifier if one exists (else -1)
   */
  private static int findExistingConfig(final String SSID) {
    final List<WifiConfiguration> list = mWifiManager.getConfiguredNetworks();
    if (list != null) {
      for (WifiConfiguration i : list) {
        if (i.SSID != null && i.SSID.equals("\"" + SSID + "\"")) {
          return i.networkId;
        }
      }
    }
    return -1;
  }

  /**
   * At the start of a connection attempt, detect if we're already connected to that network
   * and should immediately return success
   */
  private static boolean handleAlreadyConnected(final String SSID) {
    if (SSID.equals(mCurrentSSID) && isConnectedState(mCurrentNetInfo.getDetailedState())) {
      Log.v(TAG, "already connected to this network");
      onConnectionFinished(true, mCurrentNetInfo);
      return true;
    }
    return false;
  }

  private static boolean connectExisting(final int networkId, final String SSID, final int timeoutMs) {
    if (networkId < 0) {
      Log.d(TAG, "Invalid networkId passed to connectExisting()");
      return false;
    }

    if (mConnectionAttempt != null) {
      mConnectionAttempt.unregister();
    }
    mConnectionAttempt = new WifiConnectionAttempt(networkId, SSID, mActivity,
      new WifiConnectionAttempt.ConnectionListener() {
        @Override public void onFinished(final boolean success, final NetworkInfo info) {
          onConnectionFinished(success, info);
        }
    });
    return mConnectionAttempt.connect(timeoutMs);
  }

  private static void onConnectionFinished(final boolean success, final NetworkInfo info) {
    Log.v(TAG, "connection result listener - success is " + success);
    if (success) {
      if (mConnectionAttempt != null) {
        mConnectionAttempt.unregister();
      }
      mConnectionAttempt = null;
    }
    MessageSender.sendMessage("connectionFinished", new String[] {success ? "true" : "false"});
  }

  private static void handleScanResults(final boolean forceEmptyList) {
    try {
      mScanResults = forceEmptyList ? new ArrayList<ScanResult>() : mWifiManager.getScanResults();
    } catch (SecurityException e) {}

    // filter out duplicate SSID names, as they are not helpful
    final HashMap<String, ScanResult> resultsDict = new HashMap<String, ScanResult>();
    for (ScanResult result : mScanResults) {
      ScanResult existing = resultsDict.get(result.SSID);
      // if this SSID is already in the map, take whichever one has a stronger RSSI
      if (existing == null || result.level > existing.level) {
        resultsDict.put(result.SSID, result);
      }
    }
    mScanResults = new ArrayList(resultsDict.values());
    Collections.sort(mScanResults, new Comparator<ScanResult>() {
      @Override
      public int compare(ScanResult l, ScanResult r) {
        // descending order, highest RSSI first
        return r.level - l.level;
      }
    });

    // send results to native function
    final String[] ssids = new String[mScanResults.size()];
    final int[] levels = new int[mScanResults.size()];
    int i = 0;
    for (ScanResult result : mScanResults) {
      ssids[i] = dequoteSSID(result.SSID);
      levels[i] = result.level;
      i++;
    }
    NativeScanCallback(ssids, levels);
    MessageSender.sendMessage("scanResults", ssids);

    // figure out if we have a permissions issue we need to solve to get scan results working
    if (mScanResults.isEmpty()) {
      final boolean isMarshmallowPlus = isVersionAtLeast(Build.VERSION_CODES.M);
      final boolean noLocationPermission = isMarshmallowPlus && !hasLocationPermission();
      final boolean locationDisabled = needLocationService();
      final boolean noWifi = !mWifiManager.isWifiEnabled();

      mPermissionIssue = noLocationPermission || locationDisabled || noWifi;
      if (mPermissionIssue) {
        MessageSender.sendMessage("scanPermissionsIssue", null);
      }
    }
    else {
      mPermissionIssue = false;
    }
  }

  private static void handleNetworkUpdate(NetworkInfo info, final UpdateType type) {
    if (info == null) {
      if (type == UpdateType.WIFI) {
        info = findWifiNetworkInfo();
      }
      if (info == null) {
        return;
      }
    }
    Log.v(TAG, type.toString() + " update: " + info.toString());
    if (type == UpdateType.CONNECTIVITY) {
      // wifi state updates seem to be more reliable indicators of wifi connections;
      // connectivity updates might just reflect we don't have internet on wifi, which
      // we already know all too well
      return;
    }
    final boolean isWifiNetwork = info.getType() == ConnectivityManager.TYPE_WIFI;
    if (isWifiNetwork) {
      mCurrentSSID = dequoteSSID(info.getExtraInfo());
      mCurrentStatus = info.getDetailedState().toString();
      mCurrentNetInfo = info;
      NativeStatusCallback(mCurrentSSID, mCurrentStatus);
      MessageSender.sendMessage("wifiStatus", new String[] {mCurrentSSID, mCurrentStatus});
    }
  }

  private static void handleWifiStateChange(final int wifiState) {
    if (wifiState == WifiManager.WIFI_STATE_DISABLING || wifiState == WifiManager.WIFI_STATE_DISABLED) {
      Log.v(TAG, "wifi being disabled, force scan update to remove SSIDs");
      handleScanResults(true);
    }
  }

  private static Network findNetworkForInfo(final NetworkInfo info) {
    // why does this method have to be a thing? am I just an idiot and missing something?
    final String tag = TAG + ".WifiUtil.findNetworkForInfo";
    String infoString = info.toString();
    Network foundNetwork = null;
    Network lessStrictNetwork = null;
    for (Network network : mConnectivityManager.getAllNetworks()) {
      final NetworkInfo tempInfo = mConnectivityManager.getNetworkInfo(network);
      if (tempInfo == null) {
        Log.v(tag, "Skip invalid network " + network);
      }
      else if (infoString.equals(tempInfo.toString())) {
        if (foundNetwork != null) {
          Log.v(tag, "found duplicate info!!");
        }
        foundNetwork = network;
      }
      else if (tempInfo.getType() == info.getType() && tempInfo.getExtraInfo() != null
               && tempInfo.getExtraInfo().equals(info.getExtraInfo())) {
        lessStrictNetwork = network;
      }
    }
    if (foundNetwork == null && lessStrictNetwork == null) {
      Log.v(tag, "AAAHHH COULDN'T FIND NETWORK");
    }
    return foundNetwork != null ? foundNetwork : lessStrictNetwork;
  }

  private static boolean isVersionAtLeast(int versionCode) {
    return Build.VERSION.SDK_INT >= versionCode;
  }

  /////////////////////////////////////////////////////////////////////////////////////////////////
  // PING TESTER
  /////////////////////////////////////////////////////////////////////////////////////////////////

  private static class PingTester extends Thread {
    private String host;
    private boolean success;
    private int timeoutMs;
    private int delayMs;
    
    public PingTester(final String host, final int timeoutMs, final int delayMs) {
      this.host = host;
      this.timeoutMs = timeoutMs;
      this.delayMs = delayMs;
    }

    @Override
    public void run() {
      try {
        while (true) {
          if (Thread.interrupted()) {
            return;
          }

          doPing();

          sleep(delayMs);
        }
      } catch (Exception e) {
        // exception likely means we were interrupted by the main thread with the intent we terminate
        return;
      }
    }

    private void doPing() {
      InetAddress hostAddress;
      try {
        hostAddress = InetAddress.getByName(host);
      } catch (UnknownHostException e) {
        Log.v(TAG, "host exception = " + e.getMessage());
        set(false);
        return;
      }
      try {
        if (hostAddress.isReachable(this.timeoutMs)) {
          set(true);
        }
        else {
          set(false);
        }
      } catch (IOException e) {
        Log.v(TAG, "io exception = " + e.getMessage());
        set(false);
        return;
      }
    }

    private synchronized void set(final boolean success) {
      Log.v(TAG, "ping result is: " + success);
      this.success = success;
    }

    public synchronized boolean get() {
      return this.success;
    }
  }

  private static native void NativeScanCallback(final String[] ssids, final int[] levels);
  private static native void NativeStatusCallback(final String ssid, final String status);
  private static native void NativeBindNetworkCallback(long netId);
  private static native void NativeBindLollipopCallback();
}
