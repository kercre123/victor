package com.anki.cozmo;

import android.Manifest;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.NetworkInfo;
import android.net.wifi.ScanResult;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.util.Log;

import android.support.v4.app.ActivityCompat;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.anki.daslib.DAS;

public final class CozmoWifi {

  private static Activity mActivity;
  private static WifiManager mWifiManager;

  private static BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
      logWifiEvents(context, intent);
    }
  };

  public static boolean register(Activity activity) {
    mActivity = activity;
    mWifiManager = (WifiManager)mActivity.getSystemService(Context.WIFI_SERVICE);
    IntentFilter intentFilter = new IntentFilter();
    intentFilter.addAction(WifiManager.NETWORK_IDS_CHANGED_ACTION);
    intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
    intentFilter.addAction(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION);
    intentFilter.addAction(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
    intentFilter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
    intentFilter.addAction(WifiManager.WIFI_STATE_CHANGED_ACTION);
    mActivity.registerReceiver(mBroadcastReceiver, intentFilter);

    mWifiManager.startScan();
    return true;
  }

  public static void unregister() {
    if (mActivity != null) {
      mActivity.unregisterReceiver(mBroadcastReceiver);
      mActivity = null;
    }
  }

  private static void logWifiEvents(Context context, Intent intent) {
    final String action = intent.getAction();
    if (WifiManager.NETWORK_IDS_CHANGED_ACTION.equals(action)) {
      DAS.Event("android.wifi.network_ids_changed_action", "");
    } else if (WifiManager.NETWORK_STATE_CHANGED_ACTION.equals(action)) {
      NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
      if (networkInfo != null) {
        DAS.Event("android.wifi.network_state_changed_action.detailed_state", networkInfo.getDetailedState().toString());
        if (networkInfo.isConnected()) {
          WifiInfo wifiInfo = intent.getParcelableExtra(WifiManager.EXTRA_WIFI_INFO);
          if (wifiInfo != null) {
            String ssid = wifiInfo.getSSID();
            boolean isCozmoAP = isCozmoSSID(ssid);
            if (isCozmoAP) {
              DAS.Event("android.wifi.connected_to_cozmo_access_point",
                dequoteSSID(wifiInfo.getSSID()) + ","
                + wifiInfo.getBSSID() + ","
                + getHostAddress(wifiInfo.getIpAddress()) + ","
                + (wifiInfo.getHiddenSSID() ? "hidden" : "visible") + ","
                + wifiInfo.getRssi() + ","
                + wifiInfo.getFrequency() + wifiInfo.FREQUENCY_UNITS + ","
                + wifiInfo.getLinkSpeed() + wifiInfo.LINK_SPEED_UNITS);

            } else {
              DAS.Event("android.wifi.connected_to_other_access_point", "");
            }
          }
        }
      }
    } else if (WifiManager.SCAN_RESULTS_AVAILABLE_ACTION.equals(action)) {
      List<ScanResult> scanResults = mWifiManager.getScanResults();
      if (scanResults != null) {
        int otherAPs = 0;
        int cozmoAPs = 0;
        for (final ScanResult scanResult : scanResults) {
          boolean isCozmoAP = isCozmoSSID(scanResult.SSID);
          if (isCozmoAP) {
            cozmoAPs++;
            DAS.Event("android.wifi.cozmo_ap_scanned",
              dequoteSSID(scanResult.SSID) + ","
              + scanResult.BSSID + ","
              + scanResult.level + ","
              + scanResult.timestamp + ","
              + scanResult.capabilities);
          } else {
            otherAPs++;
          }
        }
        boolean zeroAPs = otherAPs + cozmoAPs == 0;
        boolean hasLocationPermission =
          ActivityCompat.checkSelfPermission(mActivity, Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED;
        if (zeroAPs && !hasLocationPermission) {
          DAS.Event("android.wifi.scan_results.no_permission", "");
        } else {
          DAS.Event("android.wifi.scan_results.cozmo_vs_other_ap_counts",
            "" + cozmoAPs + "," + otherAPs);
        }
      }
    } else if (WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION.equals(action)) {
      boolean supplicantConnected = intent.getBooleanExtra(WifiManager.EXTRA_SUPPLICANT_CONNECTED,
       false);
      DAS.Event("android.wifi.supplicant_connected", String.valueOf(supplicantConnected));
    } else if (WifiManager.SUPPLICANT_STATE_CHANGED_ACTION.equals(action)) {
      SupplicantState supplicantState = intent.getParcelableExtra(WifiManager.EXTRA_NEW_STATE);
      DAS.Event("android.wifi.supplicant_state_changed", supplicantState.toString());
    } else if (WifiManager.WIFI_STATE_CHANGED_ACTION.equals(action)) {
      int wifiState =
        intent.getIntExtra(WifiManager.EXTRA_WIFI_STATE, WifiManager.WIFI_STATE_UNKNOWN);
      int previousWifiState =
        intent.getIntExtra(WifiManager.EXTRA_PREVIOUS_WIFI_STATE, WifiManager.WIFI_STATE_UNKNOWN);
      DAS.Event("android.wifi.state_change",
        wifiStateToString(previousWifiState) + " -> "
        + wifiStateToString(wifiState));
    }
  }

  private static String wifiStateToString(final int wifiState) {
    switch(wifiState) {
      case WifiManager.WIFI_STATE_DISABLED:
      return "disabled";
      case WifiManager.WIFI_STATE_DISABLING:
      return "disabling";
      case WifiManager.WIFI_STATE_ENABLED:
      return "enabled";
      case WifiManager.WIFI_STATE_ENABLING:
      return "enabling";
      case WifiManager.WIFI_STATE_UNKNOWN:
      return "unknown";
      default:
      return Integer.toString(wifiState);
    }
  }

  private static String getHostAddress(final int ipAddress) {
    final byte[] quads = new byte[4];
    for (int k = 0; k < 4; k++) {
      quads[k] = (byte) (ipAddress >> k * 8 & 0xff);
    }
    try {
      InetAddress address = InetAddress.getByAddress(quads);
      return address.getHostAddress();
    } catch (UnknownHostException e) {
      return "";
    }
  }

  private static boolean isCozmoSSID(final String ssid) {
    if (ssid == null) {
      return false;
    }
    String dequotedSSID = dequoteSSID(ssid);
    Pattern p = Pattern.compile("^Cozmo_[0-8]{2}[0-9A-F]{4}$");
    Matcher m = p.matcher(dequotedSSID);
    return m.matches();
  }

  private static String dequoteSSID(final String ssid) {
    if (ssid == null) {
      return null;
    }
    if (ssid.startsWith("\"") && ssid.endsWith("\"")) {
      return ssid.substring(1, ssid.length() - 1);
    } else {
      return ssid;
    }
  }
}
