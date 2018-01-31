/**
 * File: DAS.java
 *
 * Author: seichert
 * Created: 07/08/14
 *
 * Description: Java wrappers around native DAS functions
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

package com.anki.daslib;

import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.BatteryManager;
import android.os.Build;
import android.os.Environment;
import android.os.StatFs;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.Base64;
import android.util.Log;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.UnsupportedEncodingException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.io.StringReader;
import java.lang.SecurityException;
import java.net.URL;
import java.net.URLEncoder;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import java.net.HttpURLConnection;
import javax.net.ssl.HttpsURLConnection;

import org.apache.http.message.BasicNameValuePair;
import org.apache.http.NameValuePair;

import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

public class DAS {

    private final static String TAG = "daslib";

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("DAS");
    }

    public final static int LogLevel_Debug = 0;
    public final static int LogLevel_Info = 1;
    public final static int LogLevel_Event = 2;
    public final static int LogLevel_Warn = 3;
    public final static int LogLevel_Error = 4;

    public final static int DisableNetworkReason_Simulator = 1 << 0;
    public final static int DisableNetworkReason_UserOptOut = 1 << 1;

    public static void Debug(String eventName, String eventValue_format, Object ... args) {
        println(LogLevel_Debug, eventName, eventValue_format, args);
    }

    public static void Info(String eventName, String eventValue_format, Object ... args) {
        println(LogLevel_Info, eventName, eventValue_format, args);
    }

    public static void Event(String eventName, String eventValue_format, Object ... args) {
        println(LogLevel_Event, eventName, eventValue_format, args);
    }

    public static void Warn(String eventName, String eventValue_format, Object ... args) {
        println(LogLevel_Warn, eventName, eventValue_format, args);
    }

    public static void Error(String eventName, String eventValue_format, Object ... args) {
        println(LogLevel_Error, eventName, eventValue_format, args);
    }

    public static void println(int dasLogLevel, String eventName, String eventValue_format, Object ... args) {
        if (IsEventEnabledForLevel(eventName, dasLogLevel)) {
            // The eventValue_format cannot have a single % without a valid format specifier following it. 
            // Ex. "OnTextchanged %" is not valid and will cause a crash
            nativeLog(dasLogLevel, eventName, String.format(eventValue_format, (Object[]) args));
        }
    }

    public static boolean postToServer(String url, String postBody, ByteBuffer out_response5k) {
        HttpURLConnection conn = null;
        boolean success = true;
        try {
            URL serverUrl = new URL(url);
            if (url.startsWith("https")) {
                conn = (HttpsURLConnection) serverUrl.openConnection();
            } else {
                conn = (HttpURLConnection) serverUrl.openConnection();
            }
            conn.setReadTimeout(10000);
            conn.setConnectTimeout(15000);
            conn.setRequestMethod("POST");
            conn.setDoInput(true);
            conn.setDoOutput(true);

            conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");

            String postBodyBase64 = Base64.encodeToString(postBody.getBytes("UTF-8"), Base64.DEFAULT);

            List<NameValuePair> params = new ArrayList<NameValuePair>();
            params.add(new BasicNameValuePair("Action", "SendMessage"));
            params.add(new BasicNameValuePair("Version", "2011-10-01"));
            params.add(new BasicNameValuePair("MessageBody", postBodyBase64));

            String finalPostBody = getQuery(params);

            conn.setRequestProperty("Content-Length", new Integer(finalPostBody.length()).toString());
            conn.setFixedLengthStreamingMode(finalPostBody.length());

            OutputStream os = conn.getOutputStream();
            BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(os, "UTF-8"));
            writer.write(finalPostBody);
            writer.flush();
            writer.close();
            os.close();
            InputStreamReader responseReader;
            int responseCode = conn.getResponseCode();
            if (responseCode < 200 || responseCode > 299) {
                Log.v(TAG, "Warning! Upload to DAS server returned a response code of " + responseCode);
                responseReader = new InputStreamReader(conn.getErrorStream());
            }
            else
            {
                responseReader = new InputStreamReader(conn.getInputStream());
            }
            int maxCharacters = 5 * 1024 - 1;
            char[] responseRaw = new char[maxCharacters];
            int length = responseReader.read(responseRaw, 0, maxCharacters);
            responseReader.close();

            String responseString = new String(responseRaw, 0, length);

            Boolean responseContainsError = false;
            try {
                XmlPullParserFactory factory = XmlPullParserFactory.newInstance();
                factory.setNamespaceAware(true);
                XmlPullParser xpp = factory.newPullParser();
                xpp.setInput(new StringReader(responseString));
                int eventType = xpp.getEventType();
                while (eventType != XmlPullParser.END_DOCUMENT) {
                    if(eventType == XmlPullParser.START_TAG && xpp.getName() == "ErrorResponse") {
                        responseContainsError = true;
                        break;
                    }
                    eventType = xpp.next();
                }
            } catch (org.xmlpull.v1.XmlPullParserException e) {
                responseContainsError = true;
            }

            out_response5k.put(responseString.getBytes(), 0, length);
            out_response5k.put((byte)'\0');

            if (responseString.isEmpty() || responseContainsError || responseCode < 200 || responseCode > 299) {
                success = false;
            }

        } catch (java.io.IOException e) {
            Log.v(TAG, "Error! Unable to upload DAS events: ", e);
            return false;
        } finally {
            if (conn != null) {
                conn.disconnect();
            }
        }

        return success;
    }

    private static String getQuery(List<NameValuePair> params) throws UnsupportedEncodingException
    {
        StringBuilder result = new StringBuilder();
        boolean first = true;

        for (NameValuePair pair : params)
        {
            if (first)
                first = false;
            else
                result.append("&");

            result.append(URLEncoder.encode(pair.getName(), "UTF-8"));
            result.append("=");
            result.append(URLEncoder.encode(pair.getValue(), "UTF-8"));
        }

        return result.toString();
    }

    ////////////////////////////////////////////////////////////
    // device info accessors, copied from OverDriveActivity.java
    ////////////////////////////////////////////////////////////

    public static String getCombinedSystemVersion() {
        String s = String.format("%s-%s-%s-%s",
                getSystemName(),
                getModel(),
                getAndroidOsVersion(),
                getOsBuildVersion());
        return s;
    }

    private static String getSystemName() {
        return "Android OS";
    }

    private static String capitalize(final String s) {
        if (s == null || s.isEmpty()) {
            return "";
        }
        final char first = s.charAt(0);
        if (Character.isUpperCase(first)) {
            return s;
        } else {
            return Character.toUpperCase(first) + s.substring(1);
        }
    }

    public static String getModel() {
        final String manufacturer = Build.MANUFACTURER;
        final String model = Build.MODEL;
        if (model.startsWith(manufacturer)) {
            return capitalize(model);
        } else {
            return capitalize(manufacturer) + " " + model;
        }
    }

    public static boolean IsOnKindle() {
        return getModel().toLowerCase().contains("amazon");
    }

    private static String getAndroidOsVersion() {
        return Build.VERSION.RELEASE;
    }

    private static String getOsBuildVersion() {
        return Build.VERSION.INCREMENTAL;
    }

    public static String getOsVersion() {
        String s = String.format("%s-%s-%s",
                getSystemName(),
                getAndroidOsVersion(),
                getOsBuildVersion());
        return s;
    }

    public static String getDeviceID(String path) {
        String uniqueID = null;

        File uuidFile = new File(path);
        if (uuidFile.exists()) {
            BufferedReader reader = null;
            try {
                reader = new BufferedReader(new FileReader(uuidFile));
                uniqueID = reader.readLine();
            } catch (IOException e) { 
            } finally {
                if (reader != null) {
                    try {
                        reader.close();
                    } catch (IOException ioe) { }
                }
            }
        }

        if (uniqueID != null)
        {
            return uniqueID;
        }

        // Otherwise time to create a new UUID and store to file
        uniqueID = UUID.randomUUID().toString();
        try {
            uuidFile.getParentFile().mkdirs();
            PrintWriter writer = null;
            try {
                writer = new PrintWriter(uuidFile);
                writer.println(uniqueID);
            } catch (FileNotFoundException fe) {
            } finally {
                if (writer != null) {
                    writer.close();
                }
            }

        } catch (SecurityException se) { }

        return uniqueID;
    }

    public static String getFreeDiskSpace() {
        StatFs statFs = new StatFs(Environment.getRootDirectory().getAbsolutePath());
        long free = statFs.getFreeBytes();
        String freeDiskSpace = String.valueOf(free);
        return freeDiskSpace;
    }

    public static String getTotalDiskSpace() {
        StatFs statFs = new StatFs(Environment.getRootDirectory().getAbsolutePath());
        long total = statFs.getTotalBytes();
        String totalDiskSpace = String.valueOf(total);
        return totalDiskSpace;
    }

    public static String getBatteryLevel(Context context) {
        IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent batteryStatus = context.registerReceiver(null, ifilter);

        int level = batteryStatus.getIntExtra(BatteryManager.EXTRA_LEVEL, -1);
        int scale = batteryStatus.getIntExtra(BatteryManager.EXTRA_SCALE, -1);

        double batteryLevel = level / (double) scale; // should produce values from 0.0 to 1.0

        return String.valueOf(batteryLevel);
    }

    public static String getBatteryState(Context context) {
        IntentFilter ifilter = new IntentFilter(Intent.ACTION_BATTERY_CHANGED);
        Intent batteryStatus = context.registerReceiver(null, ifilter);

        // Return our status as a string
        int status = batteryStatus.getIntExtra(BatteryManager.EXTRA_STATUS, -1);

        String statusString = "unknown";

        switch (status) {
            case BatteryManager.BATTERY_STATUS_NOT_CHARGING:
                statusString = "not charging";
                break;
            case BatteryManager.BATTERY_STATUS_DISCHARGING:
                statusString = "discharging";
                break;
            case BatteryManager.BATTERY_STATUS_CHARGING:
                statusString = "charging";
                break;
            case BatteryManager.BATTERY_STATUS_FULL:
                statusString = "charged";
                break;
            case BatteryManager.BATTERY_STATUS_UNKNOWN:
            default:
                statusString = "unknown";
                break;
        }
        return statusString;
    }

    public static String getPlatform() {
        return IsOnKindle() ? "kindle" : "android";
    }

    public static String getDasVersion(Context context) {
        try {
            ApplicationInfo appInfo = context.getPackageManager().getApplicationInfo(
                context.getPackageName(), PackageManager.GET_META_DATA);
            if (appInfo != null && appInfo.metaData != null) {
                String dVersion = appInfo.metaData.getString("dasVersionName");
                if (dVersion != null) {
                    return dVersion;
                }
            }
        }
        catch (Exception e) {}
        try {
            PackageInfo pkgInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            return pkgInfo.versionName;
        }
        catch (Exception e) {}
        return "";
    }

    private static boolean isUsingART() {
        boolean usingART = false;

        String vmVersion = System.getProperty("java.vm.version");
        String[] versionVals = vmVersion.split("\\.");
        if (versionVals.length > 0) {
            int majorVersion = Integer.valueOf(versionVals[0]);
            if (majorVersion >= 2) {
                usingART = true;
            }
        }
        return usingART;
    }

    public static String[] getMiscInfo() {
        ArrayList<String> list = new ArrayList<String>();
        list.add("device.art_in_use");
        list.add(Boolean.toString(isUsingART()));
        return list.toArray(new String[0]);
    }



    public static native void nativeLog(int dasLogLevel, String eventName, String eventValue);

    public static native boolean IsEventEnabledForLevel(String eventName, int dasLogLevel);

    public static native void Configure(String configurationXMLFilePath, String logDirPath, String gameLogDirPath);

    public static native void Close();

    public static native void SetGlobal(String key, String value);

    public static native void EnableNetwork(int reason);

    public static native void DisableNetwork(int reason);

    public static native int GetNetworkingDisabled();

    public static native void SetLevel(String eventName, int dasLogLevel);
}
