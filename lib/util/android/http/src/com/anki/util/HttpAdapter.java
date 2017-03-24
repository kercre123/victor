package com.anki.util.http;

import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.util.Log;

import java.io.*;
import java.net.HttpURLConnection;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;
import java.util.List;
import java.util.Map;
import java.net.URLEncoder;

public class HttpAdapter {

  private static HttpAdapter sInstance = null;
  public static HttpAdapter getInstance() {
    if (sInstance == null) {
      sInstance = new HttpAdapter();
    }
    return sInstance;
  }

  public static final int HTTP_METHOD_GET = 0;
  public static final int HTTP_METHOD_POST = 1;
  public static final int HTTP_METHOD_PUT = 2;
  public static final int HTTP_METHOD_DELETE = 3;
  public static final int HTTP_METHOD_HEAD = 4;

  // Matches error codes in source/anki/driveEngine/util/http/httpRequest.h
  // Borrowed from OSX/iOS NSURLError.h
  private static final int HTTP_RESPONSE_ERROR_TIMED_OUT = -1001;
  private static final int HTTP_RESPONSE_ERROR_CANNOT_CONNECT_TO_HOST = -1004;
  private static final int HTTP_RESPONSE_ERROR_NETWORK_CONNECTION_LOST = -1005;
  private static final int HTTP_RESPONSE_ERROR_DNS_LOOKUP_FAILED = -1006;
  private static final int HTTP_RESPONSE_ERROR_NOT_CONNECTED_TO_INTERNET = -1009;
  private static final int HTTP_RESPONSE_ERROR_FILE_NOT_FOUND = -1100;
  private static final int HTTP_RESPONSE_ERROR_NO_FILE_PERMISSIONS = -1102;

  private final Executor executor;
  private HttpAdapterCallbackThread callbackThread;

  private HttpAdapter() {
    this.executor = Executors.newFixedThreadPool(3);
    this.callbackThread = new HttpAdapterCallbackThread();
    this.callbackThread.start();
    this.callbackThread.waitUntilReady();
  }

  public void startRequest(final long hash, final String uri, final String[] headers,
                           final String[] params, final byte[] body, final int httpMethod,
                           final String storageFilePath, final int requestTimeout) {
    StringBuilder sb = new StringBuilder();
    for (int k=0;k<headers.length;k+=2) {
      sb.append(headers[k]).append("=").append(headers[k+1]).append(" ");
    }

    executor.execute(new Runnable() {
      public void run() {
        URL url;
        HttpURLConnection conn = null;
        OutputStream out;
        int contentLength = 0;
        int totalWritten = 0;
        String[] responseHeaders = new String[0];
        int responseCode = HTTP_RESPONSE_ERROR_NOT_CONNECTED_TO_INTERNET;
        byte[] result = new byte[0];
        try {
          url = new URL(uri);
          conn = (HttpURLConnection) url.openConnection();
          conn.setConnectTimeout(requestTimeout);
          conn.setReadTimeout(requestTimeout);

         for(int i = 0; i < headers.length; i+=2) {
            conn.setRequestProperty(headers[i], headers[i+1]);
          }

          boolean isFileUpload = false;

          switch(httpMethod) {
            case HTTP_METHOD_GET:
              conn.setRequestMethod("GET");
            break;
            case HTTP_METHOD_POST:
              conn.setRequestMethod("POST");
              conn.setDoOutput(true);
              conn.setChunkedStreamingMode(0);
              isFileUpload = storageFilePath.length() > 0 && body.length == 0;
            break;
            case HTTP_METHOD_PUT:
              conn.setRequestMethod("PUT");
              conn.setDoOutput(true);
              conn.setChunkedStreamingMode(0);
              isFileUpload = storageFilePath.length() > 0 && body.length == 0;
            break;
            case HTTP_METHOD_DELETE:
              conn.setRequestMethod("DELETE");
            break;
            case HTTP_METHOD_HEAD:
              conn.setRequestMethod("HEAD");
            break;
          }

          // Write params and body json in to request
          if(body.length > 0 || params.length > 0 || isFileUpload){
            OutputStream os = conn.getOutputStream();
            BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(os, "UTF-8"));
            writer.write(createQueryStringForParameters(params));
            writer.flush();

            if (!isFileUpload) {
              os.write(body, 0, body.length);
              os.flush();
            }
            else {
              OutputStream dataOut = new DataOutputStream(os);
              FileInputStream fileStream = new FileInputStream(new File(storageFilePath));
              final int readSize = 64 * 1024;
              int bufferSize = Math.min(fileStream.available(), readSize);
              byte[] buffer = new byte[bufferSize];
              int bytesRead;

              // read file chunks and write to output stream until all done
              while ((bytesRead = fileStream.read(buffer, 0, bufferSize)) > 0) {
                dataOut.write(buffer, 0, bytesRead);
                bufferSize = Math.min(fileStream.available(), readSize);
              }

              fileStream.close();
              dataOut.flush();
              dataOut.close();
            }

            writer.close();
            os.close();
            conn.connect();
          }

          // Read back response
          responseCode = conn.getResponseCode();
          responseHeaders = createHeaderArrayFromMap(conn.getHeaderFields());
          contentLength = conn.getContentLength();
          InputStream in = conn.getErrorStream();
          if (null == in) {
            in = conn.getInputStream();
          }
          boolean writeToFile;
          if (!isFileUpload && storageFilePath != null && !storageFilePath.isEmpty()) {
            writeToFile = true;
            out = new BufferedOutputStream(new FileOutputStream(storageFilePath));
          } else {
            writeToFile = false;
            if( contentLength != -1 ) {
              out = new ByteArrayOutputStream(contentLength);
            }else{
              out = new ByteArrayOutputStream(512000); // arbitrary default size
            }
          }
          byte[] buffer = new byte[10240];
          while( true ) {
            int len = in.read(buffer);
            if( len == -1 ) {
              break;
            }
            out.write(buffer, 0, len);
            totalWritten += len;
            callHttpRequestDownloadProgressCallbackOnBGThread(hash,
                                                              len,
                                                              totalWritten,
                                                              contentLength);
          }
          in.close();
          out.flush();
          out.close();

          if (!writeToFile) {
            result = ((ByteArrayOutputStream) out).toByteArray();
          }
        }catch (java.net.ConnectException e) {
          Log.v("dasJava", "HttpAdapter caught " + e);
          if (isInternetAvailable()) {
            responseCode = HTTP_RESPONSE_ERROR_CANNOT_CONNECT_TO_HOST;
          } else {
            responseCode = HTTP_RESPONSE_ERROR_NOT_CONNECTED_TO_INTERNET;
          }
        }catch (java.net.SocketTimeoutException e) {
          Log.v("dasJava", "HttpAdapter caught " + e);
          if (isInternetAvailable()) {
            responseCode = HTTP_RESPONSE_ERROR_TIMED_OUT;
          } else {
            responseCode = HTTP_RESPONSE_ERROR_NETWORK_CONNECTION_LOST;
          }
        }catch (java.net.UnknownHostException e) {
          Log.v("dasJava", "HttpAdapter caught " + e);
          if (isInternetAvailable()) {
            responseCode = HTTP_RESPONSE_ERROR_DNS_LOOKUP_FAILED;
          } else {
            responseCode = HTTP_RESPONSE_ERROR_NOT_CONNECTED_TO_INTERNET;
          }
        }catch(java.io.FileNotFoundException e) {
          Log.v("dasJava", "HttpAdapter caught " + e);
          responseCode = HTTP_RESPONSE_ERROR_FILE_NOT_FOUND;
        }catch(java.lang.SecurityException e) {
          Log.v("dasJava", "HttpAdapter caught " + e);
          responseCode = HTTP_RESPONSE_ERROR_NO_FILE_PERMISSIONS;
        }catch(Exception e) {
          Log.v("dasJava", "HttpAdapter caught " + e);
          responseCode = HTTP_RESPONSE_ERROR_NETWORK_CONNECTION_LOST;
        }finally{
          if( conn!=null ) {
            conn.disconnect();
          }
        }

        callHttpRequestCallbackOnBGThread(hash, responseCode, responseHeaders, result);
      }
    });
  }

  private boolean isInternetAvailable() {
    boolean success = false;
    try {
      InternetConnectionTester tester = new InternetConnectionTester("google.com", 80, 1000);
      Thread t = new Thread(tester);
      t.start();
      t.join(2000);
      success = tester.get();
    } catch (Exception e)
    {
      success = false;
    }
    if (!success) {
      Log.v("dasJava", "HttpAdapter.isInternetAvailable() - failed to connect to google.com:80");
    }
    return success;
  }

  private static class InternetConnectionTester implements Runnable {
    private String host;
    private int port;
    private int timeout;
    private boolean success = false;

    public InternetConnectionTester(final String host, final int port, final int timeout) {
      this.host = host;
      this.port = port;
      this.timeout = timeout;
    }

    public void run() {
      try {
        Socket soc = null;
        try {
          soc = new Socket();
          soc.connect(new InetSocketAddress(host, port), timeout);
        } finally {
          if (soc != null) {
            soc.close();
          }
        }
        set(true);
      } catch (IOException e) {
        set(false);
      }
    }

    public synchronized void set(final boolean success) {
      this.success = success;
    }

    public synchronized boolean get() {
      return this.success;
    }
  }

  private String createQueryStringForParameters(final String[] params) throws UnsupportedEncodingException{
    StringBuilder sb = new StringBuilder();
    boolean firstParam = true;

    for(int i = 0; i < params.length; i+=2) {
      if(!firstParam){
        sb.append("&");
      }

      // Don't URL Encode this! We encode this in driveEngine already since the iOS adapter does not url encode
      sb.append(params[i]);
      sb.append("=");
      sb.append(params[i+1]);

      firstParam = false;
    }

    return sb.toString();
  }

  private String[] createHeaderArrayFromMap(final Map<String, List<String>> headerMap) {
    ArrayList<String> headers = new ArrayList<String>();
    if (headerMap != null) {
      for (final String key : headerMap.keySet()) {
        if (key != null) {
          final List<String> values = headerMap.get(key);
          if (values != null && !values.isEmpty()) {
            StringBuilder valueSb = new StringBuilder();
            boolean firstValue = true;
            for (final String val : values) {
              if (firstValue) {
                firstValue = false;
              } else {
                valueSb.append("; ");
              }
              valueSb.append(val);
            }
            String value = valueSb.toString();
            headers.add(key);
            headers.add(value);
          }
        }
      }
    }
    String[] headerArray = new String[headers.size()];
    for (int i = 0 ; i < headers.size(); i++) {
      headerArray[i] = headers.get(i);
    }
    return headerArray;
  }

  private void callHttpRequestCallbackOnBGThread(final long hash, final int responseCode,
                                                 final String[] headers, final byte[] body) {
    callbackThread.post(new Runnable() {
      public void run() {
        NativeHttpRequestCallback(hash, responseCode, headers, body);
      };
    });
  }

  private void callHttpRequestDownloadProgressCallbackOnBGThread(final long hash,
                                                           final long bytesWritten,
                                                           final long totalBytesWritten,
                                                           final long totalBytesExpectedToWrite) {
    callbackThread.post(new Runnable() {
        public void run() {
          NativeHttpRequestDownloadProgressCallback(hash,
                                                    bytesWritten,
                                                    totalBytesWritten,
                                                    totalBytesExpectedToWrite);
        };
    });
  }

  private class HttpAdapterCallbackThread extends HandlerThread {
    private Handler mHandler;

    public HttpAdapterCallbackThread() {
      super("HttpAdapterCallbackThread");
    }

    public synchronized void waitUntilReady() {
      mHandler = new Handler(HttpAdapterCallbackThread.this.getLooper());
    }

    public final boolean post(Runnable r) {
      return mHandler.post(r);
    }

    @Override
    public void run() {
      super.run();
    }
  }

  public static native void NativeHttpRequestCallback(final long hash, final int responseCode,
                                                      final String[] responseHeaders,
                                                      final byte[] responseBody);

  public static native void NativeHttpRequestDownloadProgressCallback(final long hash,
                                                            final long bytesWritten,
                                                            final long totalBytesWritten,
                                                            final long totalBytesExpectedToWrite);
}
