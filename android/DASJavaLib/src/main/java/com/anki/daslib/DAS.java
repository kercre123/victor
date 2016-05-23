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

import android.util.Base64;
import android.util.Log;

import java.io.BufferedWriter;
import java.io.UnsupportedEncodingException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.net.URL;
import java.net.URLEncoder;
import java.util.List;
import java.util.ArrayList;
import java.net.HttpURLConnection;
import javax.net.ssl.HttpsURLConnection;

import org.apache.http.message.BasicNameValuePair;
import org.apache.http.NameValuePair;

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
            nativeLog(dasLogLevel, eventName, String.format(eventValue_format, (Object[]) args));
        }
    }

    public static boolean postToServer(String url, String postBody) {
        HttpURLConnection conn = null;
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
            int responseCode = conn.getResponseCode();
            if (responseCode < 200 || responseCode > 299) {
                Log.v(TAG, "Warning! Upload to DAS server returned a response code of " + responseCode);
            }
        } catch (java.io.IOException e) {
            Log.v(TAG, "Error! Unable to upload DAS events: ", e);
            return false;
        } finally {
            if (conn != null) {
                conn.disconnect();
            }
        }

        return true;
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
