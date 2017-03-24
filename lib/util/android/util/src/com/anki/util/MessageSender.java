package com.anki.util;

import java.util.LinkedList;

/**
 * General purpose messaging utility to send messages out of Java code into
 * places that we can't call directly into, but that can pull messages from
 * Java. The primary use case for this is our C# code in Unity, but could
 * hypothetically be anything.
 *
 * Other components wishing to receive messages from Java should call
 * MessageSender.getReceiver(), which will register and return a new message
 * receiver object.
 *
 * Messages contain a string "tag" identifying the type of message, and then
 * an array of string parameters to specify any additional data. When "sent",
 * messages are added to the message queue of all currently registered
 * receivers.
 *
 * When polled, a receiver will return one message per call to pullMessage()
 * until it no longer has messages, at which point it will return a zero-length
 * string array. Receivers should be polled often to react to new messages.
 */
public class MessageSender {

  public static class Receiver {

    private LinkedList<String[]> mMessages = new LinkedList<String[]>();

    public synchronized String[] pullMessage() {
      return mMessages.size() > 0 ? mMessages.remove() : new String[0];
    }

    public synchronized void addMessage(String[] message) {
      mMessages.add(message);
    }
    
    public void unregister() {
      synchronized (mReceivers) {
        mReceivers.remove(this);
      }
    }
  }

  public static Receiver getReceiver() {
    Receiver receiver = new Receiver();
    synchronized(mReceivers) {
      mReceivers.add(receiver);
    }
    return receiver;
  }

  public static void sendMessage(String tag, String[] parameters) {
    String[] message = null;
    if (parameters == null || parameters.length == 0) {
      message = new String[] { tag };
    }
    else {
      message = new String[1 + parameters.length];
      message[0] = tag;
      System.arraycopy(parameters, 0, message, 1, parameters.length);
    }
    for (Receiver r : mReceivers) {
      r.addMessage(message);
    }
  }

  private static LinkedList<Receiver> mReceivers = new LinkedList<Receiver>();

}
